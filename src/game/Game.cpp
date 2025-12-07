// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "Game.h"

#include "IApplication.h"
#include "pch.h"
#include "Common.h"


bool Game::Initialize() {
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