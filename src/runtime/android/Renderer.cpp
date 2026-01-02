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
    // 首先更新输入系统（处理上一帧的状态）
    auto& inputBackend = PrismaEngine::Input::AndroidInputBackend::GetInstance();
    inputBackend.Update();

    // handle all queued inputs
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        // no inputs yet.
        return;
    }

    static int eventCount = 0;
    static auto lastLogTime = std::chrono::steady_clock::now();

    // handle motion events (motionEventsCounts can be 0).
    auto motionEventCount = inputBuffer->motionEventsCount;
    if (motionEventCount > 0) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime).count();
        if (duration > 1000) {  // 每秒最多输出一次
            aout << "handleInput: motionEventsCount=" << motionEventCount << std::endl;
            lastLogTime = now;
        }
    }

    for (auto i = 0; i < motionEventCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        // Find the pointer index, mask and bitshift to turn it into a readable value.
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        // get the x and y position of this event if it is not ACTION_MOVE.
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        // determine the action type and process the event accordingly.
        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                aout << "Renderer: DOWN event, fingerId=" << pointer.id << " pos=(" << x << ", " << y << ")" << std::endl;
                inputBackend.OnTouchBegan(pointer.id, x, y);
                break;

            case AMOTION_EVENT_ACTION_CANCEL:
                aout << "Renderer: CANCEL event, fingerId=" << pointer.id << std::endl;
                inputBackend.OnTouchCancelled(pointer.id);
                break;

            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                aout << "Renderer: UP event, fingerId=" << pointer.id << " pos=(" << x << ", " << y << ")" << std::endl;
                inputBackend.OnTouchEnded(pointer.id, x, y);
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                // There is no pointer index for ACTION_MOVE, only a snapshot of
                // all active pointers; app needs to cache previous active pointers
                // to figure out which ones are actually moved.
                for (auto index = 0; index < motionEvent.pointerCount; index++) {
                    pointer = motionEvent.pointers[index];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                    inputBackend.OnTouchMoved(pointer.id, x, y);
                }
                break;
            default:
                break;
        }
    }
    // clear the motion input count in this buffer for main thread to re-use.
    android_app_clear_motion_events(inputBuffer);

    // handle input key events.
    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        aout << "Key: " << keyEvent.keyCode <<" ";
        switch (keyEvent.action) {
            case AKEY_EVENT_ACTION_DOWN:
                aout << "Key Down";
                break;
            case AKEY_EVENT_ACTION_UP:
                aout << "Key Up";
                break;
            case AKEY_EVENT_ACTION_MULTIPLE:
                // Deprecated since Android API level 29.
                aout << "Multiple Key Actions";
                break;
            default:
                aout << "Unknown KeyEvent Action: " << keyEvent.action;
        }
        aout << std::endl;
    }
    // clear the key input count too.
    android_app_clear_key_events(inputBuffer);
}