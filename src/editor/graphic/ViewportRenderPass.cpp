#include "ViewportRenderPass.h"
#include <algorithm>

namespace Prisma::Graphic::Vulkan {

ViewportRenderPass::ViewportRenderPass() = default;

ViewportRenderPass::~ViewportRenderPass() {
    Shutdown();
}

int ViewportRenderPass::Initialize(VkDevice device, VkImageView colorView,
                                 VkImageView depthView, uint32_t width, uint32_t height) {
    if (!device || !colorView || !depthView || width == 0 || height == 0) {
        return -1;
    }

    // 先清理旧的资源
    Shutdown();

    m_device   = device;
    m_colorView = colorView;
    m_depthView = depthView;
    m_width    = width;
    m_height   = height;

    // 创建 RenderPass
    int result = CreateRenderPass();
    if (result != 0) {
        Shutdown();
        return result;
    }

    // 创建 Framebuffer
    result = CreateFramebuffer();
    if (result != 0) {
        Shutdown();
        return result;
    }

    m_initialized = true;
    return 0;
}

void ViewportRenderPass::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_device) {
        DestroyFramebuffer();

        if (m_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_device, m_renderPass, nullptr);
            m_renderPass = VK_NULL_HANDLE;
        }
    }

    m_device     = VK_NULL_HANDLE;
    m_colorView  = VK_NULL_HANDLE;
    m_depthView  = VK_NULL_HANDLE;
    m_width      = 0;
    m_height     = 0;
    m_initialized = false;
}

void ViewportRenderPass::Begin(VkCommandBuffer cmd) {
    if (!m_initialized || cmd == VK_NULL_HANDLE) {
        return;
    }

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass        = m_renderPass;
    rpInfo.framebuffer       = m_framebuffer;
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = {m_width, m_height};

    // 设置清除值：颜色 + 深度
    VkClearValue clearValues[2] = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};  // 颜色：黑色
    clearValues[1].depthStencil = {1.0f, 0};                 // 深度：1.0f

    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues    = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 设置视口和裁剪矩形
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(m_width);
    viewport.height   = static_cast<float>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void ViewportRenderPass::End(VkCommandBuffer cmd) {
    if (!m_initialized || cmd == VK_NULL_HANDLE) {
        return;
    }

    vkCmdEndRenderPass(cmd);
}

void ViewportRenderPass::Resize(uint32_t width, uint32_t height) {
    if (width == m_width && height == m_height) {
        return;
    }

    m_width  = width;
    m_height = height;

    // 重建 Framebuffer
    DestroyFramebuffer();
    CreateFramebuffer();
}

int ViewportRenderPass::CreateRenderPass() {
    if (m_device == VK_NULL_HANDLE) {
        return -1;
    }

    // 定义附件描述
    VkAttachmentDescription attachments[2] = {};

    // 颜色附件
    attachments[0].format         = VK_FORMAT_R8G8B8A8_UNORM;  // 纹理格式
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;     // 清除
    attachments[0].storeOp       = VK_ATTACHMENT_STORE_OP_STORE;    // 保存（供 ImGui 采样）
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // 供 ImGui 读取

    // 深度附件
    attachments[1].format         = VK_FORMAT_D32_SFLOAT;             // 深度格式
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;       // 清除
    attachments[1].storeOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 不需要保存
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 子通道描述
    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    // 子通道依赖（确保布局转换正确）
    VkSubpassDependency dependency{};
    dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass      = 0;
    dependency.srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask  = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstAccessMask  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // 创建 RenderPass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount  = 2;
    renderPassInfo.pAttachments     = attachments;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
    return (result == VK_SUCCESS) ? 0 : -1;
}

int ViewportRenderPass::CreateFramebuffer() {
    if (m_device == VK_NULL_HANDLE || m_renderPass == VK_NULL_HANDLE ||
        m_colorView == VK_NULL_HANDLE || m_depthView == VK_NULL_HANDLE) {
        return -1;
    }

    // 附件列表
    VkImageView imageViews[2] = {m_colorView, m_depthView};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = m_renderPass;
    framebufferInfo.attachmentCount  = 2;
    framebufferInfo.pAttachments     = imageViews;
    framebufferInfo.width           = m_width;
    framebufferInfo.height          = m_height;
    framebufferInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffer);
    return (result == VK_SUCCESS) ? 0 : -1;
}

void ViewportRenderPass::DestroyFramebuffer() {
    if (m_device != VK_NULL_HANDLE && m_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
        m_framebuffer = VK_NULL_HANDLE;
    }
}

} // namespace Prisma::Graphic::Vulkan
