#include "CoreCLRHost.h"
#include "Logger.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Prisma {
namespace Scripting {

// ============================================================================
// hostfxr API 类型定义（来自官方 hostfxr.h / coreclr_delegates.h）
// Self-contained publish 模式下，hostfxr.dll 与程序集同目录
// ============================================================================

using hostfxr_handle = void*;

struct hostfxr_initialize_parameters {
    size_t size;
    const wchar_t* host_path;
    const wchar_t* dotnet_root;
};

using hostfxr_initialize_for_dotnet_command_line_fn = int32_t (*)(
    int32_t argc,
    const wchar_t** argv,
    const hostfxr_initialize_parameters* parameters,
    hostfxr_handle* host_context_handle);

using hostfxr_get_runtime_delegate_fn = int32_t (*)(
    hostfxr_handle host_context_handle,
    int32_t delegate_type,
    void** delegate);

using hostfxr_close_fn = int32_t (*)(
    hostfxr_handle host_context_handle);

using load_assembly_and_get_function_pointer_fn = int32_t (*)(
    const wchar_t* assembly_path,
    const wchar_t* type_name,
    const wchar_t* method_name,
    const wchar_t* delegate_type_name,
    void* reserved,
    void** delegate);

#define UNMANAGEDCALLERSONLY_METHOD ((const wchar_t*)-1)

// ============================================================================
// 平台抽象
// ============================================================================

#ifdef _WIN32
#include <windows.h>

static std::wstring widen(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wstr[0], len);
    return wstr;
}

static void* loadLibrary(const wchar_t* path) {
    return (void*)LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}

static void* getExport(void* lib, const char* name) {
    return (void*)GetProcAddress((HMODULE)lib, name);
}

static void freeLibrary(void* lib) {
    if (lib) FreeLibrary((HMODULE)lib);
}

static std::string hostfxrFilename() { return "hostfxr.dll"; }

#else
// Linux/macOS 占位
#error "CoreCLRHost: Only Windows is supported yet"
static std::wstring widen(const std::string&) { return {}; }
static void* loadLibrary(const wchar_t*) { return nullptr; }
static void* getExport(void*, const char*) { return nullptr; }
static void freeLibrary(void*) {}
static std::string hostfxrFilename() { return "libhostfxr.so"; }
static std::string runtimeConfigFilename() { return "GameScripts.runtimeconfig.json"; }
#endif

// ============================================================================
// CoreCLRHost 实现
// ============================================================================

bool CoreCLRHost::Initialize(const std::string& scriptsDir) {
    if (m_initialized) {
        LOG_WARNING("CoreCLRHost", "Already initialized");
        return true;
    }

    if (!fs::is_directory(scriptsDir)) {
        LOG_ERROR("CoreCLRHost", "Scripts directory not found: {0}", scriptsDir);
        return false;
    }

    m_scriptsDir = scriptsDir;

    // 1. 在 scriptsDir 下找 hostfxr.dll
    fs::path fxrPath = fs::path(scriptsDir) / hostfxrFilename();
    if (!fs::exists(fxrPath)) {
        LOG_ERROR("CoreCLRHost", "Not found: {0} (run 'dotnet publish --self-contained' first)",
                  fxrPath.string());
        return false;
    }

    // 2. 加载 hostfxr
    std::wstring wFxrPath = widen(fxrPath.string());
    m_hostfxrLib = loadLibrary(wFxrPath.c_str());
    if (!m_hostfxrLib) {
        LOG_ERROR("CoreCLRHost", "Failed to load {0}", fxrPath.string());
        return false;
    }

    // 3. 解析导出函数（自包含模式使用 _for_dotnet_command_line）
    auto initCmdLineFn = (hostfxr_initialize_for_dotnet_command_line_fn)
        getExport(m_hostfxrLib, "hostfxr_initialize_for_dotnet_command_line");
    auto getDelegateFn = (hostfxr_get_runtime_delegate_fn)
        getExport(m_hostfxrLib, "hostfxr_get_runtime_delegate");
    auto closeFn = (hostfxr_close_fn)
        getExport(m_hostfxrLib, "hostfxr_close");

    if (!initCmdLineFn || !getDelegateFn || !closeFn) {
        LOG_ERROR("CoreCLRHost", "hostfxr missing required exports");
        freeLibrary(m_hostfxrLib);
        m_hostfxrLib = nullptr;
        return false;
    }

    // 4. 禁止系统路径探测
    SetEnvironmentVariableW(L"DOTNET_MULTILEVEL_LOOKUP", L"0");

    // 5. 通过自包含 DLL 路径初始化运行时（argv[0] = 主程序集）
    std::string dllPath = (fs::path(scriptsDir) / "GameScripts.dll").string();
    std::wstring wDllPath = widen(dllPath);
    const wchar_t* argv[] = { wDllPath.c_str() };

    int32_t rc = initCmdLineFn(1, argv, nullptr, (hostfxr_handle*)&m_hostContext);
    if (rc != 0 || !m_hostContext) {
        LOG_ERROR("CoreCLRHost", "hostfxr_initialize_for_dotnet_command_line failed: {0}", rc);
        if (m_hostContext) closeFn((hostfxr_handle)m_hostContext);
        freeLibrary(m_hostfxrLib);
        m_hostfxrLib = nullptr;
        return false;
    }

    // 6. 获取 load_assembly_and_get_function_pointer 委托
    constexpr int32_t HDT_LOAD_ASSEMBLY_AND_GET_FN_PTR = 5;
    rc = getDelegateFn((hostfxr_handle)m_hostContext,
                       HDT_LOAD_ASSEMBLY_AND_GET_FN_PTR,
                       &m_loadAssemblyAndGetFn);
    if (rc != 0 || !m_loadAssemblyAndGetFn) {
        LOG_ERROR("CoreCLRHost", "Failed to get runtime delegate");
        closeFn((hostfxr_handle)m_hostContext);
        freeLibrary(m_hostfxrLib);
        m_hostfxrLib = nullptr;
        return false;
    }

    // 7. 完成（保持 context 打开，后续 GetFunctionPointer 需要它）
    m_initialized = true;
    LOG_INFO("CoreCLRHost", "CoreCLR initialized (scripts: {0})", scriptsDir);
    return true;
}

void* CoreCLRHost::GetFunctionPointer(const std::string& assemblyPath,
                                       const std::string& typeName,
                                       const std::string& methodName) {
    if (!m_initialized || !m_loadAssemblyAndGetFn) {
        LOG_ERROR("CoreCLRHost", "Not initialized");
        return nullptr;
    }

    // 如果 assemblyPath 是相对路径，补全为 scriptsDir + 文件名
    std::string fullPath = assemblyPath;
    if (!fs::path(assemblyPath).is_absolute()) {
        fullPath = (fs::path(m_scriptsDir) / assemblyPath).string();
    }

    auto loadFn = (load_assembly_and_get_function_pointer_fn)m_loadAssemblyAndGetFn;
    std::wstring wAssembly = widen(fullPath);
    std::wstring wType     = widen(typeName);
    std::wstring wMethod   = widen(methodName);

    void* fn = nullptr;
    int32_t rc = loadFn(wAssembly.c_str(), wType.c_str(), wMethod.c_str(),
                        UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn);

    if (rc != 0 || !fn) {
        LOG_ERROR("CoreCLRHost", "Failed to resolve {0}::{1} (rc={2})",
                  typeName, methodName, rc);
        return nullptr;
    }

    LOG_DEBUG("CoreCLRHost", "Resolved {0}::{1}", typeName, methodName);
    return fn;
}

void CoreCLRHost::Shutdown() {
    if (!m_initialized) return;

    if (m_hostContext) {
        auto closeFn = (hostfxr_close_fn)getExport(m_hostfxrLib, "hostfxr_close");
        if (closeFn) closeFn((hostfxr_handle)m_hostContext);
    }

    m_initialized = false;
    m_loadAssemblyAndGetFn = nullptr;
    m_hostContext = nullptr;

    if (m_hostfxrLib) {
        freeLibrary(m_hostfxrLib);
        m_hostfxrLib = nullptr;
    }

    LOG_DEBUG("CoreCLRHost", "Shut down");
}

} // namespace Scripting
} // namespace Prisma
