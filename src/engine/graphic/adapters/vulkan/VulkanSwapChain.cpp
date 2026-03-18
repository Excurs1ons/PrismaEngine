#include "VulkanSwapChain.h"
#include "RenderDeviceVulkan.h"
#include <VkBootstrap.h>
#include <vector>
#include "Logger.h"

namespace Prisma::Graphic::Vulkan {

VulkanSwapChain::VulkanSwapChain(RenderDeviceVulkan* device)
    : m_device(device) {
}

VulkanSwapChain::~VulkanSwapChain() {
    Cleanup();
}

int VulkanSwapChain::Initialize(void* windowHandle, uint32_t width, uint32_t height, bool vsync) {
    // 在重新初始化（如窗口缩放）前等待 GPU 空闲，避免销毁正在被 CommandBuffer 引用的旧资源。
    if (m_device->GetVkDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device->GetVkDevice());
    }
    Cleanup();

    LOG_INFO("Vulkan", "Initializing SwapChain: {0}x{1}, vsync: {2}", width, height, vsync);

    // 修正：按照 (VkPhysicalDevice, VkDevice, VkSurfaceKHR) 的顺序传递
    vkb::SwapchainBuilder swapchain_builder{m_device->GetPhysicalDevice(), m_device->GetVkDevice(), (VkSurfaceKHR)windowHandle};
    auto vkb_swap_ret = swapchain_builder
        .set_desired_extent(width, height)
        .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
        .build();

    if (!vkb_swap_ret) {
        LOG_ERROR("Vulkan", "Failed to create swapchain: {0}", vkb_swap_ret.error().message());
        return -1;
    }

    vkb::Swapchain vkb_swapchain = vkb_swap_ret.value();
    m_swapchain = vkb_swapchain.swapchain;
    m_images = vkb_swapchain.get_images().value();
    m_imageViews = vkb_swapchain.get_image_views().value();
    m_format = vkb_swapchain.image_format;
    m_extent = vkb_swapchain.extent;

    // Create RenderPass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device->GetVkDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        LOG_ERROR("Vulkan", "Failed to create render pass!");
        return -2;
    }

    // Create Framebuffers
    m_framebuffers.resize(m_imageViews.size());
    for (size_t i = 0; i < m_imageViews.size(); i++) {
        VkImageView attachments[] = { m_imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_extent.width;
        framebufferInfo.height = m_extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device->GetVkDevice(), &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            LOG_ERROR("Vulkan", "Failed to create framebuffer!");
            return -3;
        }
    }

    return 0;
}

void VulkanSwapChain::Cleanup() {
    VkDevice device = m_device->GetVkDevice();
    if (device == VK_NULL_HANDLE) return;

    for (auto fb : m_framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    for (auto view : m_imageViews) vkDestroyImageView(device, view, nullptr);
    if (m_renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(device, m_renderPass, nullptr);
    if (m_swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, m_swapchain, nullptr);

    m_framebuffers.clear();
    m_imageViews.clear();
    m_images.clear();
    m_swapchain = VK_NULL_HANDLE;
    m_renderPass = VK_NULL_HANDLE;
}

bool VulkanSwapChain::AcquireNextImage(VkSemaphore semaphore, VkFence fence) {
    VkResult result = vkAcquireNextImageKHR(m_device->GetVkDevice(), m_swapchain, UINT64_MAX, semaphore, fence, &m_currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    }
    return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
}

bool VulkanSwapChain::Present(VkSemaphore waitSemaphore) {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &m_currentImageIndex;

    VkResult result = vkQueuePresentKHR(m_device->GetGraphicsQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;
    }
    return result == VK_SUCCESS;
}

bool VulkanSwapChain::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return true;
    return Initialize(m_device->GetVkSurface(), width, height, true) == 0; // 暂时写死 vsync
}

}
