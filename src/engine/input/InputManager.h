#pragma once

#include "../Export.h"
#include "../core/ISubSystem.h"  // 继承自这个，干净利索
#include "math/MathTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Prisma {

// 前置声明
class Event;

namespace Input {

// 按键码定义（对应 SDL3 scancode 值 — 与 C# Prisma.Core/KeyCode.cs 保持一致）
enum class KeyCode : uint32_t {
    A          = 4,
    B          = 5,
    C          = 6,
    D          = 7,
    E          = 8,
    F          = 9,
    G          = 10,
    H          = 11,
    I          = 12,
    J          = 13,
    K          = 14,
    L          = 15,
    M          = 16,
    N          = 17,
    O          = 18,
    P          = 19,
    Q          = 20,
    R          = 21,
    S          = 22,
    T          = 23,
    U          = 24,
    V          = 25,
    W          = 26,
    X          = 27,
    Y          = 28,
    Z          = 29,
    Num1       = 30,
    Num2       = 31,
    Num3       = 32,
    Num4       = 33,
    Num5       = 34,
    Num6       = 35,
    Num7       = 36,
    Num8       = 37,
    Num9       = 38,
    Num0       = 39,
    Enter      = 40,
    Escape     = 41,
    Backspace  = 42,
    Tab        = 43,
    Space      = 44,
    Delete     = 76,
    LeftBracket  = 47,
    RightBracket = 48,
    ArrowRight = 79,
    ArrowLeft  = 80,
    ArrowDown  = 81,
    ArrowUp    = 82,
    F1         = 58,
    F2         = 59,
    F3         = 60,
    F4         = 61,
    F5         = 62,
    F6         = 63,
    F7         = 64,
    F8         = 65,
    F9         = 66,
    F10        = 67,
    F11        = 68,
    F12        = 69,
    LeftCtrl   = 224,
    LShift     = 225,
    LeftAlt    = 226,
    RightCtrl  = 228,
    RShift     = 229,
    RightAlt   = 230,
    Unknown    = 0
};

// 鼠标按钮
enum class MouseButton : uint32_t { Left = 0, Right = 1, Middle = 2, X1 = 3, X2 = 4, Count };

// 输入动作类型
enum class InputAction { Pressed, Released, Held, DoubleClick };

/* 输入管理器 (由 Engine 拥有) */
class ENGINE_API InputManager : public ISubSystem {
public:
    InputManager();
    ~InputManager() override;

    // ISubSystem 接口
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "InputManager"; }

    // 事件处理 (由 Engine 分发)
    void OnEvent(Event& e);

    // 状态查询
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyJustPressed(KeyCode key) const;
    bool IsKeyJustReleased(KeyCode key) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonJustPressed(MouseButton button) const;
    bool IsMouseButtonJustReleased(MouseButton button) const;

    // 之前叫 vec2，现在建议加上命名空间前缀
    PrismaMath::vec2 GetMousePosition() const;
    PrismaMath::vec2 GetMouseDelta() const;

    // 内部使用的状态更新方法
    void SetKeyState(KeyCode key, bool pressed);
    void SetMouseButtonState(MouseButton button, bool pressed);
    void SetMousePosition(const PrismaMath::vec2& pos);
    void ClearState();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace Input
}  // namespace Prisma
