#include "Renderer.h"
#include "RendererOpenGL.h"
#include "AndroidOut.h"
#include "RendererVulkan.h"
#include "AndroidInputBackend.h"
#include <game-activity/GameActivity.h>
#include <android/asset_manager.h>
#include <chrono>

Renderer::Renderer(ANativeWindow *window) : window_(window) {
    // Decide which backend to use. For now, default to Vulkan.
    // You could check system properties or app configuration here.
    bool useVulkan = true;

    if (useVulkan) {
        impl_ = std::make_unique<RendererVulkan>(window_);
    } else {
        impl_ = std::make_unique<RendererOpenGL>(window_);
    }
}

Renderer::~Renderer() {
    // impl_ will be automatically deleted
}

void Renderer::render() {
    if (impl_) {
        impl_->render();
    }
}

void Renderer::onConfigChanged() {
    if (impl_) {
        impl_->onConfigChanged();
    }
}

void Renderer::handleInput() {
    // 首先让具体实现处理输入（包括 UI 交互）
    if (impl_) {
        impl_->handleInput();
    }
}

void Renderer::onNativeWindowCreated(ANativeWindow *window) {
    window_ = window;
    if (impl_) {
        impl_->onNativeWindowCreated(window);
    }
}

void Renderer::onNativeWindowChanged(ANativeWindow *window) {
    window_ = window;
    if (impl_) {
        impl_->onNativeWindowChanged(window);
    }
}

void Renderer::onNativeWindowDestroyed() {
    if (impl_) {
        impl_->onNativeWindowDestroyed();
    }
    window_ = nullptr;
}

void Renderer::onResume() {
    if (impl_) {
        impl_->onResume();
    }
}

void Renderer::onPause() {
    if (impl_) {
        impl_->onPause();
    }
}

void Renderer::onKeyDown(int keyCode) {
    if (impl_) {
        impl_->onKeyDown(keyCode);
    }
}

void Renderer::onKeyUp(int keyCode) {
    if (impl_) {
        impl_->onKeyUp(keyCode);
    }
}

void Renderer::onTouchEvent(int action, float x, float y) {
    if (impl_) {
        impl_->onTouchEvent(action, x, y);
    }
}

void Renderer::setAssetManager(void *assetManager) {
    if (impl_) {
        impl_->setAssetManager(static_cast<AAssetManager*>(assetManager));
    }
}

void Renderer::setContentRect(int32_t left, int32_t top, int32_t right, int32_t bottom) {
    if (impl_) {
        impl_->setContentRect(left, top, right, bottom);
    }
}
