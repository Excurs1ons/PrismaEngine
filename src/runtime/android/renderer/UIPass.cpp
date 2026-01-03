#include "UIPass.h"
#include "UIComponent.h"
#include "2d/ButtonComponent.h"
#include "AndroidOut.h"
#include <vulkan/vulkan.h>

namespace PrismaEngine {

void UIPass::addUIComponent(std::shared_ptr<UIComponent> component) {
    if (component) {
        m_uiComponents.push_back(component);
    }
}

void UIPass::record(VkCommandBuffer cmdBuffer) {
    // 简单的矩形渲染实现
    // TODO: 集成到 Vulkan 渲染管线

    for (const auto& component : m_uiComponents) {
        if (!component || !component->IsVisible()) {
            continue;
        }

        // 获取组件的屏幕位置和颜色
        const auto& pos = component->GetScreenPosition();
        const auto& size = component->GetSize();
        const auto& color = component->GetColor();

        // 调试输出
        static int debugCount = 0;
        if (debugCount < 3) {  // 只输出几次
            aout << "渲染 UI 组件: pos=(" << pos.x << ", " << pos.y
                 << ") size=(" << size.x << "x" << size.y
                 << ") color=(" << color.x << ", " << color.y << ", " << color.z << ", " << color.w << ")" << std::endl;
            debugCount++;
        }
    }
}

    void UIPass::initialize(VkDevice device, VkRenderPass renderPass) {

    }

    void UIPass::cleanup(VkDevice device) {

    }

} // namespace PrismaEngine
