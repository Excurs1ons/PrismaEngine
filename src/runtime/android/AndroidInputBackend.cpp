#include "AndroidInputBackend.h"
#include "AndroidOut.h"
#include <algorithm>

namespace PrismaEngine {
namespace Input {

AndroidInputBackend& AndroidInputBackend::GetInstance() {
    static AndroidInputBackend instance;
    return instance;
}

void AndroidInputBackend::Initialize(android_app* app) {
    if (app && app->window) {
        // 获取当前窗口尺寸
        int32_t width = ANativeWindow_getWidth(app->window);
        int32_t height = ANativeWindow_getHeight(app->window);
        SetScreenSize(width, height);
        aout << "AndroidInputBackend initialized with screen: " << width << "x" << height << std::endl;
    }
}

void AndroidInputBackend::Update() {
    // 保存上一帧的触摸点
    previousTouches_ = activeTouches_;
    previousTouchCount_ = static_cast<int>(activeTouches_.size());

    // 更新触摸点阶段
    UpdateTouchPhases();

    // 清理已结束的触摸点（延迟清理，让组件有机会读取 Ended/Cancelled 状态）
    for (auto it = activeTouches_.begin(); it != activeTouches_.end();) {
        // 检查是否在上一帧就已经是 Ended/Cancelled
        auto prevIt = previousTouches_.find(it->first);
        bool wasEnded = (prevIt != previousTouches_.end() &&
                        (prevIt->second.phase == TouchPhase::Ended ||
                         prevIt->second.phase == TouchPhase::Cancelled));

        if (wasEnded) {
            // 上一帧已经是结束状态，现在可以删除了
            it = activeTouches_.erase(it);
        } else {
            ++it;
        }
    }
}

const Touch* AndroidInputBackend::GetTouch(int index) const {
    if (index < 0 || index >= static_cast<int>(activeTouches_.size())) {
        return nullptr;
    }

    auto it = activeTouches_.begin();
    std::advance(it, index);
    return &(it->second);
}

const Touch* AndroidInputBackend::GetTouchById(int fingerId) const {
    auto it = activeTouches_.find(fingerId);
    if (it != activeTouches_.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool AndroidInputBackend::IsTouching(int fingerId) const {
    return activeTouches_.find(fingerId) != activeTouches_.end();
}

void AndroidInputBackend::OnTouchBegan(int fingerId, float x, float y) {
    Touch touch;
    touch.fingerId = fingerId;
    touch.positionX = x;
    touch.positionY = y;
    touch.deltaX = 0.0f;
    touch.deltaY = 0.0f;
    touch.pressure = 1.0f;  // Android 默认压力
    touch.phase = TouchPhase::Began;

    activeTouches_[fingerId] = touch;

    aout << "Touch began: fingerId=" << fingerId << " pos=(" << x << ", " << y << ")" << std::endl;
}

void AndroidInputBackend::OnTouchMoved(int fingerId, float x, float y) {
    auto it = activeTouches_.find(fingerId);
    if (it != activeTouches_.end()) {
        // 计算位置变化
        it->second.deltaX = x - it->second.positionX;
        it->second.deltaY = y - it->second.positionY;
        it->second.positionX = x;
        it->second.positionY = y;
        it->second.phase = TouchPhase::Moved;

        // aout << "Touch moved: fingerId=" << fingerId << " pos=(" << x << ", " << y << ") delta=("
        //      << it->second.deltaX << ", " << it->second.deltaY << ")" << std::endl;
    }
}

void AndroidInputBackend::OnTouchEnded(int fingerId, float x, float y) {
    auto it = activeTouches_.find(fingerId);
    if (it != activeTouches_.end()) {
        it->second.positionX = x;
        it->second.positionY = y;
        it->second.phase = TouchPhase::Ended;

        // aout << "Touch ended: fingerId=" << fingerId << " pos=(" << x << ", " << y << ")" << std::endl;
    }
}

void AndroidInputBackend::OnTouchCancelled(int fingerId) {
    auto it = activeTouches_.find(fingerId);
    if (it != activeTouches_.end()) {
        it->second.phase = TouchPhase::Cancelled;

        aout << "Touch cancelled: fingerId=" << fingerId << std::endl;
    }
}

Vector2 AndroidInputBackend::GetMousePosition() const {
    if (!activeTouches_.empty()) {
        const auto& touch = activeTouches_.begin()->second;
        return Vector2(touch.positionX, touch.positionY);
    }
    return Vector2(0, 0);
}

bool AndroidInputBackend::GetMouseButton(int button) const {
    // 0 = 左键 / 单指触摸
    if (button == 0) {
        return !activeTouches_.empty();
    }
    return false;
}

// === IInputBackend 接口实现 ===

bool AndroidInputBackend::GetKeyDown(KeyCode key) {
    // Android 外接键盘支持（如果需要）
    auto it = keyStates_.find(static_cast<int>(key));
    return it != keyStates_.end() && it->second;
}

bool AndroidInputBackend::GetKeyUp(KeyCode key) {
    auto it = keyStates_.find(static_cast<int>(key));
    return it == keyStates_.end() || !it->second;
}

bool AndroidInputBackend::GetPointerDown(MouseButton button) {
    if (button == MouseButton::Left) {
        return !activeTouches_.empty();
    }
    return false;
}

bool AndroidInputBackend::GetPointerUp(MouseButton button) {
    if (button == MouseButton::Left) {
        return activeTouches_.empty();
    }
    return true;
}

void AndroidInputBackend::UpdateTouchPhases() {
    // 更新触摸点的阶段状态
    for (auto& pair : activeTouches_) {
        Touch& touch = pair.second;

        // 查找上一帧的状态
        auto prevIt = previousTouches_.find(touch.fingerId);

        if (touch.phase == TouchPhase::Began) {
            // Began 在下一帧变为 Moved 或 Stationary
            touch.phase = TouchPhase::Stationary;
        } else if (touch.phase == TouchPhase::Moved) {
            // 如果位置没变，变为 Stationary
            if (std::abs(touch.deltaX) < 0.01f && std::abs(touch.deltaY) < 0.01f) {
                touch.phase = TouchPhase::Stationary;
            }
        }

        // 如果是新的触摸点（上一帧不存在），保持 Began
        if (prevIt == previousTouches_.end()) {
            touch.phase = TouchPhase::Began;
        }
    }
}

// === 全局输入函数实现 ===

bool Input_GetMouseButtonDown(int button) {
    static auto& backend = AndroidInputBackend::GetInstance();
    static int previousCount = 0;
    int currentCount = backend.GetTouchCount();

    // 检测新增加的触摸点
    bool result = currentCount > previousCount && button == 0;
    previousCount = currentCount;
    return result;
}

bool Input_GetMouseButtonUp(int button) {
    static auto& backend = AndroidInputBackend::GetInstance();
    static int previousCount = 0;
    int currentCount = backend.GetTouchCount();

    // 检测减少的触摸点
    bool result = currentCount < previousCount && button == 0;
    previousCount = currentCount;
    return result;
}

} // namespace Input
} // namespace PrismaEngine
