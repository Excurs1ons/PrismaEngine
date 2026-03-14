#include "CanvasComponent.h"
#include "../Platform.h"
#include <algorithm>

namespace Prisma {

void CanvasComponent::Initialize() {
    int w = 0, h = 0;
    auto window = Prisma::Platform::GetCurrentWindow();
    if (window) {
        Prisma::Platform::GetWindowSize(window, w, h);
    }
    if (w <= 0 || h <= 0) {
        w = 1920;
        h = 1080;
    }
    m_size = {static_cast<float>(w), static_cast<float>(h)};
    m_position = {0.0f, 0.0f};
}

void CanvasComponent::Update(Timestep ts) {
    UIComponent::Update(ts);

    // 更新所有子组件
    for (auto* child : m_children) {
        child->Update(ts);
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

} // namespace Prisma
