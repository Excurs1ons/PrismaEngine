#include "EngineCAPI.h"
#include "Engine.h"
#include "CommandLineParser.h"
#include "Application.h"
#include "Export.h"
#include <cstring>
#include <cstdio>
#include <memory>

// 加载 Application 插件 DLL（平台无关）
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    using PluginHandle = HMODULE;
    static PluginHandle OpenPlugin(const char* path) { return LoadLibraryA(path); }
    static void*        GetPluginSym(PluginHandle h, const char* name) { return (void*)GetProcAddress(h, name); }
    static void         ClosePlugin(PluginHandle h) { FreeLibrary(h); }
#else
    #include <dlfcn.h>
    using PluginHandle = void*;
    static PluginHandle OpenPlugin(const char* path) { return dlopen(path, RTLD_LAZY | RTLD_LOCAL); }
    static void*        GetPluginSym(PluginHandle h, const char* name) { return dlsym(h, name); }
    static void         ClosePlugin(PluginHandle h) { dlclose(h); }
#endif

// ============================================================
// IEngine 实现
// 在 Engine DLL 内部包装 Prisma::Engine，通过虚函数表对外暴露
// ============================================================
class EngineImpl : public IEngine {
public:
    explicit EngineImpl(const EngineCreateInfo* info) {
        Prisma::EngineSpecification spec;

        if (info) {
            spec.Name                             = info->Name ? info->Name : "Prisma Engine";
            spec.Headless                         = info->Headless != 0;
            spec.MinLogLevel                      = static_cast<Prisma::LogLevel>(info->MinLogLevel);
            spec.PresentMode                      = static_cast<Prisma::Graphic::PresentMode>(info->PresentMode);
            spec.MaxFPS                           = info->MaxFPS;
            spec.RefreshAssetDatabaseOnStartup    = info->RefreshAssetDatabaseOnStartup != 0;
        }

        m_engine = new Prisma::Engine(spec);
    }

    ~EngineImpl() override {
        delete m_engine;
    }

    void SetCommandLine(int argc, char** argv) override {
        Prisma::Engine::Get().GetCommandLineParser().Parse(argc, argv);
    }

    int Initialize() override {
        return m_engine->Initialize();
    }

    int Run(const char* pluginPath) override {
        PluginHandle plugin = OpenPlugin(pluginPath);
        if (!plugin) {
            std::fprintf(stderr, "[Engine] Failed to load plugin: %s\n", pluginPath);
            return -1;
        }

        using CreateAppFn = Prisma::Application* (*)();
        auto fnCreateApp = (CreateAppFn)GetPluginSym(plugin, "CreateApplication");
        if (!fnCreateApp) {
            std::fprintf(stderr, "[Engine] Plugin missing CreateApplication: %s\n", pluginPath);
            ClosePlugin(plugin);
            return -1;
        }

        Prisma::Application* app = fnCreateApp();
        if (!app) {
            std::fprintf(stderr, "[Engine] Plugin CreateApplication returned null: %s\n", pluginPath);
            ClosePlugin(plugin);
            return -1;
        }

        int result = m_engine->Run(std::unique_ptr<Prisma::Application>(app));
        ClosePlugin(plugin);
        return result;
    }

    void Shutdown() override {
        m_engine->Shutdown();
    }

private:
    Prisma::Engine* m_engine;
};

// CreateInterface — Engine DLL 唯一导出函数
extern "C" {

PRISMA_ENGINE_C_API void* CreateInterface(
    const char* name,
    void*       param)
{
    if (std::strcmp(name, "IEngine") == 0) {
        auto* info = static_cast<const EngineCreateInfo*>(param);
        return new EngineImpl(info);
    }

    return nullptr;
}

} // extern "C"
