#include "EditorAPI.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "core/Node.h"
#include "transform/Transform.h"
#include "logger/Logger.h"
#include "logger/LogEntry.h"
#include "config/EngineConfig.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ISwapChain.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <atomic>
#include <filesystem>
#include <vector>
#include <ctime>
#include <iomanip>

namespace Prisma {
namespace Scripting {
namespace {

// ===== 编辑器全局状态（线程安全） =====
// 选择状态：编辑器 UI 独立于引擎场景的选择状态
static std::atomic<uint64_t> s_selectedEntityId{0};
// 视口尺寸：编辑器视口的像素尺寸
static std::atomic<uint32_t> s_viewportWidth{1920};
static std::atomic<uint32_t> s_viewportHeight{1080};

// ===== JSON 辅助函数 =====

static std::string EscapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

static std::string TimestampToString(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32) || defined(_MSC_VER)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

static const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

// ===== 场景辅助函数 =====

static Scene* GetCurrentScene() {
    auto* sm = Engine::Get().GetSceneManager();
    return sm ? sm->GetCurrentScene() : nullptr;
}

static std::shared_ptr<Transform> GetTransform(uint64_t id) {
    auto* scene = GetCurrentScene();
    if (!scene) return nullptr;
    return scene->GetComponent<Transform>(Node(static_cast<uint32_t>(id)));
}

static void AppendNodeToJson(Scene* scene, Node node, std::string& json, bool& first) {
    if (!first) json += ",";
    first = false;
    json += "{\"id\":";
    json += std::to_string(node.handle);
    json += ",\"name\":\"";
    json += EscapeJson(scene->GetNodeName(node));
    json += "\"";

    auto children = scene->GetChildren(node);
    if (!children.empty()) {
        json += ",\"children\":[";
        bool childFirst = true;
        for (const auto& child : children) {
            AppendNodeToJson(scene, child, json, childFirst);
        }
        json += "]";
    }
    json += "}";
}

// =====================================================================
// Scene Group (4 functions)
// =====================================================================

static void* S_SceneGetHierarchy() {
    auto* scene = GetCurrentScene();
    if (!scene) return strdup("[]");

    auto rootNodes = scene->GetRootNodes();
    std::string json = "[";
    bool first = true;
    for (const auto& root : rootNodes) {
        AppendNodeToJson(scene, root, json, first);
    }
    json += "]";
    return strdup(json.c_str());
}

static uint64_t S_SceneCreateEntity(const char* name) {
    auto* scene = GetCurrentScene();
    if (!scene) return 0;
    Node node = scene->CreateNode(name ? name : "Entity");
    return node.handle;
}

static bool S_SceneDeleteEntity(uint64_t id) {
    auto* scene = GetCurrentScene();
    if (!scene) return false;
    scene->RemoveNode(Node(static_cast<uint32_t>(id)));
    return true;
}

static bool S_SceneRenameEntity(uint64_t id, const char* name) {
    auto* scene = GetCurrentScene();
    if (!scene || !name) return false;
    scene->SetNodeName(Node(static_cast<uint32_t>(id)), name);
    return true;
}

// =====================================================================
// Transform Group (6 functions)
// =====================================================================

static void S_TransformGetPosition(uint64_t id, float* outX, float* outY, float* outZ) {
    if (!outX || !outY || !outZ) return;
    auto transform = GetTransform(id);
    if (transform) {
        auto pos = transform->GetPosition();
        *outX = pos.x;
        *outY = pos.y;
        *outZ = pos.z;
    } else {
        *outX = *outY = *outZ = 0.0f;
    }
}

static void S_TransformSetPosition(uint64_t id, float x, float y, float z) {
    auto transform = GetTransform(id);
    if (transform) {
        transform->SetPosition(Vector3(x, y, z));
    }
}

static void S_TransformGetRotation(uint64_t id, float* outX, float* outY, float* outZ, float* outW) {
    if (!outX || !outY || !outZ || !outW) return;
    auto transform = GetTransform(id);
    if (transform) {
        auto q = transform->GetRotation();
        *outX = q.x;
        *outY = q.y;
        *outZ = q.z;
        *outW = q.w;
    } else {
        *outX = *outY = *outZ = 0.0f;
        *outW = 1.0f;
    }
}

static void S_TransformSetRotation(uint64_t id, float x, float y, float z, float w) {
    auto transform = GetTransform(id);
    if (transform) {
        // glm::quat stores (w, x, y, z), API passes (x, y, z, w)
        transform->SetRotation(Quaternion(w, x, y, z));
    }
}

static void S_TransformGetScale(uint64_t id, float* outX, float* outY, float* outZ) {
    if (!outX || !outY || !outZ) return;
    auto transform = GetTransform(id);
    if (transform) {
        auto s = transform->GetScale();
        *outX = s.x;
        *outY = s.y;
        *outZ = s.z;
    } else {
        *outX = *outY = *outZ = 1.0f;
    }
}

static void S_TransformSetScale(uint64_t id, float x, float y, float z) {
    auto transform = GetTransform(id);
    if (transform) {
        transform->SetScale(Vector3(x, y, z));
    }
}

// =====================================================================
// Selection Group (3 functions)
// =====================================================================

static uint64_t S_SelectionGetSelected() {
    return s_selectedEntityId.load();
}

static void S_SelectionSetSelected(uint64_t id) {
    s_selectedEntityId.store(id);
}

static void S_SelectionClear() {
    s_selectedEntityId.store(0);
}

// =====================================================================
// Viewport Group (3 functions)
// =====================================================================

static void S_ViewportGetSize(uint32_t* outW, uint32_t* outH) {
    if (outW) *outW = s_viewportWidth.load();
    if (outH) *outH = s_viewportHeight.load();
}

static void S_ViewportSetSize(uint32_t w, uint32_t h) {
    s_viewportWidth.store(w);
    s_viewportHeight.store(h);
}

static bool S_ViewportCaptureScreenshot(const char* path) {
    if (!path) return false;
    auto* rs = Engine::Get().GetRenderSystem();
    if (!rs) return false;
    auto* device = rs->GetDevice();
    if (!device) return false;
    auto* swapChain = device->GetSwapChain();
    if (!swapChain) return false;
    return swapChain->Screenshot(path);
}

// =====================================================================
// Asset Group (3 functions)
// =====================================================================

static void* S_AssetBrowseDirectory(const char* path) {
    if (!path) return strdup("[]");

    std::string json = "[";
    bool first = true;

    try {
        std::filesystem::path dirPath(path);
        if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                if (!first) json += ",";
                first = false;
                auto p = entry.path();
                json += "{";
                json += "\"name\":\"" + EscapeJson(p.filename().string()) + "\",";
                json += "\"path\":\"" + EscapeJson(p.lexically_normal().string()) + "\",";
                json += "\"isDirectory\":" + std::string(entry.is_directory() ? "true" : "false") + ",";
                json += "\"size\":" + std::to_string(entry.is_regular_file() ? entry.file_size() : 0);
                json += "}";
            }
        }
    } catch (const std::exception&) {
        // If directory iteration fails, return empty array
    }

    json += "]";
    return strdup(json.c_str());
}

static void* S_AssetGetInfo(const char* path) {
    if (!path || !std::filesystem::exists(path)) {
        return strdup("{}");
    }

    try {
        std::filesystem::path p(path);
        auto ftime = std::filesystem::last_write_time(p);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now());
        std::time_t t = std::chrono::system_clock::to_time_t(sctp);
        std::tm tm{};
#if defined(_WIN32) || defined(_MSC_VER)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream timeStr;
        timeStr << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");

        std::string json = "{";
        json += "\"name\":\"" + EscapeJson(p.filename().string()) + "\",";
        json += "\"path\":\"" + EscapeJson(p.lexically_normal().string()) + "\",";
        json += "\"size\":" + std::to_string(std::filesystem::file_size(p)) + ",";
        json += "\"isDirectory\":" + std::string(std::filesystem::is_directory(p) ? "true" : "false") + ",";
        json += "\"lastModified\":\"" + timeStr.str() + "\"";
        json += "}";
        return strdup(json.c_str());
    } catch (const std::exception&) {
        return strdup("{}");
    }
}

static bool S_AssetImport(const char* sourcePath) {
    if (!sourcePath || !std::filesystem::exists(sourcePath)) return false;

    try {
        // 复制文件到项目资产目录
        // 获取引擎资产目录
        std::string assetsDir = "assets/imported";
        std::filesystem::create_directories(assetsDir);

        std::filesystem::path src(sourcePath);
        std::filesystem::path dst = std::filesystem::path(assetsDir) / src.filename();
        std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// =====================================================================
// Editor Group (3 functions)
// =====================================================================

static void* S_EditorGetStatus() {
    auto& engine = Engine::Get();
    auto* scene = GetCurrentScene();

    std::string json = "{";
    json += "\"fps\":" + std::to_string(engine.GetFPS()) + ",";
    json += "\"sceneName\":\"" + EscapeJson(scene ? scene->GetName() : "None") + "\",";
    json += "\"entityCount\":" + std::to_string(scene ? scene->GetNodes().size() : 0) + ",";
    json += "\"viewportWidth\":" + std::to_string(s_viewportWidth.load()) + ",";
    json += "\"viewportHeight\":" + std::to_string(s_viewportHeight.load());
    json += "}";
    return strdup(json.c_str());
}

static void* S_EditorGetEngineInfo() {
    std::string json = "{";
    json += "\"engineVersion\":\"" PRISMA_STRINGIFY(PRISMA_ENGINE_VERSION_MAJOR) "."
                              PRISMA_STRINGIFY(PRISMA_ENGINE_VERSION_MINOR) "."
                              PRISMA_STRINGIFY(PRISMA_ENGINE_VERSION_PATCH) "\",";
    json += "\"engineName\":\"" PRISMA_ENGINE_NAME "\",";
    json += "\"renderer\":\"Vulkan\",";
    json += "\"apiVersion\":\"1.3\"";
    json += "}";
    return strdup(json.c_str());
}

static bool S_EditorExecuteCommand(const char* cmd) {
    if (!cmd) return false;

    std::string command(cmd);

    if (command == "quit" || command == "exit") {
        Engine::Get().Shutdown();
        return true;
    }

    if (command == "save") {
        auto* scene = GetCurrentScene();
        if (!scene) return false;
        return scene->Serialize("scene.json");
    }

    if (command.rfind("load ", 0) == 0) {
        std::string path = command.substr(5);
        if (path.empty()) return false;
        auto* scene = GetCurrentScene();
        if (!scene) return false;
        return scene->Deserialize(path);
    }

    return false;
}

// =====================================================================
// Log Group (2 functions)
// =====================================================================

static void* S_LogGetLogs(int level, int count) {
    // 忽略 level 过滤器（用于未来扩展），返回最近的日志
    size_t limit = (count > 0) ? static_cast<size_t>(count) : 50;
    auto logs = Logger::Get().GetRecentLogs(limit);

    std::string json = "[";
    bool first = true;
    for (const auto& entry : logs) {
        if (!first) json += ",";
        first = false;
        json += "{";
        json += "\"timestamp\":\"" + TimestampToString(entry.timestamp) + "\",";
        json += "\"level\":\"" + std::string(LogLevelToString(entry.level)) + "\",";
        json += "\"message\":\"" + EscapeJson(entry.message) + "\",";
        json += "\"subsystem\":\"" + EscapeJson(entry.category) + "\"";
        json += "}";
    }
    json += "]";
    return strdup(json.c_str());
}

static void S_LogClear() {
    // Logger 没有直接的清除方法，但我们可以通过读取所有日志来"清空"
    // 实际上 Logger 的环形缓冲区不支持清除，但可以读取并丢弃
    Logger::Get().GetRecentLogs(10000);
}

// =====================================================================
// Memory Management
// =====================================================================

static void S_FreeString(void* ptr) {
    if (ptr) std::free(ptr);
}

} // anonymous namespace

void FillEditorAPI(EditorAPI& api) {
    // Scene
    api.sceneGetHierarchy = S_SceneGetHierarchy;
    api.sceneCreateEntity = S_SceneCreateEntity;
    api.sceneDeleteEntity = S_SceneDeleteEntity;
    api.sceneRenameEntity = S_SceneRenameEntity;

    // Transform
    api.transformGetPosition = S_TransformGetPosition;
    api.transformSetPosition = S_TransformSetPosition;
    api.transformGetRotation = S_TransformGetRotation;
    api.transformSetRotation = S_TransformSetRotation;
    api.transformGetScale = S_TransformGetScale;
    api.transformSetScale = S_TransformSetScale;

    // Selection
    api.selectionGetSelected = S_SelectionGetSelected;
    api.selectionSetSelected = S_SelectionSetSelected;
    api.selectionClear = S_SelectionClear;

    // Viewport
    api.viewportGetSize = S_ViewportGetSize;
    api.viewportSetSize = S_ViewportSetSize;
    api.viewportCaptureScreenshot = S_ViewportCaptureScreenshot;

    // Asset
    api.assetBrowseDirectory = S_AssetBrowseDirectory;
    api.assetGetInfo = S_AssetGetInfo;
    api.assetImport = S_AssetImport;

    // Editor
    api.editorGetStatus = S_EditorGetStatus;
    api.editorGetEngineInfo = S_EditorGetEngineInfo;
    api.editorExecuteCommand = S_EditorExecuteCommand;

    // Log
    api.logGetLogs = S_LogGetLogs;
    api.logClear = S_LogClear;

    // Memory
    api.freeString = S_FreeString;

    // Version validation
    api.structSize = sizeof(EditorAPI);
}

} // namespace Scripting
} // namespace Prisma

extern "C" {

ENGINE_API void GetEditorAPI(Prisma::Scripting::EditorAPI* outApi) {
    Prisma::Scripting::FillEditorAPI(*outApi);
}

}
