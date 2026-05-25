#pragma once

#include "../LogicalPass.h"
#include "TextRendererComponent.h"
#include "../../ui/UIComponent.h"
#include "interfaces/IPass.h"
#include "interfaces/IDeviceContext.h"
#include "interfaces/IRenderTarget.h"
#include <vector>
#include <memory>

namespace Prisma {

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

// UI 逻辑 Pass
/// 负责渲染 UI 元素（文本、按钮等），不包含具体图形 API
class UIPass : public Graphic::LogicalPass {
public:
    UIPass();
    ~UIPass() override = default;

    // === IPass 接口实现 ===

    // 执行 Pass
    void Execute(const Graphic::PassExecutionContext& context) override;

    // === UI 特有功能 ===

    // 添加文本到渲染队列
    void AddText(TextRendererComponent* text, const PrismaMath::mat4& transform);

    // 添加 UI 组件到渲染队列
    void AddUIComponent(UIComponent* component);

    // 清空渲染队列
    void ClearQueue() { m_renderQueue.clear(); }

    // 获取渲染队列
    const std::vector<UIRenderItem>& GetRenderQueue() const { return m_renderQueue; }

    // 获取渲染队列（可修改）
    std::vector<UIRenderItem>& GetRenderQueue() { return m_renderQueue; }

private:
    // 渲染 UI 组件（矩形按钮等）
    void RenderUIComponent(const Graphic::PassExecutionContext& context, UIComponent* component);

    std::vector<UIRenderItem> m_renderQueue;
};

} // namespace Prisma
