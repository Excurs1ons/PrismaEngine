#include "CoreCLRHost.h"
#include "Logger.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Prisma {
namespace Scripting {

// ============================================================================
// hostfxr API 类型定义 (跨平台适配)
// ============================================================================

#ifdef _WIN32
    #include <windows.h>
    using char_t = wchar_t;
    #define STR(s) L##s
    static std::string hostfxrFilename() { return "hostfxr.dll"; }
    static std::string runtimeConfigFilename() { return "GameScripts.runtimeconfig.json"; }
#else
    #include <dlfcn.h>
    using char_t = char;
    #define STR(s) s
    static std::string hostfxrFilename() { return "libhostfxr.so"; }
    static std::string runtimeConfigFilename() { return "GameScripts.runtimeconfig.json"; }
#endif

using hostfxr_handle = void*;

struct hostfxr_initialize_parameters {
    size_t size;
    const char_t* host_path;
    const char_t* dotnet_root;
};

using hostfxr_initialize_for_runtime_config_fn = int32_t (*)(
    const char_t* runtime_config_path,
    const hostfxr_initialize_parameters* parameters,
    hostfxr_handle* host_context_handle);

using hostfxr_get_runtime_delegate_fn = int32_t (*)(
    hostfxr_handle host_context_handle,
    int32_t delegate_type,
    void** delegate);

using hostfxr_close_fn = int32_t (*)(
    hostfxr_handle host_context_handle);

using load_assembly_and_get_function_pointer_fn = int32_t (*)(
    const char_t* assembly_path,
    const char_t* type_name,
    const char_t* method_name,
    const char_t* delegate_type_name,
    void* reserved,
    void** delegate);

#define UNMANAGEDCALLERSONLY_METHOD ((const char_t*)-1)

// ============================================================================
// 平台抽象辅助
// ============================================================================

#ifdef _WIN32
static std::wstring to_native(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wstr[0], len);
    return wstr;
}
static void* loadLibrary(const char_t* path) { return (void*)LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS); }
static void* getExport(void* lib, const char* name) { return (void*)GetProcAddress((HMODULE)lib, name); }
static void freeLibrary(void* lib) { if (lib) FreeLibrary((HMODULE)lib); }
#else
static std::string to_native(const std::string& utf8) { return utf8; }
static void* loadLibrary(const char_t* path) { return dlopen(path, RTLD_LAZY | RTLD_LOCAL); }
static void* getExport(void* lib, const char* name) { return dlsym(lib, name); }
static void freeLibrary(void* lib) { if (lib) dlclose(lib); }
#endif

// ============================================================================
// CoreCLRHost 实现
// ============================================================================

bool CoreCLRHost::Initialize(const std::string& scriptsDir) {
    if (m_initialized) return true;
    m_scriptsDir = scriptsDir;

    // 1. 加载 hostfxr 库
    // 在 self-contained 模式下，它应该在程序集目录下
    fs::path hostfxrPath = fs::path(scriptsDir) / hostfxrFilename();
    m_hostfxrLib = loadLibrary(to_native(hostfxrPath.string()).c_str());
    
    if (!m_hostfxrLib) {
        LOG_ERROR("CoreCLRHost", "Failed to load {0}", hostfxrPath.string());
        return false;
    }

    // 2. 获取函数指针
    auto init_fn = (hostfxr_initialize_for_runtime_config_fn)getExport(m_hostfxrLib, "hostfxr_initialize_for_runtime_config");
    auto get_delegate_fn = (hostfxr_get_runtime_delegate_fn)getExport(m_hostfxrLib, "hostfxr_get_runtime_delegate");
    auto close_fn = (hostfxr_close_fn)getExport(m_hostfxrLib, "hostfxr_close");

    if (!init_fn || !get_delegate_fn || !close_fn) {
        LOG_ERROR("CoreCLRHost", "Failed to get hostfxr exports");
        return false;
    }

    // 3. 初始化运行时
    fs::path configPath = fs::path(scriptsDir) / runtimeConfigFilename();
    hostfxr_handle ctx = nullptr;
    int rc = init_fn(to_native(configPath.string()).c_str(), nullptr, &ctx);
    if (rc != 0 || !ctx) {
        LOG_ERROR("CoreCLRHost", "hostfxr_initialize failed with rc: {0}", rc);
        return false;
    }

    // 4. 获取程序集加载委托
    rc = get_delegate_fn(ctx, 1 /* hdt_load_assembly_and_get_function_pointer */, (void**)&m_loadAssemblyAndGetFn);
    close_fn(ctx);

    if (rc != 0 || !m_loadAssemblyAndGetFn) {
        LOG_ERROR("CoreCLRHost", "Failed to get load_assembly delegate, rc: {0}", rc);
        return false;
    }

    m_initialized = true;
    LOG_INFO("CoreCLRHost", "Successfully initialized .NET Runtime (CoreCLR)");
    return true;
}

void* CoreCLRHost::GetFunctionPointer(const std::string& assemblyPath, const std::string& typeName, const std::string& methodName) {
    if (!m_initialized || !m_loadAssemblyAndGetFn) return nullptr;

    void* fn = nullptr;
    int rc = ((load_assembly_and_get_function_pointer_fn)m_loadAssemblyAndGetFn)(
        to_native(assemblyPath).c_str(),
        to_native(typeName).c_str(),
        to_native(methodName).c_str(),
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        &fn
    );

    if (rc != 0) {
        LOG_ERROR("CoreCLRHost", "Failed to get function pointer for {0}.{1}, rc: {0}", typeName, methodName, rc);
        return nullptr;
    }

    return fn;
}

void CoreCLRHost::Shutdown() {
    if (m_hostfxrLib) {
        freeLibrary(m_hostfxrLib);
        m_hostfxrLib = nullptr;
    }
    m_initialized = false;
}

} // namespace Scripting
} // namespace Prisma
