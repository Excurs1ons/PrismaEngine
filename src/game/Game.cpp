// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "Game.h"

#include "../engine/SceneManager.h"
#include "../engine/TriangleExample.h"
#include "Common.h"
#include "IApplication.h"
#include "pch.h"

bool Game::Initialize() {
    // 调用基类初始化
    if (!Application::Initialize()) {
        return false;
    }

    // SceneManager 的 Initialize() 方法已经创建了包含索引缓冲区测试的示例场景
    LOG_INFO("Game", "游戏初始化完成 - 使用默认索引缓冲区测试场景");
    return true;
}
int Game::Run() {
    return 0;
}
void Game::Shutdown() {

}

// Platform->Engine->Application->Game|Editor

// 添加导出函数实现
extern "C" {
    __declspec(dllexport) bool Initialize() {
        return Game::GetInstance().Initialize();
    }

    __declspec(dllexport) int Run() {
        return Game::GetInstance().Run();
    }

    __declspec(dllexport) void Shutdown() {
        Game::GetInstance().Shutdown();
    }
}