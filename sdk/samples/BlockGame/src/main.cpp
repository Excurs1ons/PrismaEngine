#include <PrismaEngine/PrismaEngine.h>
#include <PrismaEngine/input/InputManager.h>
#include "Game.h"
#include <iostream>

using namespace PrismaEngine;

int main(int argc, char* argv[]) {
    // 初始化日志系统
    Logger::Init("BlockGame");

    LOG_INFO("Main", "BlockGame 示例项目启动");

    try {
        // 创建游戏实例
        BlockGame::Game game;

        // 初始化游戏
        if (!game.Initialize()) {
            LOG_ERROR("Main", "游戏初始化失败");
            return -1;
        }

        // 运行游戏主循环
        int exitCode = game.Run();

        // 关闭游戏
        game.Shutdown();

        LOG_INFO("Main", "游戏正常退出，退出码: {}", exitCode);
        return exitCode;

    } catch (const std::exception& e) {
        LOG_ERROR("Main", "未捕获的异常: {}", e.what());
        return -1;
    }
}
