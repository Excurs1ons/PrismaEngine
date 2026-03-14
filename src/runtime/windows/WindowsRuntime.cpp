#include <iostream>
#include <memory>
#include "../../engine/Engine.h"
#include "../../engine/DynamicLoader.h"
#include "../../engine/CommandLineParser.h"
#include "../../engine/Application.h"

/**
 * @brief Prisma Launcher
 */
int main(int argc, char* argv[]) {
    // 0. 日志系统先行 (Eyes open first)
    Prisma::LogConfig logConfig;
    logConfig.target = Prisma::LogTarget::Both;
    Prisma::Logger::Get().Initialize(logConfig);

    // 1. Awake the creator with specific configuration
    Prisma::EngineSpecification spec;
    spec.Name = "Prisma Runtime";
    spec.MinLogLevel = Prisma::LogLevel::Info;

    auto engine = Prisma::Engine::Get();
    if (engine->Initialize(spec) != 0) {
        return -1;
    }

    // 2. Load application plugin
    Prisma::DynamicLoader loader;
    try {
        loader.Load("PrismaEditor.dll"); 
    } catch (const std::exception& e) {
        std::cerr << "Fatal: Failed to load application DLL: " << e.what() << std::endl;
        return -1;
    }

    // 3. Get factory function
    using CreateAppFn = Prisma::Application* (*)();
    auto create = (CreateAppFn)loader.GetFunction("CreateApplication");
    
    if (create) {
        std::unique_ptr<Prisma::Application> app(create());
        
        // 4. Engine takes full ownership and responsibility
        engine->Run(std::move(app));
    }

    // 5. Shutdown
    engine->Shutdown();

    return 0;
}
