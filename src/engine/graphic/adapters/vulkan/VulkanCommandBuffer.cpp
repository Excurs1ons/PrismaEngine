#include "VulkanCommandBuffer.h"
#include "VulkanResources.h"
#include "VulkanPipelineState.h"
#include "VulkanSwapChain.h"
#include "VulkanShader.h"
#include <algorithm>

namespace Prisma::Graphic::Vulkan {

void VulkanCommandBuffer::BeginRenderPass(const RenderPassDesc& desc) {
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    
    // 如果没有指定 RenderTarget，默认使用交换链
    if (!desc.renderTarget) {
        // 这里需要访问 SwapChain，逻辑较复杂，暂时假设外部已经开启了默认 RenderPass
        // 或者从 Device 获取当前 Framebuffer
        return; 
    }

    auto vkTex = dynamic_cast<VulkanTexture*>(desc.renderTarget);
    if (!vkTex) return;

    // rpInfo.renderPass = ...; // 需要从接口获取 RenderPass 句柄
    // rpInfo.framebuffer = ...;
    rpInfo.renderArea.offset = { desc.renderArea.x, desc.renderArea.y };
    rpInfo.renderArea.extent = { static_cast<uint32_t>(desc.renderArea.width), static_cast<uint32_t>(desc.renderArea.height) };

    VkClearValue clearColor = {{{ desc.clearColor.r, desc.clearColor.g, desc.clearColor.b, desc.clearColor.a }}};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(m_cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::SetPipelineState(IPipelineState* pipelineState) {
    auto vkPipeline = dynamic_cast<VulkanPipelineState*>(pipelineState);
    if (vkPipeline) {
        vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetVkPipeline());
        m_currentLayout = vkPipeline->GetVkPipelineLayout();
    }
}

void VulkanCommandBuffer::SetViewport(const Viewport& viewport) {
    VkViewport v{};
    v.x = viewport.x;
    v.y = viewport.y;
    v.width = viewport.width;
    v.height = viewport.height;
    v.minDepth = viewport.minDepth;
    v.maxDepth = viewport.maxDepth;
    vkCmdSetViewport(m_cmd, 0, 1, &v);
}

void VulkanCommandBuffer::SetScissorRect(const Rect& rect) {
    VkRect2D s{};
    s.offset = { rect.x, rect.y };
    s.extent = { static_cast<uint32_t>(rect.width), static_cast<uint32_t>(rect.height) };
    vkCmdSetScissor(m_cmd, 0, 1, &s);
}

void VulkanCommandBuffer::SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset) {
    auto vkBuf = dynamic_cast<VulkanBuffer*>(buffer);
    if (vkBuf) {
        VkBuffer buf = vkBuf->GetVkBuffer();
        VkDeviceSize off = offset;
        vkCmdBindVertexBuffers(m_cmd, slot, 1, &buf, &off);
    }
}

void VulkanCommandBuffer::SetIndexBuffer(IBuffer* buffer, bool is32Bit, uint32_t offset) {
    auto vkBuf = dynamic_cast<VulkanBuffer*>(buffer);
    if (vkBuf) {
        vkCmdBindIndexBuffer(m_cmd, vkBuf->GetVkBuffer(), offset, is32Bit ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
    }
}

void VulkanCommandBuffer::BindDescriptorSet(uint32_t set, IDescriptorSet* descriptorSet) {
    if (!m_currentLayout || !descriptorSet) return;
    VkDescriptorSet ds = static_cast<VkDescriptorSet>(descriptorSet->GetNativeHandle());
    vkCmdBindDescriptorSets(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_currentLayout, set, 1, &ds, 0, nullptr);
}

void VulkanCommandBuffer::PushConstants(ShaderType stage, const void* data, uint32_t size) {
    if (!m_currentLayout) return;
    VkShaderStageFlags flag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    vkCmdPushConstants(m_cmd, m_currentLayout, flag, 0, size, data);
}

void VulkanCommandBuffer::DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset) {
    auto vkBuf = dynamic_cast<VulkanBuffer*>(indirectBuffer);
    if (vkBuf) {
        vkCmdDrawIndexedIndirect(m_cmd, vkBuf->GetVkBuffer(), offset, 1, sizeof(VkDrawIndexedIndirectCommand));
    }
}

} // namespace Prisma::Graphic::Vulkan
