#include "Engine.h"
#include "Application.h"
#include "Logger.h"

#ifdef __ANDROID__
#include <dlfcn.h>

extern "C" int SDL_main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "PrismaEngine Android";
    spec.RefreshAssetDatabaseOnStartup = false;

    Prisma::Engine engine(spec);
    if (engine.Initialize() != 0) {
        LOG_FATAL("SDLMain", "Engine initialization failed");
        return -1;
    }

    // 在 Android 上，我们直接加载打包在 APK 中的插件
    // 逻辑类似于 LauncherMain.cpp
    void* handle = dlopen("libPathTracing3D.so", RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        LOG_FATAL("SDLMain", "Failed to load libPathTracing3D.so: {0}", dlerror());
        return -1;
    }

    // 获取项目名称
    using GetNameFn = const char* (*)();
    auto fnGetName = (GetNameFn)dlsym(handle, "GetProjectName");
    if (fnGetName) {
        const char* name = fnGetName();
        if (name) {
            engine.SetProjectName(name);
            LOG_INFO("SDLMain", "Project Name identified: {0}", name);
        }
    }

    auto createFunc = (Prisma::Application* (*)())dlsym(handle, "CreateApplication");
    if (!createFunc) {
        LOG_FATAL("SDLMain", "Failed to find CreateApplication in libPathTracing3D.so");
        dlclose(handle);
        return -1;
    }

    auto* app = createFunc();
    int result = engine.Run(std::unique_ptr<Prisma::Application>(app));
    
    engine.Shutdown();
    dlclose(handle);

    return result;
}

#endif
