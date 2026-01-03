#pragma once

#include "RenderPass.h"
#include <memory>
#include <vector>

namespace PrismaEngine {

class UIComponent;

/// @brief UI 渲染 Pass（Android runtime 适配器）
class UIPass : public RenderPass {
public:
    UIPass():RenderPass("UI Pass"){};
    ~UIPass() override = default;

    void record(VkCommandBuffer cmdBuffer) override;

    void addUIComponent(std::shared_ptr<UIComponent> component);

    // RenderPass 接口实现
    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void cleanup(VkDevice device) override;

private:
    std::vector<std::shared_ptr<UIComponent>> m_uiComponents;
};

} // namespace PrismaEngine
