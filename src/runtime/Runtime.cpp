#include <iostream>
#include <memory>
#include <vector>
#include "../engine/Engine.h"
#include "../engine/DynamicLoader.h"
#include "../engine/CommandLineParser.h"
#include "../engine/Application.h"
#include "../engine/Logger.h"

/**
 * @brief Prisma Launcher (Cross-Platform)
 */
int main(int argc, char* argv[]) {
    // 0. Eyes open first
    Prisma::LogConfig logConfig;
    logConfig.target = Prisma::LogTarget::Both;
    Prisma::Logger::Get().Initialize(logConfig);

    // 1. Create Engine (Architecture fix)
    Prisma::EngineSpecification spec;
    spec.Name = "Prisma Runtime";
    spec.MinLogLevel = Prisma::LogLevel::Info;
    Prisma::Engine engine(spec);

    // 2. Initialize engine
    if (engine.Initialize() != 0) {
        return -1;
    }

    // 3. Load application plugin
    Prisma::DynamicLoader loader;
    bool loaded = false;
    
    // 跨平台尝试加载动态库
    std::vector<std::string> libNames = {
#if defined(_WIN32)
        "Editor.dll",
        "PrismaEditor.dll",
#elif defined(__APPLE__)
        "libEditor.dylib",
        "libPrismaEditor.dylib",
#else
        "libEditor.so",
        "libPrismaEditor.so",
#endif
    };

    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && argv[i][0] != '\0') {
            libNames.insert(libNames.begin(), argv[i]);
        }
    }

    for (const auto& name : libNames) {
        try {
            if (loader.Load(name)) {
                loaded = true;
                break;
            }
        } catch (const std::exception& e) {
            // 继续尝试下一个
            std::cout << "Trying next library name due to: " << e.what() << std::endl;
        }
    }

    if (!loaded) {
        std::cerr << "Fatal: Failed to load application dynamic library." << std::endl;
        return -1;
    }

    // 4. Get factory function
    using CreateAppFn = Prisma::Application* (*)();
    auto create = (CreateAppFn)loader.GetFunction<CreateAppFn>("CreateApplication");
    
    if (create) {
        std::unique_ptr<Prisma::Application> app(create());
        
        // 5. Engine takes full ownership and responsibility
        engine.Run(std::move(app));
    }

    // 6. Shutdown
    engine.Shutdown();

    return 0;
}
