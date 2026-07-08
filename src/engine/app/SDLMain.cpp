#include "Engine.h"
#include "Application.h"
#include "Logger.h"
#include "ProjectConfig.h"
#include "platform/Platform.h"

#ifdef __ANDROID__
#include <dlfcn.h>
#include <glaze/glaze.hpp>

extern "C" int SDL_main(int argc, char* argv[]) {
    Prisma::EngineSpecification spec;
    spec.Name = "PrismaEngine Android";
    spec.RefreshAssetDatabaseOnStartup = false;

    Prisma::Engine engine(spec);
    if (engine.Initialize() != 0) {
        LOG_FATAL("SDLMain", "Engine initialization failed");
        return -1;
    }

    // 先从项目设置读取 plugin 库名，再 dlopen（而非硬编码库名）
    // 项目设置由 export 脚本生成为标准化 project.jsonc，不依赖项目名
    std::string coreLibrary;
    std::string projectName;
    {
        // Android: Platform::ReadBinaryFile 经 SDL_LoadFile / AAssetManager 读 APK assets
        auto data = Prisma::Platform::ReadBinaryFile("assets/project.jsonc");
        if (data.empty())
            data = Prisma::Platform::ReadBinaryFile("project.jsonc");

        if (!data.empty()) {
            std::string buf(data.begin(), data.end());
            // 跳过 UTF-8 BOM
            if (buf.size() >= 3) {
                const auto* u = reinterpret_cast<const unsigned char*>(buf.data());
                if (u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF)
                    buf.erase(0, 3);
            }

            Prisma::ProjectConfig config;
            auto err = glz::read<glz::opts{ .comments = true, .error_on_unknown_keys = false }>(config, buf);
            if (!err) {
                coreLibrary  = config.coreLibrary;
                projectName  = config.name;
            } else {
                LOG_WARNING("SDLMain", "project.jsonc 解析失败: {}", glz::format_error(err, buf));
            }
        } else {
            LOG_WARNING("SDLMain", "未找到 project.jsonc");
        }
    }

    if (coreLibrary.empty()) {
        LOG_FATAL("SDLMain", "项目设置中未指定 coreLibrary，无法加载 plugin");
        return -1;
    }

    // 构造库名并加载
    std::string libName = "lib" + coreLibrary + ".so";
    LOG_INFO("SDLMain", "Loading plugin: {0}", libName);
    void* handle = dlopen(libName.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        LOG_FATAL("SDLMain", "Failed to load {0}: {1}", libName, dlerror());
        return -1;
    }

    if (!projectName.empty()) {
        engine.SetProjectName(projectName);
        LOG_INFO("SDLMain", "Project Name from settings: {0}", projectName);
    }

    auto createFunc = (Prisma::Application* (*)())dlsym(handle, "CreateApplication");
    if (!createFunc) {
        LOG_FATAL("SDLMain", "Failed to find CreateApplication in {0}", libName);
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
