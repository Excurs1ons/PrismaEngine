#include "app/Application.h"
#include "app/Engine.h"
#include "app/CommandLineParser.h"
#include "logger/Logger.h"
#include <memory>

extern "C" Prisma::Application* CreateApplication();

/**
 * @brief Prisma Editor Launcher
 */
int main(int argc, char* argv[]) {
    // 0. Parse command line
    CommandLineParser::Get().Parse(argc, argv);

    // 1. Eyes open first
    Prisma::LogConfig logConfig;
    logConfig.target = Prisma::LogTarget::Both;
    Prisma::Logger::Get().Initialize(logConfig);

    if (argc > 1) {
        std::string arguments;
        for (int i = 1; i < argc; ++i) {
            if (argv[i] == nullptr || argv[i][0] == '\0') {
                continue;
            }
            if (!arguments.empty()) {
                arguments += ' ';
            }
            arguments += argv[i];
        }

        if (!arguments.empty()) {
            LOG_INFO("Editor", "Startup arguments: {}", arguments);
        }
    }

    // 2. Create Engine (Architecture fix: explicit instantiation on stack)
    Prisma::EngineSpecification spec;
    spec.Name                          = "Prisma Editor";
    spec.RefreshAssetDatabaseOnStartup = true;
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
