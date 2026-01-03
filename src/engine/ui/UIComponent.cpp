#include "UIComponent.h"

namespace PrismaEngine {

void UIComponent::Update(float deltaTime) {
    // 基础 Update - 子类可以扩展
}

Vector2 UIComponent::GetScreenPosition() const {
    Vector2 parentPos{0.0f, 0.0f};
    Vector2 parentSize{1920.0f, 1080.0f};  // 默认屏幕尺寸

    if (m_parent != nullptr) {
        parentPos = m_parent->GetScreenPosition();
        parentSize = m_parent->GetSize();
    }

    // 计算锚点在父组件中的位置
    Vector2 anchorPos = parentPos + Vector2(
        parentSize.x * m_anchor.x,
        parentSize.y * m_anchor.y
    );

    // 减去枢轴偏移（使枢轴对齐到锚点）
    Vector2 pivotOffset = Vector2(
        m_size.x * m_pivot.x,
        m_size.y * m_pivot.y
    );

    return anchorPos + m_position - pivotOffset;
}

bool UIComponent::HitTest(const Vector2& point) const {
    Vector2 screenPos = GetScreenPosition();
    return point.x >= screenPos.x && point.x <= screenPos.x + m_size.x &&
           point.y >= screenPos.y && point.y <= screenPos.y + m_size.y;
}

} // namespace PrismaEngine
