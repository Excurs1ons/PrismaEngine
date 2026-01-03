#include "ButtonComponent.h"

namespace PrismaEngine {

void ButtonComponent::Initialize() {
    // 初始化为正常颜色
    UpdateColor();
}

void ButtonComponent::Update(float deltaTime) {
    UIComponent::Update(deltaTime);
    // 颜色在状态改变时更新
}

void ButtonComponent::UpdateColor() {
    if (m_isPressed) {
        m_color = m_pressedColor;
    } else if (m_isHovered) {
        m_color = m_hoverColor;
    } else {
        m_color = m_normalColor;
    }
}

void ButtonComponent::OnHoverEnter() {
    m_isHovered = true;
    UpdateColor();
}

void ButtonComponent::OnHoverLeave() {
    m_isHovered = false;
    m_isPressed = false;
    UpdateColor();
}

void ButtonComponent::OnPressed() {
    m_isPressed = true;
    UpdateColor();
}

void ButtonComponent::OnReleased() {
    if (m_isPressed && m_isHovered) {
        OnClicked();  // 触发点击回调
    }
    m_isPressed = false;
    UpdateColor();
}

} // namespace PrismaEngine
