#pragma once

#include "interfaces/ICommandBuffer.h"
#include "interfaces/IComputePipeline.h"
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
        m_commandCount = 0;
    }
    void End() override { vkEndCommandBuffer(m_cmd); }
    bool Reset() override { m_commandCount = 0; return vkResetCommandBuffer(m_cmd, 0) == VK_SUCCESS; }

    // === 渲染通道 ===
    void BeginRenderPass(const RenderPassDesc& desc) override;
    void EndRenderPass() override { vkCmdEndRenderPass(m_cmd); }

    // === 状态与管线 ===
    void SetPipelineState(IPipelineState* pipelineState) override;
    void SetComputePipeline(IComputePipeline* pipeline) override;
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
        m_commandCount++;
    }
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0) override {
        vkCmdDraw(m_cmd, vertexCount, instanceCount, startVertex, 0);
        m_commandCount++;
    }

    // === 间接绘制与计算 ===
    void Dispatch(uint32_t x, uint32_t y, uint32_t z) override {
        vkCmdDispatch(m_cmd, x, y, z);
        m_commandCount++;
    }
    void DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset = 0) override;

    // === 资源同步与屏障 ===
    void PipelineBarrier() override {
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

        vkCmdPipelineBarrier(m_cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 1, &barrier, 0, nullptr, 0, nullptr);
    }
    void PipelineBarrier(const std::vector<ImageBarrier>& imageBarriers) override;

    // === 调试 (修复接口匹配) ===
    void BeginDebugGroup([[maybe_unused]] const std::string& name) override { }
    void EndDebugGroup() override {}

    void* GetNativeHandle() const override { return reinterpret_cast<void*>(m_cmd); }
    VkCommandBuffer GetVkCommandBuffer() const { return m_cmd; }
    uint32_t GetAndResetCommandCount() { uint32_t c = m_commandCount; m_commandCount = 0; return c; }

    // === 离屏渲染资源管理 ===
    static void ReleaseOffscreenResources(VkImageView imageView);
    static void ReleaseAllOffscreenResources();

    /// 预创建并缓存离屏渲染通道（RenderPass）。
    /// 在初始化阶段调用，使得 PSO 可以在任何 BeginRenderPass 调用之前
    /// 使用正确的 RenderPass 创建。返回 VkRenderPass 句柄。
    static VkRenderPass PreCreateOffscreenRenderPass(
        VkDevice device,
        VkImageView colorView, VkFormat colorFormat,
        VkImageView depthView, VkFormat depthFormat,
        uint32_t width, uint32_t height,
        bool clearColor = true, bool clearDepth = true);

private:
    VkCommandBuffer m_cmd;
    VkPipelineLayout m_currentLayout = VK_NULL_HANDLE;
    VkPipelineBindPoint m_currentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    uint32_t m_commandCount = 0;
};

} // namespace Prisma::Graphic::Vulkan
