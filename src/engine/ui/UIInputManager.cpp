#include "UIInputManager.h"
#include "2d/ButtonComponent.h"

namespace PrismaEngine {

UIInputManager& UIInputManager::GetInstance() {
    static UIInputManager instance;
    return instance;
}

void UIInputManager::RegisterComponent(UIComponent* component) {
    if (component) {
        m_components.push_back(component);
    }
}

void UIInputManager::UnregisterComponent(UIComponent* component) {
    auto it = std::remove(m_components.begin(), m_components.end(), component);
    m_components.erase(it, m_components.end());
}

void UIInputManager::ProcessInput(const Input::InputManager& inputManager) {
    PrismaMath::vec2 mousePos = inputManager.GetMousePosition();
    PrismaMath::vec2 mouseDelta = inputManager.GetMouseDelta();

    // 处理鼠标移动
    if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
        HandleMouseMove(mousePos);
    }

    // 处理鼠标按键
    for (int i = 0; i < static_cast<int>(Input::MouseButton::Count); ++i) {
        auto button = static_cast<Input::MouseButton>(i);
        if (inputManager.IsMouseButtonJustPressed(button)) {
            HandleMouseButton(mousePos, button, Input::InputAction::Pressed);
        } else if (inputManager.IsMouseButtonJustReleased(button)) {
            HandleMouseButton(mousePos, button, Input::InputAction::Released);
        }
    }

    m_lastMousePosition = mousePos;
}

void UIInputManager::HandleMouseMove(const PrismaMath::vec2& mousePos) {
    for (auto* component : m_components) {
        if (!component->IsInteractable()) {
            continue;
        }

        bool wasHovered = component->IsHovered();
        bool isHovered = component->HitTest(mousePos);

        if (isHovered && !wasHovered) {
            // 进入悬停
            component->SetHovered(true);
            auto* button = dynamic_cast<ButtonComponent*>(component);
            if (button) {
                button->OnHoverEnter();
            }
        } else if (!isHovered && wasHovered) {
            // 离开悬停
            component->SetHovered(false);
            auto* button = dynamic_cast<ButtonComponent*>(component);
            if (button) {
                button->OnHoverLeave();
            }
        }
    }
}

void UIInputManager::HandleMouseButton(const PrismaMath::vec2& mousePos, Input::MouseButton button, Input::InputAction action) {
    if (button != Input::MouseButton::Left) {
        return;  // 只处理左键
    }

    // 从后往前遍历（后添加的在上层）
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
        auto* component = *it;
        if (!component->IsInteractable()) {
            continue;
        }

        if (component->HitTest(mousePos)) {
            auto* buttonComp = dynamic_cast<ButtonComponent*>(component);
            if (buttonComp) {
                if (action == Input::InputAction::Pressed) {
                    component->SetPressed(true);
                    buttonComp->OnPressed();
                } else if (action == Input::InputAction::Released) {
                    component->SetPressed(false);
                    buttonComp->OnReleased();
                }
            }
            break;  // 只处理最上层的组件
        }
    }
}

} // namespace PrismaEngine
