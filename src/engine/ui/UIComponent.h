#pragma once

#include "Component.h"
#include "math/MathTypes.h"
#include <functional>
#include <memory>

namespace PrismaEngine {

/// @brief UI 组件基类（最简实现）
/// 使用屏幕坐标系：原点在左上角，X 向右，Y 向下
class UIComponent : public Component {
public:
    UIComponent() = default;
    ~UIComponent() override = default;

    // Component 接口
    void Initialize() override {}
    void Update(float deltaTime) override;
    void Shutdown() override {}

    // === 基础属性（屏幕坐标） ===
    /// @brief 设置位置（相对于锚点的偏移）
    void SetPosition(const PrismaMath::vec2& pos) { m_position = pos; }
    const PrismaMath::vec2& GetPosition() const { return m_position; }

    /// @brief 设置尺寸（像素）
    void SetSize(const PrismaMath::vec2& size) { m_size = size; }
    const PrismaMath::vec2& GetSize() const { return m_size; }

    /// @brief 设置锚点（相对于父组件，0-1 范围）
    /// (0,0) = 左上角, (0.5,0.5) = 中心, (1,1) = 右下角
    void SetAnchor(const PrismaMath::vec2& anchor) { m_anchor = anchor; }
    const PrismaMath::vec2& GetAnchor() const { return m_anchor; }

    /// @brief 设置枢轴（相对于自身，0-1 范围）
    /// 枢轴是组件自身的旋转和缩放中心
    void SetPivot(const PrismaMath::vec2& pivot) { m_pivot = pivot; }
    const PrismaMath::vec2& GetPivot() const { return m_pivot; }

    /// @brief 获取最终屏幕位置（考虑锚点和父组件）
    PrismaMath::vec2 GetScreenPosition() const;

    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }

    void SetInteractable(bool interactable) { m_interactable = interactable; }
    bool IsInteractable() const { return m_interactable && m_visible; }

    // === 点击检测（屏幕坐标系） ===
    virtual bool HitTest(const PrismaMath::vec2& point) const;

    // === 事件处理 ===
    using ClickCallback = std::function<void()>;
    void SetOnClick(ClickCallback callback) { m_onClick = std::move(callback); }

    virtual void OnClicked() {
        if (m_onClick) {
            m_onClick();
        }
    }

    // === 状态查询 ===
    bool IsHovered() const { return m_isHovered; }
    void SetHovered(bool hovered) { m_isHovered = hovered; }

    bool IsPressed() const { return m_isPressed; }
    void SetPressed(bool pressed) { m_isPressed = pressed; }

    // === 颜色（用于渲染）===
    void SetColor(const PrismaMath::vec4& color) { m_color = color; }
    const PrismaMath::vec4& GetColor() const { return m_color; }

    // === 层级 ===
    void SetParent(std::shared_ptr<UIComponent> parent) { m_parent = parent; }
    std::shared_ptr<UIComponent> GetParent() const { return m_parent; }

protected:
    // 基础属性（屏幕坐标系，左上角为原点）
    PrismaMath::vec2 m_position{0.0f, 0.0f};  // 相对于锚点的偏移
    PrismaMath::vec2 m_size{100.0f, 50.0f};   // 宽度和高度

    // 锚点和枢轴（默认居中）
    PrismaMath::vec2 m_anchor{0.5f, 0.5f};    // 锚点（相对父组件，默认居中）
    PrismaMath::vec2 m_pivot{0.5f, 0.5f};     // 枢轴（相对自身，默认居中）

    bool m_visible = true;
    bool m_interactable = true;

    // 层级
    std::shared_ptr<UIComponent> m_parent = nullptr;

    // 交互状态
    bool m_isHovered = false;
    bool m_isPressed = false;

    // 渲染颜色
    PrismaMath::vec4 m_color{1.0f, 1.0f, 1.0f, 1.0f};

    // 点击回调
    ClickCallback m_onClick;
};

} // namespace PrismaEngine
