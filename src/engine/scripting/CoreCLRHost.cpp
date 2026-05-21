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
    [[maybe_unused]] static std::string runtimeConfigFilename() { return "PrismaEngine.Host.runtimeconfig.json"; }
#else
    #include <dlfcn.h>
    using char_t = char;
    #define STR(s) s
    static std::string hostfxrFilename() { return "libhostfxr.so"; }
    [[maybe_unused]] static std::string runtimeConfigFilename() { return "PrismaEngine.Host.runtimeconfig.json"; }
#endif

using hostfxr_handle = void*;

struct hostfxr_initialize_parameters {
    size_t size;
    const char_t* host_path;
    const char_t* dotnet_root;
};

// 自包含 publish 必须使用 hostfxr_initialize_for_dotnet_command_line。
// hostfxr_initialize_for_runtime_config 不支持 self-contained 模式的
// runtimeconfig.json（报错 "Initialization for self-contained components..."）。
using hostfxr_initialize_for_dotnet_command_line_fn = int32_t (*)(
    int32_t argc,
    const char_t** argv,
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
    auto initCmdLineFn = (hostfxr_initialize_for_dotnet_command_line_fn)
        getExport(m_hostfxrLib, "hostfxr_initialize_for_dotnet_command_line");
    auto getDelegateFn = (hostfxr_get_runtime_delegate_fn)
        getExport(m_hostfxrLib, "hostfxr_get_runtime_delegate");
    auto closeFn = (hostfxr_close_fn)
        getExport(m_hostfxrLib, "hostfxr_close");

    if (!initCmdLineFn || !getDelegateFn || !closeFn) {
        LOG_ERROR("CoreCLRHost", "hostfxr missing required exports");
        return false;
    }

    // 3. 禁止系统路径探测（仅使用 scriptsDir 内的运行时）
#ifdef _WIN32
    SetEnvironmentVariableW(L"DOTNET_MULTILEVEL_LOOKUP", L"0");
#else
    setenv("DOTNET_MULTILEVEL_LOOKUP", "0", 1);
#endif

    // 4. 通过自包含 Host 程序集初始化运行时
    //    PrismaEngine.Host 提供运行时环境，GameScripts 作为插件由 ScriptEngine 单独加载
    std::string dllPath = (fs::path(scriptsDir) / "PrismaEngine.Host.dll").string();
    auto nativePath = to_native(dllPath);
    std::vector<char_t> dllBuf(nativePath.begin(), nativePath.end());
    dllBuf.push_back(0); // null-terminate
    const char_t* argv[] = { dllBuf.data() };

    int32_t rc = initCmdLineFn(1, argv, nullptr, (hostfxr_handle*)&m_hostContext);
    if (rc != 0 || !m_hostContext) {
        LOG_ERROR("CoreCLRHost", "hostfxr_initialize_for_dotnet_command_line failed: {0}", rc);
        if (m_hostContext) closeFn((hostfxr_handle)m_hostContext);
        m_hostContext = nullptr;
        return false;
    }

    // 5. 获取 load_assembly_and_get_function_pointer 委托
    constexpr int32_t HDT_LOAD_ASSEMBLY_AND_GET_FN_PTR = 5;
    rc = getDelegateFn((hostfxr_handle)m_hostContext,
                       HDT_LOAD_ASSEMBLY_AND_GET_FN_PTR,
                       &m_loadAssemblyAndGetFn);
    if (rc != 0 || !m_loadAssemblyAndGetFn) {
        LOG_ERROR("CoreCLRHost", "Failed to get runtime delegate, rc: {0}", rc);
        closeFn((hostfxr_handle)m_hostContext);
        m_hostContext = nullptr;
        return false;
    }

    // 6. 完成 - 保持 m_hostContext 打开，后续 GetFunctionPointer 需要它
    m_initialized = true;
    LOG_INFO("CoreCLRHost", "CoreCLR initialized (scripts: {0})", scriptsDir);
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
        LOG_ERROR("CoreCLRHost", "Failed to get function pointer for {0}.{1}, rc: {2}", typeName, methodName, rc);
        return nullptr;
    }

    return fn;
}

void CoreCLRHost::Shutdown() {
    if (m_hostContext) {
        auto closeFn = (hostfxr_close_fn)getExport(m_hostfxrLib, "hostfxr_close");
        if (closeFn) closeFn((hostfxr_handle)m_hostContext);
        m_hostContext = nullptr;
    }
    if (m_hostfxrLib) {
        freeLibrary(m_hostfxrLib);
        m_hostfxrLib = nullptr;
    }
    m_loadAssemblyAndGetFn = nullptr;
    m_initialized = false;
}

} // namespace Scripting
} // namespace Prisma
