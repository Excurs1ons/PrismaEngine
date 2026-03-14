#include <memory>
#include "../engine/Engine.h"
#include "../engine/Application.h"
#include "../engine/Logger.h"

/**
 * @brief Prisma Editor Launcher
 */
int main(int argc, char* argv[]) {
    // 0. Eyes open first
    Prisma::LogConfig logConfig;
    logConfig.target = Prisma::LogTarget::Both;
    Prisma::Logger::Get().Initialize(logConfig);

    // 1. Initialize engine
    auto engine = Prisma::Engine::Get();
    if (engine->Initialize() != 0) {
        return -1;
    }

    // 2. Create application
    std::unique_ptr<Prisma::Application> app(Prisma::CreateApplication());

    if (app) {
        // 3. Engine drives the lifecycle
        engine->Run(std::move(app));
    }

    // 4. Shutdown
    engine->Shutdown();

    return 0;
}
