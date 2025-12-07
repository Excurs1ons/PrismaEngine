#include "core/android_application.h"

AndroidApplication::AndroidApplication() {
    gameWindow = nullptr;
    gameRenderer = nullptr;
    defaultFont = nullptr;
    running =false;
    fpsRenderer = nullptr;
}

AndroidApplication::~AndroidApplication() {
    gameWindow = nullptr;
    gameRenderer = nullptr;

    if(defaultFont)
        TTF_CloseFont(defaultFont);
    defaultFont= nullptr;
}

int AndroidApplication::run() {
    if(!initialize()){
        SDL_Log("Failed to initialize!");
        return 1;
    }

    gameWindow = new GameWindow();
    if(!gameWindow->initialize()){
        return 1;
    }

    gameRenderer = new GameRenderer();
    gameRenderer->gameWindow = gameWindow;
    if(!gameRenderer->initialize()){
        return 1;
    }
    running = true;

    const int defaultFrameRateLimitation = 144;


    // 获取安全区域
    SDL_Rect safeArea;


    // 创建文本表面
    //SDL_Color color = {255, 255, 255, 255}; // 白色
    //SDL_Surface* textSurface = TTF_RenderText_Solid(defaultFont, "你好世界", 3*4,color);


    // 转换为纹理
    //SDL_Texture* textTexture = SDL_CreateTextureFromSurface(gameRenderer->renderer, textSurface);
    //SDL_DestroySurface(textSurface);
    // 渲染文本
    // SDL_FRect destRect = {100, 100, (float)textSurface->w, (float)textSurface->h};

    //fpsRenderer = new FPSRenderer(gameRenderer->renderer,defaultFont,64);
    // FPS计算相关变量
    using namespace std::chrono;
    //lastTime = std::chrono::high_resolution_clock::now();
    //frameCount = 0;
    //fps = 0.0f;


    //maxFramerate = 0.0;
    while (running) {
        // 处理事件
        handleEvents();

        // 计算FPS
        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = duration_cast<milliseconds>(currentTime - lastTime).count();

        if (deltaTime >= 1000) { // 每秒更新一次FPS值
            fps = (float)frameCount * 1000.0f / (float)deltaTime;
            frameCount = 0;
            lastTime = currentTime;
        }
        gameRenderer->clear();
        if (SDL_GetWindowSafeArea(gameWindow->window, &safeArea)) {
            // 在安全区域内绘制一个绿色的矩形作为参考
            SDL_SetRenderDrawColor(gameRenderer->renderer, 0, 255, 0, 128); // 半透明绿色
            SDL_FRect safeAreaRect = { (float)safeArea.x, (float)safeArea.y, (float)safeArea.w, (float)safeArea.h };
            SDL_RenderFillRect(gameRenderer->renderer, &safeAreaRect);
        } else {
            SDL_Log("Could not get window safe area: %s", SDL_GetError());
        }

        // auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);
        // 显示FPS文本
        // char fpsText[64];
        // snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", fps);

        // SDL_Log("FPS: %.1f", fps);
        // 更新 FPS 显示
        // fpsRenderer->updateFPS(fps);
        // SDL_RenderTexture(renderer, textTexture, nullptr, &destRect);
        // 渲染 FPS
        //fpsRenderer->render(safeArea.x, safeArea.y);
        gameRenderer->tick();
        gameRenderer->render();

        //SDL_Delay(1); // ~60 FPS
    }

    // 清理
    shutdown();

    return 0;
}

bool AndroidApplication::initialize() {
    // 在 SDL_Init 之前设置屏幕方向 Hint
    // 这告诉 SDL 我们偏好横屏模式
    if (!SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight")) {
        SDL_Log("Warning: Failed to set orientation hint: %s", SDL_GetError());
        return false;
    }
    else{
        SDL_Log("Set Orientation Successfully");
    }
    // 新增：强制 SDL 使用 vulkan 渲染驱动
    if (!SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan")) {
        SDL_Log("Warning: Failed to set render driver hint: %s", SDL_GetError());
        return false;
    }
    else{
        SDL_Log("Set Render Driver Successfully");
    }
    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) { // 注意：SDL_Init 成功时返回 0
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    else{
        SDL_Log("SDL_Init Successfully");
    }
    // 初始化 TTF 字体
    if (!TTF_Init()) {
        SDL_Log("TTF_Init failed: %s", SDL_GetError());
        return false;
    }
    else {
        SDL_Log("TTF_Init Successfully");
    }

    // 加载字体
    defaultFont = TTF_OpenFont("fonts/MiSans-Regular.ttf", 284); // 24 是字体大小
    if (!defaultFont) {
        SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
        return false;
    }
    // 启用 SDF 渲染
    if (!TTF_SetFontSDF(defaultFont, true)) {
        SDL_Log("SDF rendering not supported: %s", SDL_GetError());
    } else {
        SDL_Log("SDF rendering enabled");
    }
    return true;
}

void AndroidApplication::shutdown() {
    SDL_Quit();
}
void AndroidApplication::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        if(event.type==SDL_EVENT_KEY_DOWN){
            SDL_KeyboardEvent& keyEvent = event.key;
            SDL_Keycode keyCode = keyEvent.key;
            SDL_Log("Key pressed: %d",keyCode);
        }
        if (event.type==SDL_EVENT_FINGER_DOWN)
        {
            SDL_TouchFingerEvent& fingerEvent = event.tfinger;
            // 触摸坐标是标准化的(0.0-1.0)，需要转换为屏幕坐标
            float touchX = fingerEvent.x;  // 范围 0.0 - 1.0
            float touchY = fingerEvent.y;  // 范围 0.0 - 1.0

            // 获取窗口尺寸以转换为像素坐标
            int windowWidth, windowHeight;
            SDL_GetWindowSize(gameWindow->window, &windowWidth, &windowHeight);

            int pixelX = static_cast<int>(touchX * (float)windowWidth);
            int pixelY = static_cast<int>(touchY * (float)windowHeight);

            SDL_Log("触摸位置: (%.2f, %.2f) => (%d, %d)", touchX, touchY, pixelX, pixelY);
        }
    }
}

void AndroidApplication::update(float deltaTime) {

}

void AndroidApplication::render() {

}
