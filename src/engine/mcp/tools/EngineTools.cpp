#include "EngineTools.h"
#include "app/Engine.h"
#include "scene/SceneManager.h"
#include "scene/Scene.h"
#include "Build.h"

namespace Prisma {
namespace MCP {

// ---- EngineStatusTool ----

EngineStatusTool::EngineStatusTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json EngineStatusTool::GetInputSchema() const {
    return {{"type", "object"}, {"properties", {}}};
}

nlohmann::json EngineStatusTool::Execute(const nlohmann::json& /*args*/) {
    auto* sceneManager = m_Engine->GetSceneManager();
    auto* scene = sceneManager ? sceneManager->GetCurrentScene() : nullptr;

    return {
        {"running", m_Engine->IsRunning()},
        {"engine_name", m_Engine->GetSpecification().Name},
        {"headless", m_Engine->GetSpecification().Headless},
        {"scene_loaded", scene != nullptr},
        {"scene_name", scene ? scene->GetName() : ""},
#if defined(PRISMA_ENABLE_MCP)
        {"mcp_enabled", true},
#else
        {"mcp_enabled", false},
#endif
    };
}

// ---- EngineStateHashTool ----

EngineStateHashTool::EngineStateHashTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json EngineStateHashTool::GetInputSchema() const {
    return {{"type", "object"}, {"properties", {}}};
}

nlohmann::json EngineStateHashTool::Execute(const nlohmann::json& /*args*/) {
    // This is handled by the MCPServer directly via handleGetStateHash
    // This tool exists so it appears in tool discovery
    return {{"note", "Use mcp/get_state_hash directly for session hash access"}};
}

// ---- EngineBuildInfoTool ----

EngineBuildInfoTool::EngineBuildInfoTool(Engine* engine) : m_Engine(engine) {}

nlohmann::json EngineBuildInfoTool::GetInputSchema() const {
    return {{"type", "object"}, {"properties", {}}};
}

nlohmann::json EngineBuildInfoTool::Execute(const nlohmann::json& /*args*/) {
    return {
        {"platform",
#if defined(_WIN32)
            "windows"
#elif defined(__linux__)
            "linux"
#elif defined(__ANDROID__)
            "android"
#else
            "unknown"
#endif
        },
        {"compiler",
#if defined(_MSC_VER)
            "msvc"
#elif defined(__clang__)
            "clang"
#elif defined(__GNUC__)
            "gcc"
#else
            "unknown"
#endif
        },
        {"configuration",
#if defined(_DEBUG) || defined(DEBUG)
            "debug"
#else
            "release"
#endif
        },
        {"mcp_version", "1.0.0"},
    };
}

} // namespace MCP
} // namespace Prisma
