#include "CanvasComponent.h"

namespace PrismaEngine {

void CanvasComponent::Initialize() {
    // 设置为全屏
    // TODO: 从窗口/视口获取屏幕尺寸
    m_size = {1920.0f, 1080.0f};
    m_position = {0.0f, 0.0f};
}

void CanvasComponent::Update(float deltaTime) {
    UIComponent::Update(deltaTime);

    // 更新所有子组件
    for (auto* child : m_children) {
        child->Update(deltaTime);
    }
}

void CanvasComponent::AddChild(UIComponent* child) {
    if (child) {
        m_children.push_back(child);
    }
}

void CanvasComponent::RemoveChild(UIComponent* child) {
    auto it = std::remove(m_children.begin(), m_children.end(), child);
    m_children.erase(it, m_children.end());
}

} // namespace PrismaEngine
