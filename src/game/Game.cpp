// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "Game.h"

#include "SceneManager.h"
#include "TriangleExample.h"
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
    return Application::Run();
}
void Game::Shutdown() {
    Application::Shutdown();
}

// Platform->Engine->Application->Game|Editor

// 添加导出函数实现
// 动态库模式使用 Initialize/Run/Shutdown，静态库模式使用 Game_ 前缀
extern "C" {
#ifdef GAME_BUILD_SHARED
    // 动态库模式 - 导出标准函数名
    #if defined(_WIN32) || defined(_WIN64)
        __declspec(dllexport)
    #else
        __attribute__((visibility("default")))
    #endif
    bool Initialize() {
        return Game::GetInstance().Initialize();
    }

    #if defined(_WIN32) || defined(_WIN64)
        __declspec(dllexport)
    #else
        __attribute__((visibility("default")))
    #endif
    int Run() {
        return Game::GetInstance().Run();
    }

    #if defined(_WIN32) || defined(_WIN64)
        __declspec(dllexport)
    #else
        __attribute__((visibility("default")))
    #endif
    void Shutdown() {
        Game::GetInstance().Shutdown();
    }
#else
    // 静态库模式 - 使用 Game_ 前缀避免符号冲突
    bool Game_Initialize() {
        return Game::GetInstance().Initialize();
    }

    int Game_Run() {
        return Game::GetInstance().Run();
    }

    void Game_Shutdown() {
        Game::GetInstance().Shutdown();
    }
#endif
}