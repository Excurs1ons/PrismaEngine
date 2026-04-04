#pragma once

#include "interfaces/ICommandBuffer.h"
#include <vulkan/vulkan.h>

namespace Prisma::Graphic::Vulkan {

class VulkanCommandBuffer : public ICommandBuffer {
public:
    VulkanCommandBuffer(VkCommandBuffer cmd) : m_cmd(cmd) {}
    ~VulkanCommandBuffer() override = default;

    // === 生命周期 ===
    void Begin() override {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(m_cmd, &beginInfo);
    }
    void End() override { vkEndCommandBuffer(m_cmd); }
    bool Reset() override { return vkResetCommandBuffer(m_cmd, 0) == VK_SUCCESS; }

    // === 渲染通道 ===
    void BeginRenderPass(const RenderPassDesc& desc) override;
    void EndRenderPass() override { vkCmdEndRenderPass(m_cmd); }

    // === 状态与管线 ===
    void SetPipelineState(IPipelineState* pipelineState) override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissorRect(const Rect& rect) override;

    // === 资源绑定 ===
    void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset = 0) override;
    void SetIndexBuffer(IBuffer* buffer, bool is32Bit = true, uint32_t offset = 0) override;
    void BindDescriptorSet(uint32_t set, IDescriptorSet* descriptorSet) override;
    void PushConstants(ShaderType stage, const void* data, uint32_t size) override;

    // === 绘制命令 ===
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0) override {
        vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, startIndex, baseVertex, 0);
    }
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0) override {
        vkCmdDraw(m_cmd, vertexCount, instanceCount, startVertex, 0);
    }

    // === 间接绘制与计算 ===
    void Dispatch(uint32_t x, uint32_t y, uint32_t z) override { vkCmdDispatch(m_cmd, x, y, z); }
    void DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset = 0) override;

    // === 资源同步与屏障 ===
    void PipelineBarrier() override {}

    // === 调试 ===
    void BeginDebugGroup(const std::string& name) override {}
    void EndDebugGroup() override {}

    VkCommandBuffer GetVkCommandBuffer() const { return m_cmd; }

private:
    VkCommandBuffer m_cmd;
    VkPipelineLayout m_currentLayout = VK_NULL_HANDLE;
};

} // namespace Prisma::Graphic::Vulkan
