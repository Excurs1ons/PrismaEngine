#pragma once

#include "interfaces/IComputePipeline.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Prisma::Graphic::Vulkan {

class RenderDeviceVulkan;

class VulkanComputePipeline : public IComputePipeline {
public:
    VulkanComputePipeline();
    ~VulkanComputePipeline() override;

    bool IsValid() const override { return m_pipeline != VK_NULL_HANDLE; }

    void SetShader(std::shared_ptr<IShader> shader) override;
    std::shared_ptr<IShader> GetShader() const override { return m_shader; }

    void SetPushConstantRange(uint32_t size, uint32_t offset = 0) override;
    uint32_t GetPushConstantRangeSize() const override { return m_pushConstantRange.size; }
    uint32_t GetPushConstantRangeOffset() const override { return m_pushConstantRange.offset; }

    const std::vector<std::shared_ptr<IDescriptorSetLayout>>& GetDescriptorSetLayouts() const override { return m_descriptorSetLayouts; }

    bool Create(IRenderDevice* device) override;

    void SetDebugName(const std::string& name) override { m_debugName = name; }
    const std::string& GetDebugName() const override { return m_debugName; }

    VkPipeline GetVkPipeline() const { return m_pipeline; }
    VkPipelineLayout GetVkPipelineLayout() const { return m_pipelineLayout; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPushConstantRange m_pushConstantRange{};
    std::shared_ptr<IShader> m_shader;
    std::vector<std::shared_ptr<IDescriptorSetLayout>> m_descriptorSetLayouts;
    std::string m_debugName;
};

} // namespace Prisma::Graphic::Vulkan
