#include <memory>
#include "../engine/Engine.h"
#include "../engine/Application.h"
#include "../engine/Logger.h"

extern "C" Prisma::Application* CreateApplication();

/**
 * @brief Prisma Editor Launcher
 */
int main(int argc, char* argv[]) {
    // 0. Eyes open first
    Prisma::LogConfig logConfig;
    logConfig.target = Prisma::LogTarget::Both;
    Prisma::Logger::Get().Initialize(logConfig);

    // 1. Create Engine (Architecture fix: explicit instantiation on stack)
    Prisma::EngineSpecification spec;
    spec.Name = "Prisma Editor";
    Prisma::Engine engine(spec);

    // 2. Initialize engine
    if (engine.Initialize() != 0) {
        return -1;
    }

    // 3. Create application
    std::unique_ptr<Prisma::Application> app(CreateApplication());

    if (app) {
        // 4. Engine drives the lifecycle
        engine.Run(std::move(app));
    }

    // 5. Shutdown
    engine.Shutdown();

    return 0;
}
