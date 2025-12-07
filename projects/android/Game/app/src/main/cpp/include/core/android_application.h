#pragma once
#ifndef ANDROIDAPPLICATION_H
#define ANDROIDAPPLICATION_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <chrono>
#include <string>
#include <core/application.h>
#include "engine/render/GameWindow.h"
#include "engine/render/GameRenderer.h"
#include "engine/render/FPSRenderer.h"

// SDL3 in Android needs this macro
#define SDL_MAIN_NEEDED

// Forward-declare FPSRenderer to break the circular dependency
class FPSRenderer;

class AndroidApplication : Application{
public:
    AndroidApplication();
    ~AndroidApplication() override;
    // Main loop
    bool running;
    int run();
    TTF_Font* defaultFont;
    GameWindow* gameWindow{};
    GameRenderer* gameRenderer{};
    FPSRenderer* fpsRenderer; // This will now compile correctly

    std::chrono::high_resolution_clock::time_point lastTime;
    int frameCount{};
    float fps{};
    float maxFramerate{};
private:
    bool initialize();
    void handleEvents();
    void update(float deltaTime);
    void render();
    void shutdown();
};

#endif // ANDROIDAPPLICATION_H
