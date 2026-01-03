#pragma once

#include "UIComponent.h"
#include <vector>

namespace PrismaEngine {

/// @brief 渲染模式
enum class CanvasRenderMode {
    ScreenSpace,       // 屏幕空间（2D UI）
    ScreenSpaceCamera, // 屏幕空间-相机（带透视）
    WorldSpace,        // 世界空间（3D UI，暂不实现）
};

/// @brief Canvas 画布容器（最简实现）
class CanvasComponent : public UIComponent {
public:
    CanvasComponent() = default;
    ~CanvasComponent() override = default;

    void Initialize() override;
    void Update(float deltaTime) override;

    /// @brief 设置渲染模式
    void SetRenderMode(CanvasRenderMode mode) { m_renderMode = mode; }
    CanvasRenderMode GetRenderMode() const { return m_renderMode; }

    /// @brief 添加子组件
    void AddChild(UIComponent* child);
    void RemoveChild(UIComponent* child);

    /// @brief 获取所有子组件
    const std::vector<UIComponent*>& GetChildren() const { return m_children; }

private:
    CanvasRenderMode m_renderMode = CanvasRenderMode::ScreenSpace;
    std::vector<UIComponent*> m_children;
};

} // namespace PrismaEngine
