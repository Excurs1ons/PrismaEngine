#include <memory>
#include "Editor.h"
#include "../engine/Logger.h"

/**
 * @brief Prisma Editor Launcher
 */
int main(int argc, char* argv[]) {
    // 0. Eyes open first
    Prisma::LogConfig logConfig;
    logConfig.target = Prisma::LogTarget::Both;
    Prisma::Logger::Get().Initialize(logConfig);

    LOG_INFO("Editor", "Starting Prisma Editor...");

    // 1. Create Editor Application (マスター Master)
    // 架构调整：编辑器现在是驱动者，它持有并管理引擎。
    auto editor = std::make_unique<Prisma::Editor>();

    // 2. Initialize Editor
    if (editor->Initialize() != 0) {
        LOG_FATAL("Editor", "Failed to initialize editor!");
        return -1;
    }

    // 3. Main Loop
    editor->Run();

    // 4. Shutdown
    editor->Shutdown();

    return 0;
}
