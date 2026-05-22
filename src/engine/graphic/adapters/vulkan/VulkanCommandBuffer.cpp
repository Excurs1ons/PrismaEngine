#include "VulkanCommandBuffer.h"
#include "VulkanResources.h"
#include "VulkanPipelineState.h"
#include "VulkanComputePipeline.h"
#include "VulkanSwapChain.h"
#include "VulkanShader.h"
#include <algorithm>
#include <unordered_map>
#include <array>

namespace Prisma::Graphic::Vulkan {

// ============================================================================
// 离屏渲染目标缓存 (Offscreen Render Target Cache)
//
// 用途:
//   当 VulkanCommandBuffer 接收到离屏纹理（非交换链）作为 BeginRenderPass
//   的渲染目标时，需要为它创建 VkRenderPass 和 VkFramebuffer。
//   这些对象按纹理的 VkImageView 缓存，避免每帧重复创建。
//
// 生命周期:
//   缓存项在纹理被首次用作渲染目标时创建，在 ReleaseAllOffscreenResources()
//   或 ReleaseOffscreenResources(VkImageView) 调用时销毁。
// ============================================================================
namespace {

struct OffscreenRT {
    VkRenderPass  renderPass  = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkDevice      device      = VK_NULL_HANDLE;
};

std::unordered_map<VkImageView, OffscreenRT> s_offscreenRTs;

OffscreenRT CreateOffscreenRT(VkDevice device, VkImageView imageView,
                               VkFormat format, uint32_t width, uint32_t height,
                               bool clearRT)
{
    OffscreenRT rt{};
    rt.device = device;

    // --- 1. 创建 RenderPass ---
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = format;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = clearRT ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                             : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // initialLayout = UNDEFINED → render pass 会丢弃原有内容（LOAD_OP_CLEAR 已足够）
    // 同时允许从任何当前 layout 自动转换，无需外部 barrier
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // finalLayout = SHADER_READ_ONLY_OPTIMAL → 渲染完成后可供后续 Pass 采样
    colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorRef;

    // 依赖 1: 外部 → Subpass 0（等待上一帧的着色器读取完成）
    // 依赖 2: Subpass 0 → 外部（使颜色附件写入对后续着色器读取可见）
    std::array<VkSubpassDependency, 2> deps{};
    deps[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass      = 0;
    deps[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    deps[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[0].dependencyFlags = 0;

    deps[1].srcSubpass      = 0;
    deps[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    deps[1].dependencyFlags = 0;

    VkRenderPassCreateInfo rpCI{};
    rpCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpCI.attachmentCount = 1;
    rpCI.pAttachments    = &colorAttachment;
    rpCI.subpassCount    = 1;
    rpCI.pSubpasses      = &subpass;
    rpCI.dependencyCount = static_cast<uint32_t>(deps.size());
    rpCI.pDependencies   = deps.data();

    if (vkCreateRenderPass(device, &rpCI, nullptr, &rt.renderPass) != VK_SUCCESS) {
        return {}; // 失败返回空结构
    }

    // --- 2. 创建 Framebuffer ---
    VkFramebufferCreateInfo fbCI{};
    fbCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCI.renderPass      = rt.renderPass;
    fbCI.attachmentCount = 1;
    fbCI.pAttachments    = &imageView;
    fbCI.width           = width;
    fbCI.height          = height;
    fbCI.layers          = 1;

    if (vkCreateFramebuffer(device, &fbCI, nullptr, &rt.framebuffer) != VK_SUCCESS) {
        vkDestroyRenderPass(device, rt.renderPass, nullptr);
        return {};
    }

    return rt;
}

} // anonymous namespace

// ============================================================================
// 公开的缓存清理接口
// ============================================================================

void VulkanCommandBuffer::ReleaseOffscreenResources(VkImageView imageView) {
    auto it = s_offscreenRTs.find(imageView);
    if (it != s_offscreenRTs.end()) {
        vkDestroyFramebuffer(it->second.device, it->second.framebuffer, nullptr);
        vkDestroyRenderPass(it->second.device, it->second.renderPass, nullptr);
        s_offscreenRTs.erase(it);
    }
}

void VulkanCommandBuffer::ReleaseAllOffscreenResources() {
    for (auto& [key, rt] : s_offscreenRTs) {
        vkDestroyFramebuffer(rt.device, rt.framebuffer, nullptr);
        vkDestroyRenderPass(rt.device, rt.renderPass, nullptr);
    }
    s_offscreenRTs.clear();
}

// ============================================================================
// BeginRenderPass — 支持离屏渲染目标
// ============================================================================

void VulkanCommandBuffer::BeginRenderPass(const RenderPassDesc& desc) {
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

    // 未指定渲染目标 → 忽略（假设外部已开启默认交换链 RP）
    if (!desc.renderTarget) {
        return;
    }

    auto vkTex = dynamic_cast<VulkanTexture*>(desc.renderTarget);
    if (!vkTex) return;

    VkImageView imageView = vkTex->GetVkImageView();
    VkDevice    device    = vkTex->GetVkDevice();
    if (!imageView || !device) return;

    // 从缓存查找或创建离屏 RP/FB
    auto it = s_offscreenRTs.find(imageView);
    if (it == s_offscreenRTs.end()) {
        OffscreenRT rt = CreateOffscreenRT(
            device, imageView,
            vkTex->GetVkFormat(),
            static_cast<uint32_t>(vkTex->GetWidth()),
            static_cast<uint32_t>(vkTex->GetHeight()),
            desc.clearRenderTarget
        );
        if (!rt.renderPass || !rt.framebuffer) {
            return; // 创建失败，跳过（不崩溃）
        }
        it = s_offscreenRTs.emplace(imageView, rt).first;
    }

    rpInfo.renderPass  = it->second.renderPass;
    rpInfo.framebuffer = it->second.framebuffer;

    // 渲染区域
    if (desc.renderArea.width > 0 && desc.renderArea.height > 0) {
        rpInfo.renderArea.offset = { desc.renderArea.x, desc.renderArea.y };
        rpInfo.renderArea.extent = {
            static_cast<uint32_t>(desc.renderArea.width),
            static_cast<uint32_t>(desc.renderArea.height)
        };
    } else {
        rpInfo.renderArea.offset = { 0, 0 };
        rpInfo.renderArea.extent = {
            static_cast<uint32_t>(vkTex->GetWidth()),
            static_cast<uint32_t>(vkTex->GetHeight())
        };
    }

    // 清除值
    VkClearValue clearColor = {{
        desc.clearColor.r,
        desc.clearColor.g,
        desc.clearColor.b,
        desc.clearColor.a
    }};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(m_cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::SetPipelineState(IPipelineState* pipelineState) {
    auto vkPipeline = dynamic_cast<VulkanPipelineState*>(pipelineState);
    if (vkPipeline) {
        vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetVkPipeline());
        m_currentLayout = vkPipeline->GetVkPipelineLayout();
        m_currentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
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
    vkCmdBindDescriptorSets(m_cmd, m_currentBindPoint, m_currentLayout, set, 1, &ds, 0, nullptr);
}

void VulkanCommandBuffer::PushConstants([[maybe_unused]] ShaderType stage, const void* data, uint32_t size) {
    if (!m_currentLayout) return;
    VkShaderStageFlags flag;
    if (m_currentBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
        flag = VK_SHADER_STAGE_COMPUTE_BIT;
    } else {
        switch (stage) {
            case ShaderType::Vertex:   flag = VK_SHADER_STAGE_VERTEX_BIT; break;
            case ShaderType::Pixel:    flag = VK_SHADER_STAGE_FRAGMENT_BIT; break;
            case ShaderType::Compute:  flag = VK_SHADER_STAGE_COMPUTE_BIT; break;
            case ShaderType::Geometry: flag = VK_SHADER_STAGE_GEOMETRY_BIT; break;
            case ShaderType::Hull:     flag = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; break;
            case ShaderType::Domain:   flag = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; break;
            default:                   flag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; break;
        }
    }
    vkCmdPushConstants(m_cmd, m_currentLayout, flag, 0, size, data);
}

void VulkanCommandBuffer::DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset) {
    auto vkBuf = dynamic_cast<VulkanBuffer*>(indirectBuffer);
    if (vkBuf) {
        vkCmdDrawIndexedIndirect(m_cmd, vkBuf->GetVkBuffer(), offset, 1, sizeof(VkDrawIndexedIndirectCommand));
    }
}

void VulkanCommandBuffer::SetComputePipeline(IComputePipeline* pipeline) {
    auto* vkPipeline = dynamic_cast<VulkanComputePipeline*>(pipeline);
    if (vkPipeline) {
        vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->GetVkPipeline());
        m_currentLayout = vkPipeline->GetVkPipelineLayout();
        m_currentBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
}

// === PipelineBarrier with explicit image barriers ===

static VkImageLayout ResourceStateToVkLayout(ResourceState state) {
    switch (state) {
        case ResourceState::Undefined:      return VK_IMAGE_LAYOUT_UNDEFINED;
        case ResourceState::Common:         return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceState::ShaderRead:     return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ResourceState::UnorderedAccess:return VK_IMAGE_LAYOUT_GENERAL;
        case ResourceState::CopySrc:        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case ResourceState::CopyDst:        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case ResourceState::RenderTarget:   return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ResourceState::DepthStencil:   return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ResourceState::Present:        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        default:                            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void VulkanCommandBuffer::PipelineBarrier(const std::vector<ImageBarrier>& imageBarriers) {
    if (imageBarriers.empty()) {
        PipelineBarrier();
        return;
    }

    std::vector<VkImageMemoryBarrier> barriers;
    barriers.reserve(imageBarriers.size());

    for (const auto& ib : imageBarriers) {
        auto* vkTex = dynamic_cast<VulkanTexture*>(ib.texture);
        if (!vkTex) continue;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = ResourceStateToVkLayout(ib.oldState);
        barrier.newLayout = ResourceStateToVkLayout(ib.newState);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vkTex->GetVkImage();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = ib.mipLevel;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = ib.arraySlice;
        barrier.subresourceRange.layerCount = 1;

        // Map resource states to access masks and pipeline stages
        VkAccessFlags srcAccess = 0, dstAccess = 0;
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        // oldState -> srcAccess + srcStage
        switch (ib.oldState) {
            case ResourceState::Undefined:
                srcAccess = 0;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;
            case ResourceState::ShaderRead:
                srcAccess = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                         | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;
            case ResourceState::UnorderedAccess:
                srcAccess = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;
            case ResourceState::CopySrc:
                srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ResourceState::CopyDst:
                srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ResourceState::RenderTarget:
                srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case ResourceState::Present:
                srcAccess = 0;
                srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            default:
                break;
        }

        // newState -> dstAccess + dstStage
        switch (ib.newState) {
            case ResourceState::ShaderRead:
                dstAccess = VK_ACCESS_SHADER_READ_BIT;
                dstStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                         | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;
            case ResourceState::UnorderedAccess:
                dstAccess = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
                dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;
            case ResourceState::CopySrc:
                dstAccess = VK_ACCESS_TRANSFER_READ_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ResourceState::CopyDst:
                dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ResourceState::RenderTarget:
                dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case ResourceState::Present:
                dstAccess = 0;
                dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            default:
                break;
        }

        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;

        barriers.push_back(barrier);
    }

    if (!barriers.empty()) {
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        vkCmdPipelineBarrier(m_cmd, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
    }
}

} // namespace Prisma::Graphic::Vulkan
