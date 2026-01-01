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


// 添加导出函数实现
extern "C" {
    __declspec(dllexport) bool Initialize();
    __declspec(dllexport) int Run();
    __declspec(dllexport) void Shutdown();
}
#endif //GAME_H
