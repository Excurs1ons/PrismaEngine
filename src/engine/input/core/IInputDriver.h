#pragma once

#include <cstdint>
#include <string>

namespace PrismaEngine::Input {

/// @brief 按键码
enum class KeyCode : uint32_t {
    // 未知
    Unknown = 0,

    // 字母键
    A = 4, B = 5, C = 6, D = 7, E = 8, F = 9, G = 10, H = 11, I = 12,
    J = 13, K = 14, L = 15, M = 16, N = 17, O = 18, P = 19, Q = 20, R = 21,
    S = 22, T = 23, U = 24, V = 25, W = 26, X = 27, Y = 28, Z = 29,

    // 数字键
    Num0 = 30, Num1 = 31, Num2 = 32, Num3 = 33, Num4 = 34,
    Num5 = 35, Num6 = 36, Num7 = 37, Num8 = 38, Num9 = 39,

    // 功能键
    F1 = 40, F2 = 41, F3 = 42, F4 = 43, F5 = 44, F6 = 45,
    F7 = 46, F8 = 47, F9 = 48, F10 = 49, F11 = 50, F12 = 51,

    // 方向键
    Up = 82, Down = 81, Left = 80, Right = 79,

    // 控制键
    Escape = 1, Enter = 40, Tab = 43, Backspace = 42,
    Space = 44, Shift = 225, Ctrl = 224, Alt = 226,

    // 符号键
    Minus = 45, Equals = 46, BracketLeft = 47, BracketRight = 48,
    Backslash = 49, Semicolon = 51, Apostrophe = 52, Grave = 53,
    Comma = 54, Period = 55, Slash = 56,

    // 特殊键
    CapsLock = 57, PrintScreen = 70, ScrollLock = 71, Pause = 72,
    Insert = 73, Home = 74, PageUp = 75, Delete = 76, End = 77, PageDown = 78
};

/// @brief 鼠标按钮
enum class MouseButton : uint8_t {
    None = 0,
    Left = 1,
    Middle = 2,
    Right = 3,
    X1 = 4,
    X2 = 5
};

/// @brief 手柄按钮
enum class GamepadButton : uint8_t {
    None = 0,
    // 面按钮
    A = 1, B = 2, X = 3, Y = 4,
    // 肩膀按钮
    LeftShoulder = 5, RightShoulder = 6,
    // 扳机按钮
    LeftTrigger = 7, RightTrigger = 8,
    // 系统按钮
    Back = 9, Start = 10, Guide = 11,
    // 摇杆按钮
    LeftStick = 12, RightStick = 13,
    // 方向键
    DPadUp = 14, DPadDown = 15, DPadLeft = 16, DPadRight = 17
};

/// @brief 手柄轴
enum class GamepadAxis : uint8_t {
    LeftX = 0,
    LeftY = 1,
    RightX = 2,
    RightY = 3,
    LeftTrigger = 4,
    RightTrigger = 5
};

/// @brief 输入状态
struct InputState {
    bool pressed;    // 当前是否按下
    bool justPressed;   // 刚按下（本帧）
    bool justReleased;  // 刚释放（本帧）
};

/// @brief 鼠标状态
struct MouseState {
    int x, y;           // 位置
    int deltaX, deltaY; // 相对移动
    InputState buttons[6];  // 按钮状态
    int wheelDelta;     // 滚轮增量
};

/// @brief 手柄状态
struct GamepadState {
    InputState buttons[18];      // 按钮状态
    float axes[6];              // 轴值 (-1.0 到 1.0)
    bool connected;             // 是否连接
};

/// @brief 输入驱动抽象接口
/// 职责：与平台原生输入API交互，提供最底层的输入数据采集
///
/// 设计原则：
/// 1. 只处理与系统输入API的直接交互
/// 2. 只采集原始输入数据，不做高级处理
/// 3. 接口简洁，易于跨平台实现
class IInputDriver {
public:
    virtual ~IInputDriver() = default;

    /// @brief 获取驱动名称
    virtual const char* GetName() const = 0;

    /// @brief 初始化输入驱动
    /// @return 是否成功
    virtual bool Initialize() = 0;

    /// @brief 关闭输入驱动
    virtual void Shutdown() = 0;

    /// @brief 检查是否已初始化
    virtual bool IsInitialized() const = 0;

    /// @brief 更新输入状态（每帧调用）
    virtual void Update() = 0;

    // ========== 键盘 ==========

    /// @brief 检查按键是否按下
    virtual bool IsKeyDown(KeyCode key) const = 0;

    /// @brief 检查按键是否刚刚按下
    virtual bool IsKeyJustPressed(KeyCode key) const = 0;

    /// @brief 检查按键是否刚刚释放
    virtual bool IsKeyJustReleased(KeyCode key) const = 0;

    // ========== 鼠标 ==========

    /// @brief 获取鼠标状态
    virtual const MouseState& GetMouseState() const = 0;

    /// @brief 设置鼠标位置
    virtual void SetMousePosition(int x, int y) = 0;

    /// @brief 是否支持绝对鼠标位置
    virtual bool SupportsAbsolutePosition() const { return true; }

    // ========== 手柄 ==========

    /// @brief 获取手柄数量
    virtual uint32_t GetGamepadCount() const = 0;

    /// @brief 检查手柄是否连接
    virtual bool IsGamepadConnected(uint32_t index) const = 0;

    /// @brief 获取手柄状态
    virtual const GamepadState& GetGamepadState(uint32_t index) const = 0;

    /// @brief 设置手柄振动
    /// @param index 手柄索引
    /// @param leftMotor 左马达强度 (0.0 - 1.0)
    /// @param rightMotor 右马达强度 (0.0 - 1.0)
    /// @param duration 持续时间（毫秒）
    virtual void SetVibration(uint32_t index, float leftMotor, float rightMotor, uint32_t duration) = 0;

    // ========== 文本输入 ==========

    /// @brief 获取输入的文本（本帧）
    virtual const std::string& GetTextInput() const = 0;

    /// @brief 开始文本输入（如打开IME）
    virtual void StartTextInput() = 0;

    /// @brief 停止文本输入
    virtual void StopTextInput() = 0;
};

// ========== 驱动工厂函数类型 ==========

/// @brief 驱动创建函数类型
using DriverCreateFunc = IInputDriver*(*)();

} // namespace PrismaEngine::Input
