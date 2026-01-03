#pragma once

#include "../LogicalPass.h"
#include "TextRendererComponent.h"
#include "../../ui/UIComponent.h"
#include "interfaces/IPass.h"
#include "interfaces/IDeviceContext.h"
#include "interfaces/IRenderTarget.h"
#include <vector>
#include <memory>

namespace PrismaEngine {

// 前向声明
class UIComponent;

// UI 渲染项
struct UIRenderItem {
    enum class Type {
        Text,
        Component
    } type = Type::Text;

    TextRendererComponent* textComponent = nullptr;
    UIComponent* uiComponent = nullptr;
    PrismaMath::mat4 transform;
};

/// @brief UI 逻辑 Pass
/// 负责渲染 UI 元素（文本、按钮等），不包含具体图形 API
class UIPass : public Graphic::LogicalPass {
public:
    UIPass();
    ~UIPass() override = default;

    // === IPass 接口实现 ===

    /// @brief 执行 Pass
    /// @param context 执行上下文
    void Execute(const Graphic::PassExecutionContext& context) override;

    // === UI 特有功能 ===

    /// @brief 添加文本到渲染队列
    /// @param text 文本渲染组件
    /// @param transform 变换矩阵
    void AddText(TextRendererComponent* text, const PrismaMath::mat4& transform);

    /// @brief 添加 UI 组件到渲染队列
    /// @param component UI 组件
    void AddUIComponent(UIComponent* component);

    /// @brief 清空渲染队列
    void ClearQueue() { m_renderQueue.clear(); }

    /// @brief 获取渲染队列
    const std::vector<UIRenderItem>& GetRenderQueue() const { return m_renderQueue; }

    /// @brief 获取渲染队列（可修改）
    std::vector<UIRenderItem>& GetRenderQueue() { return m_renderQueue; }

private:
    /// @brief 渲染 UI 组件（矩形按钮等）
    void RenderUIComponent(const Graphic::PassExecutionContext& context, UIComponent* component);

    std::vector<UIRenderItem> m_renderQueue;
};

} // namespace PrismaEngine
