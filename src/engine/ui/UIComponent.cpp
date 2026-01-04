#include "UIComponent.h"

namespace PrismaEngine {

void UIComponent::Update(float deltaTime) {
    // 基础 Update - 子类可以扩展
}

Vector2 UIComponent::GetScreenPosition() const {
    // 如果没有父组件（根组件），直接返回自己的位置
    // 假设根组件的位置就是屏幕坐标
    if (m_parent == nullptr) {
        return m_position;
    }

    // 有父组件，计算相对于父组件的位置
    Vector2 parentPos = m_parent->GetScreenPosition();
    Vector2 parentSize = m_parent->GetSize();

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
