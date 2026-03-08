#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include "math/MathTypes.h"
#include "../Export.h"
#include "../ManagerBase.h"

namespace PrismaEngine {
namespace Input {

// 按键码定义
enum class KeyCode : uint32_t {
    // 字母
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74, K = 75, L = 76, M = 77,
    N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88, Y = 89, Z = 90,
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52, Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295, F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,
    Space = 32, Enter = 257, Tab = 258, Backspace = 259, Delete = 261, Escape = 256,
    ArrowLeft = 263, ArrowRight = 262, ArrowUp = 265, ArrowDown = 264,
    Left = 263, Right = 262, Up = 265, Down = 264,
    LeftShift = 340, RightShift = 344, LeftCtrl = 341, RightCtrl = 345,
    LeftAlt = 342, RightAlt = 346,
    CapsLock = 280, Grave = 96, Minus = 45, Equal = 61,
    LeftBracket = 91, RightBracket = 93, Backslash = 92, Semicolon = 59,
    Apostrophe = 39, Comma = 44, Period = 46, Slash = 47,
    Unknown = 0
};

// 鼠标按钮
enum class MouseButton : uint32_t {
    Left = 0, Right = 1, Middle = 2, X1 = 3, X2 = 4, Count
};

// 输入动作类型
enum class InputAction { Pressed, Released, Held, DoubleClick };

// 输入绑定 helper
class ENGINE_API InputBinding {
public:
    using Callback = std::function<void()>;
    InputBinding(const std::string& name, KeyCode key, InputAction action, Callback callback)
        : m_name(name), m_key(key), m_action(action), m_callback(callback) {}
    
    void Update(bool isPressed) {
        if (!m_enabled || !m_callback) return;
        bool triggered = false;
        if (m_action == InputAction::Pressed && isPressed && !m_wasPressed) triggered = true;
        else if (m_action == InputAction::Released && !isPressed && m_wasPressed) triggered = true;
        else if (m_action == InputAction::Held && isPressed) triggered = true;
        if (triggered) m_callback();
        m_wasPressed = isPressed;
    }
    
    void SetEnabled(bool enabled) { m_enabled = enabled; }
private:
    std::string m_name;
    KeyCode m_key;
    InputAction m_action;
    Callback m_callback;
    bool m_enabled = true;
    bool m_wasPressed = false;
};

// 输入管理器
class ENGINE_API InputManager : public ManagerBase<InputManager> {
public:
    static std::shared_ptr<InputManager> GetInstance();

    InputManager();
    ~InputManager() override;

    int Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    // 状态查询
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyJustPressed(KeyCode key) const;
    bool IsKeyJustReleased(KeyCode key) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonJustPressed(MouseButton button) const;
    bool IsMouseButtonJustReleased(MouseButton button) const;
    PrismaMath::vec2 GetMousePosition() const;
    PrismaMath::vec2 GetMouseDelta() const;

    // 设置状态
    void SetKeyState(KeyCode key, bool pressed);
    void SetMouseButtonState(MouseButton button, bool pressed);
    void SetMousePosition(const PrismaMath::vec2& pos);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Input
} // namespace PrismaEngine
