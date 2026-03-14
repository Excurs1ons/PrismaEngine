#pragma once

#include "../Export.h"
#include "../ISubSystem.h" // 继承自这个，干净利索
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

// 按键码定义 (省略详细内容以节省篇幅，实际文件中保留)
enum class KeyCode : uint32_t {
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88, Y = 89, Z = 90,
    Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52, Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295, F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,
    Space = 32, Enter = 257, Tab = 258, Backspace = 259, Delete = 261, Escape = 256,
    ArrowLeft = 263, ArrowRight = 262, ArrowUp = 265, ArrowDown = 264,
    LeftShift = 340, RightShift = 344, LeftCtrl = 341, RightCtrl = 345, LeftAlt = 342, RightAlt = 346,
    Unknown = 0
};

// 鼠标按钮
enum class MouseButton : uint32_t { Left = 0, Right = 1, Middle = 2, X1 = 3, X2 = 4, Count };

// 输入动作类型
enum class InputAction { Pressed, Released, Held, DoubleClick };

/**
 * @brief 输入管理器 (由 Engine 拥有)
 * 删掉所有静态 Get() 和垃圾单例逻辑。
 */
class ENGINE_API InputManager : public ISubSystem {
public:
    InputManager();
    ~InputManager() override;

    // ISubSystem 接口
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

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
