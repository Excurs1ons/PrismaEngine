#include "Renderer.h"
#include "RendererOpenGL.h"
#include "AndroidOut.h"
#include "RendererVulkan.h"
#include "AndroidInputBackend.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <chrono>

Renderer::Renderer(android_app *pApp) : app_(pApp) {
    // Decide which backend to use. For now, default to OpenGL.
    // You could check system properties or app configuration here.
    bool useVulkan = true;

    if (useVulkan) {
        impl_ = std::make_unique<RendererVulkan>(pApp);
    } else {
        impl_ = std::make_unique<RendererOpenGL>(pApp);
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