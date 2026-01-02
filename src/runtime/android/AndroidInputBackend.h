#pragma once
#include "math/MathTypes.h"
#include "InputBackend.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <unordered_map>
#include <memory>

namespace PrismaEngine {
namespace Input {

/**
 * 触摸阶段（Unity TouchPhase 风格）
 */
enum class TouchPhase {
    Began = 0,      // 手指刚触碰到屏幕
    Moved = 1,      // 手指在屏幕上移动
    Stationary = 2, // 手指静止在屏幕上
    Ended = 3,      // 手指离开屏幕
    Cancelled = 4   // 系统取消触摸（如电话）
};

/**
 * Android 触控点数据（仿 Unity Touch 结构）
 */
struct Touch {
    int fingerId;           // 手指 ID
    float positionX;        // 当前 X 位置（屏幕坐标）
    float positionY;        // 当前 Y 位置（屏幕坐标）
    float deltaX;           // X 位置变化量
    float deltaY;           // Y 位置变化量
    float pressure;         // 压力值（0-1）
    TouchPhase phase;       // 触摸阶段

    // Unity 风格的便利方法
    Vector2 Position() const { return Vector2(positionX, positionY); }
    Vector2 DeltaPosition() const { return Vector2(deltaX, deltaY); }
    Vector2 RawPosition() const { return Position(); }  // Android 原始坐标

    // 便利方法：检查触摸阶段
    bool IsBegan() const { return phase == TouchPhase::Began; }
    bool IsMoved() const { return phase == TouchPhase::Moved; }
    bool IsStationary() const { return phase == TouchPhase::Stationary; }
    bool IsEnded() const { return phase == TouchPhase::Ended; }
    bool IsCancelled() const { return phase == TouchPhase::Cancelled; }
};

/**
 * Android 输入后端
 * 处理 Android 触控事件并转换为统一的输入格式
 */
class AndroidInputBackend : public IInputBackend {
public:
    static AndroidInputBackend& GetInstance();

    // 初始化
    void Initialize(android_app* app);

    // 每帧更新
    void Update();

    // === IInputBackend 接口实现 ===
    bool GetKeyDown(KeyCode key) override;
    bool GetKeyUp(KeyCode key) override;
    bool GetPointerDown(MouseButton button) override;
    bool GetPointerUp(MouseButton button) override;

    // === Unity 风格的触控 API ===

    /**
     * 获取当前帧的触摸点数量
     */
    int GetTouchCount() const { return static_cast<int>(activeTouches_.size()); }

    /**
     * 获取指定索引的触摸点（Unity 风格）
     * @param index 触摸点索引（0-based）
     * @return 触摸点数据，如果索引无效返回空指针
     */
    const Touch* GetTouch(int index) const;

    /**
     * 获取指定手指 ID 的触摸点
     * @param fingerId 手指 ID
     * @return 触摸点数据，如果未找到返回空指针
     */
    const Touch* GetTouchById(int fingerId) const;

    /**
     * 检查是否有任何触摸点
     */
    bool IsAnyTouch() const { return !activeTouches_.empty(); }

    /**
     * 检查指定手指是否正在触摸
     */
    bool IsTouching(int fingerId) const;

    /**
     * 获取所有触摸点
     */
    const std::unordered_map<int, Touch>& GetAllTouches() const { return activeTouches_; }

    // === Android 事件处理 ===

    /**
     * 处理触摸开始事件
     */
    void OnTouchBegan(int fingerId, float x, float y);

    /**
     * 处理触摸移动事件
     */
    void OnTouchMoved(int fingerId, float x, float y);

    /**
     * 处理触摸结束事件
     */
    void OnTouchEnded(int fingerId, float x, float y);

    /**
     * 处理触摸取消事件
     */
    void OnTouchCancelled(int fingerId);

    /**
     * 设置屏幕尺寸（用于坐标转换）
     */
    void SetScreenSize(int width, int height) {
        screenWidth_ = width;
        screenHeight_ = height;
    }

    // === 鼠标模拟（用于测试）===
    Vector2 GetMousePosition() const;
    bool GetMouseButton(int button) const;

private:
    AndroidInputBackend() = default;
    ~AndroidInputBackend() = default;
    AndroidInputBackend(const AndroidInputBackend&) = delete;
    AndroidInputBackend& operator=(const AndroidInputBackend&) = delete;

    // 触摸状态
    std::unordered_map<int, Touch> activeTouches_;
    std::unordered_map<int, Touch> previousTouches_;

    // 键盘状态（如果需要连接外接键盘）
    std::unordered_map<int, bool> keyStates_;

    // 屏幕信息
    int screenWidth_ = 0;
    int screenHeight_ = 0;

    // 上一帧的触摸点数量（用于检测触摸点变化）
    int previousTouchCount_ = 0;

    // 更新触摸点状态
    void UpdateTouchPhases();
};

// === Unity 风格的全局输入函数 ===

/**
 * 获取触摸点数量
 */
inline int Input_GetTouchCount() {
    return AndroidInputBackend::GetInstance().GetTouchCount();
}

/**
 * 获取指定索引的触摸点
 */
inline const Touch* Input_GetTouch(int index) {
    return AndroidInputBackend::GetInstance().GetTouch(index);
}

/**
 * 检查是否有触摸输入
 */
inline bool Input_IsAnyTouch() {
    return AndroidInputBackend::GetInstance().IsAnyTouch();
}

/**
 * 获取鼠标位置（Android 上模拟）
 */
inline Vector2 Input_GetMousePosition() {
    return AndroidInputBackend::GetInstance().GetMousePosition();
}

/**
 * 获取鼠标按钮状态（Android 上模拟为触摸）
 */
inline bool Input_GetMouseButton(int button) {
    return AndroidInputBackend::GetInstance().GetMouseButton(button);
}

/**
 * 获取鼠标按钮按下状态（仅检测当前帧）
 */
inline bool Input_GetMouseButtonDown(int button);

/**
 * 获取鼠标按钮释放状态（仅检测当前帧）
 */
inline bool Input_GetMouseButtonUp(int button);

} // namespace Input
} // namespace PrismaEngine
