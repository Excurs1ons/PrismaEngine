#pragma once

#include "../UIComponent.h"
#include "../math/MathTypes.h"
#include <string>

namespace PrismaEngine {

/// @brief 按钮组件（最简实现）
class ButtonComponent : public UIComponent {
public:
    ButtonComponent() = default;
    ~ButtonComponent() override = default;

    void Initialize() override;
    void Update(float deltaTime) override;

    // === 文本设置 ===
    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() const { return m_text; }

    // === 状态颜色 ===
    void SetNormalColor(const PrismaMath::vec4& color) { m_normalColor = color; UpdateColor(); }
    void SetHoverColor(const PrismaMath::vec4& color) { m_hoverColor = color; UpdateColor(); }
    void SetPressedColor(const PrismaMath::vec4& color) { m_pressedColor = color; UpdateColor(); }

    // === 生命周期回调 ===
    void OnHoverEnter();
    void OnHoverLeave();
    void OnPressed();
    void OnReleased();

protected:
    void UpdateColor();

private:
    std::string m_text = "Button";

    // 状态颜色
    PrismaMath::vec4 m_normalColor{0.2f, 0.6f, 1.0f, 1.0f};  // 蓝色
    PrismaMath::vec4 m_hoverColor{0.3f, 0.7f, 1.0f, 1.0f};   // 亮蓝色
    PrismaMath::vec4 m_pressedColor{0.1f, 0.5f, 0.9f, 1.0f}; // 深蓝色
};

} // namespace PrismaEngine
