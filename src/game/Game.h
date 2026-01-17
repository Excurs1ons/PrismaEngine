//
// Created by JasonGu on 25-12-7.
//
#pragma once
#include "IApplication.h"
#include "platform/Application.h"

#ifndef GAME_H
#define GAME_H
class Game : public PrismaEngine::Application {
public:
    bool Initialize() override;
    int Run() override;
    void Shutdown() override;
};


// 导出函数声明 - 支持动态库和静态库两种模式
extern "C" {
#ifdef GAME_BUILD_SHARED
    // 动态库模式 - 导出标准函数名
    #if defined(_WIN32) || defined(_WIN64)
        __declspec(dllexport)
    #else
        __attribute__((visibility("default")))
    #endif
    bool Initialize();

    #if defined(_WIN32) || defined(_WIN64)
        __declspec(dllexport)
    #else
        __attribute__((visibility("default")))
    #endif
    int Run();

    #if defined(_WIN32) || defined(_WIN64)
        __declspec(dllexport)
    #else
        __attribute__((visibility("default")))
    #endif
    void Shutdown();
#else
    // 静态库模式 - 使用 Game_ 前缀
    bool Game_Initialize();
    int Game_Run();
    void Game_Shutdown();
#endif
}
#endif //GAME_H
