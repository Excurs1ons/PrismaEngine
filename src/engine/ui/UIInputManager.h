#pragma once

#include "UIComponent.h"
#include "../input/InputManager.h"
#include "../math/MathTypes.h"
#include <vector>
#include <memory>

namespace PrismaEngine {

/// @brief UI 输入管理器（最简实现）
/// 处理鼠标/触摸输入并分发到 UI 组件
class UIInputManager {
public:
    static UIInputManager& GetInstance();

    /// @brief 注册 UI 组件
    void RegisterComponent(UIComponent* component);

    /// @brief 注销 UI 组件
    void UnregisterComponent(UIComponent* component);

    /// @brief 处理输入事件（每帧调用）
    void ProcessInput(const Input::InputManager& inputManager);

private:
    UIInputManager() = default;
    ~UIInputManager() = default;

    void HandleMouseMove(const PrismaMath::vec2& mousePos);
    void HandleMouseButton(const PrismaMath::vec2& mousePos, Input::MouseButton button, Input::InputAction action);

    std::vector<UIComponent*> m_components;
    PrismaMath::vec2 m_lastMousePosition{0.0f, 0.0f};
};

} // namespace PrismaEngine
