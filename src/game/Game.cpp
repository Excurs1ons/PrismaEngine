// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "Game.h"

#include "IApplication.h"
#include "pch.h"


bool Game::Initialize() {
    return true;
}
int Game::Run() {
    return 0;
}
void Game::Shutdown() {

}

// Platform->Engine->Application->Game|Editor