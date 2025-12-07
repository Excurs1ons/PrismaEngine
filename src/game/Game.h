//
// Created by JasonGu on 25-12-7.
//
#pragma once
#include "IApplication.h"
#include "platform/Application.h"

#ifndef GAME_H
#define GAME_H
class Game : public Engine::Application {
public:
    bool Initialize() override;
    int Run() override;
    void Shutdown() override;
};
#endif //GAME_H
