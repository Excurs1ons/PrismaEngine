/**
 * @brief Prisma Launcher (Cross-Platform)
 *
 * 唯一职责：加载 Engine DLL，传递插件路径，让引擎自己加载 Application。
 *
 * 工作流程:
 *   1. 从 engine.ini 加载配置
 *   2. 运行时加载 Engine.dll
 *   3. CreateInterface("IEngine", &config) 创建引擎
 *   4. engine->Initialize()
 *   5. engine->Run(pluginPath)   ← 引擎内部 LoadLibrary + CreateApplication
 *   6. engine->Shutdown()
 *   7. 卸载 Engine.dll
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "IEngine.h"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    using LibHandle = HMODULE;
    static LibHandle OpenLibrary(const char* n) { return LoadLibraryA(n); }
    static void*     GetSymbol(LibHandle l, const char* n) { return (void*)GetProcAddress(l, n); }
    static void      CloseLibrary(LibHandle l) { FreeLibrary(l); }
#else
    #include <dlfcn.h>
    using LibHandle = void*;
    static LibHandle OpenLibrary(const char* n) { return dlopen(n, RTLD_LAZY | RTLD_LOCAL); }
    static void*     GetSymbol(LibHandle l, const char* n) { return dlsym(l, n); }
    static void      CloseLibrary(LibHandle l) { dlclose(l); }
#endif

using CreateInterfaceFn = void* (*)(const char* name, void* param);

static EngineCreateInfo LoadEngineConfig() {
    EngineCreateInfo cfg = {
        .Name = "Prisma Launcher", .Headless = 0, .MinLogLevel = 0,
        .PresentMode = 0, .MaxFPS = 0, .RefreshAssetDatabaseOnStartup = 0,
    };
    FILE* fp = fopen("engine.ini", "r");
    if (!fp) return cfg;
    static char buf[256];
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;
        char k[128]={0}, v[256]={0};
        if (sscanf(line, " %127[^=]= %255[^\r\n]", k, v) != 2) continue;
        if      (strcmp(k, "Name") == 0) { strncpy(buf, v, sizeof(buf)-1); buf[sizeof(buf)-1]='\0'; cfg.Name = buf; }
        else if (strcmp(k, "Headless") == 0)                        cfg.Headless = (atoi(v) != 0);
        else if (strcmp(k, "MinLogLevel") == 0)                     cfg.MinLogLevel = atoi(v);
        else if (strcmp(k, "PresentMode") == 0)                     cfg.PresentMode = atoi(v);
        else if (strcmp(k, "MaxFPS") == 0)                          cfg.MaxFPS = (uint32_t)atoi(v);
        else if (strcmp(k, "RefreshAssetDatabaseOnStartup") == 0)   cfg.RefreshAssetDatabaseOnStartup = (atoi(v) != 0);
    }
    fclose(fp);
    return cfg;
}

static const char* PickPlugin(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i)
        if (argv[i][0] != '-') return argv[i];
    return nullptr;
}

int main(int argc, char* argv[]) {
    // ---- 1. 加载配置 ----
    EngineCreateInfo config = LoadEngineConfig();

    // ---- 2. 加载 Engine DLL ----
    static const char* kEngineNames[] = {
#if defined(_WIN32)
        "Prisma.dll",
#elif defined(__APPLE__)
        "libPrisma.dylib",
#else
        "libPrisma.so",
#endif
    };
    LibHandle engineLib = nullptr;
    for (auto n : kEngineNames) { engineLib = OpenLibrary(n); if (engineLib) { std::printf("[Launcher] Loaded Engine: %s\n", n); break; } }
    if (!engineLib) { std::fprintf(stderr, "[Launcher] Failed to load Engine.\n"); return -1; }

    // ---- 3. 获取 CreateInterface ----
    auto createInterface = (CreateInterfaceFn)GetSymbol(engineLib, "CreateInterface");
    if (!createInterface) { std::fprintf(stderr, "[Launcher] Engine missing CreateInterface.\n"); CloseLibrary(engineLib); return -1; }

    // ---- 4. 创建引擎 ----
    IEngine* engine = static_cast<IEngine*>(createInterface("IEngine", &config));
    if (!engine) { std::fprintf(stderr, "[Launcher] Failed to create Engine.\n"); CloseLibrary(engineLib); return -1; }

    // ---- 5. 初始化 ----
    std::printf("[Launcher] Initializing engine...\n");
    if (engine->Initialize() != 0) { std::fprintf(stderr, "[Launcher] Engine init failed.\n"); delete engine; CloseLibrary(engineLib); return -1; }
    std::printf("[Launcher] Engine initialized.\n");

    // ---- 6. 运行（引擎内部加载插件 DLL） ----
    const char* plugin = PickPlugin(argc, argv);
    if (!plugin) plugin = "Template2D.dll";
    std::printf("[Launcher] Running engine with plugin: %s\n", plugin);
    engine->Run(plugin);

    // ---- 7. 关闭 ----
    std::printf("[Launcher] Shutting down...\n");
    engine->Shutdown();
    delete engine;
    CloseLibrary(engineLib);
    std::printf("[Launcher] Goodbye.\n");
    return 0;
}
