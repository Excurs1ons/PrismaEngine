/**
 * Pac-Man Game
 * Built with PrismaEngine
 */

#include "PacManGame.h"
#include "core/Timestep.h"
#include <iostream>
#include <memory>

namespace PacMan {

// 全局游戏实例
PacManGame* g_PacManGame = nullptr;

} // namespace PacMan

int main(int argc, char* argv[]) {
    using namespace PacMan;

    // 创建游戏实例
    g_PacManGame = new PacManGame();
    if (!g_PacManGame) {
        std::cerr << "Failed to create Pac-Man game instance!" << std::endl;
        return -1;
    }

    // 初始化游戏
    try {
        g_PacManGame->Initialize();
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Pac-Man game: " << e.what() << std::endl;
        delete g_PacManGame;
        return -1;
    }

    // 运行游戏
    g_PacManGame->Run();

    // 关闭游戏
    g_PacManGame->Shutdown();
    delete g_PacManGame;
    g_PacManGame = nullptr;

    return 0;
}
