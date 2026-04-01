#include "VulkanSwapChain.h"
#include "RenderDeviceVulkan.h"
#include <VkBootstrap.h>
#include <vector>
#include <fstream>
#include <limits>
#include <chrono>
#include "Logger.h"

namespace Prisma::Graphic::Vulkan {

namespace {

class SwapChainRenderTarget final : public ITexture {
public:
    SwapChainRenderTarget(VkImage image, VkImageView imageView, TextureFormat format, uint32_t width, uint32_t height)
        : m_image(image), m_imageView(imageView), m_format(format), m_width(width), m_height(height) {}

    ResourceType GetType() const override { return ResourceType::Texture; }
    TextureType GetTextureType() const override { return TextureType::Texture2D; }
    TextureFormat GetFormat() const override { return m_format; }
    float GetWidth() const override { return static_cast<float>(m_width); }
    float GetHeight() const override { return static_cast<float>(m_height); }
    uint32_t GetDepth() const override { return 1; }
    uint32_t GetMipLevels() const override { return 1; }
    uint32_t GetArraySize() const override { return 1; }
    uint32_t GetSampleCount() const override { return 1; }
    uint32_t GetSampleQuality() const override { return 0; }
    bool IsRenderTarget() const override { return true; }
    bool IsDepthStencil() const override { return false; }
    bool IsShaderResource() const override { return false; }
    bool IsUnorderedAccess() const override { return false; }
    uint64_t GetBytesPerPixel() const override { return 4; }
    uint64_t GetSubresourceSize(uint32_t) const override { return static_cast<uint64_t>(m_width) * m_height * 4; }
    TextureMapDesc Map(uint32_t = 0, uint32_t = 0, uint32_t = 0) override { return {}; }
    void Unmap(uint32_t = 0, uint32_t = 0) override {}
    void UpdateData(const void*, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t) override {}
    void GenerateMips() override {}
    void CopyFrom(ITexture*, uint32_t, uint32_t, uint32_t, uint32_t) override {}
    bool ReadData(uint32_t, uint32_t, void*, uint64_t) override { return false; }
    uint64_t CreateDescriptor(TextureDescriptorType, TextureFormat = TextureFormat::Unknown, uint32_t = 0, uint32_t = 0) override {
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_imageView));
    }
    uint64_t GetDefaultSRV() const override { return 0; }
    uint64_t GetDefaultRTV() const override { return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_imageView)); }
    uint64_t GetDefaultDSV() const override { return 0; }
    uint64_t GetDefaultUAV() const override { return 0; }
    void Clear(const Color&, uint32_t = 0, uint32_t = 0) override {}
    void ClearDepthStencil(float = 1.0f, uint8_t = 0) override {}
    void ResolveMultisampled(ITexture*, TextureFormat = TextureFormat::Unknown) override {}
    void Discard(uint32_t = 0, uint32_t = 0) override {}
    void Compact() override {}
    uint64_t GetMemoryUsage() const override { return GetSubresourceSize(0); }
    bool DebugSaveToFile(const std::string&, uint32_t = 0, uint32_t = 0) override { return false; }
    bool Validate() override { return m_image != VK_NULL_HANDLE && m_imageView != VK_NULL_HANDLE; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    TextureFormat m_format = TextureFormat::RGBA8_UNorm;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

TextureFormat ToTextureFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_B8G8R8A8_UNORM:
            return TextureFormat::BGRA8_UNorm;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return TextureFormat::BGRA8_UNorm_sRGB;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return TextureFormat::RGBA8_UNorm_sRGB;
        case VK_FORMAT_R8G8B8A8_UNORM:
        default:
            return TextureFormat::RGBA8_UNorm;
    }
}

}  // namespace
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

    LOG_INFO("Vulkan", "正在初始化交换链: {0}x{1}, 垂直同步: {2}", width, height, vsync);

    // 修正：按照 (VkPhysicalDevice, VkDevice, VkSurfaceKHR) 的顺序传递
    vkb::SwapchainBuilder swapchain_builder{m_device->GetPhysicalDevice(), m_device->GetVkDevice(), (VkSurfaceKHR)windowHandle};
    auto vkb_swap_ret = swapchain_builder
        .set_desired_extent(width, height)
        .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
        .build();

    if (!vkb_swap_ret) {
        LOG_ERROR("Vulkan", "创建交换链失败: {0}", vkb_swap_ret.error().message());
        return -1;
    }

    vkb::Swapchain vkb_swapchain = vkb_swap_ret.value();
    m_swapchain = vkb_swapchain.swapchain;
    m_images = vkb_swapchain.get_images().value();
    m_imageViews = vkb_swapchain.get_image_views().value();
    m_format = vkb_swapchain.image_format;
    m_extent = vkb_swapchain.extent;
    m_mode = vsync ? SwapChainMode::VSync : SwapChainMode::Immediate;
    m_hdrEnabled = false;
    m_renderTargets.clear();

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
        LOG_ERROR("Vulkan", "创建渲染通道失败！");
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
            LOG_ERROR("Vulkan", "创建帧缓冲区失败！");
            return -3;
        }

        m_renderTargets.emplace_back(std::make_unique<SwapChainRenderTarget>(
            m_images[i], m_imageViews[i], ToTextureFormat(m_format), m_extent.width, m_extent.height));
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
    m_renderTargets.clear();
    m_swapchain = VK_NULL_HANDLE;
    m_renderPass = VK_NULL_HANDLE;
}

ITexture* VulkanSwapChain::GetRenderTarget(uint32_t bufferIndex) {
    if (bufferIndex >= m_renderTargets.size()) {
        return nullptr;
    }
    return m_renderTargets[bufferIndex].get();
}

ITexture* VulkanSwapChain::GetCurrentRenderTarget() {
    return GetRenderTarget(m_currentImageIndex);
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
    const auto now = std::chrono::steady_clock::now();
    if (m_lastPresentTime.time_since_epoch().count() != 0) {
        const float frameTimeMs = std::chrono::duration<float, std::milli>(now - m_lastPresentTime).count();
        m_presentStats.totalFrames += 1;
        m_presentStats.executionTime = frameTimeMs;
        m_presentStats.minFrameTime = std::min(m_presentStats.minFrameTime, frameTimeMs);
        m_presentStats.maxFrameTime = std::max(m_presentStats.maxFrameTime, frameTimeMs);
        m_presentStats.averageFrameTime =
            m_presentStats.totalFrames == 0
                ? frameTimeMs
                : ((m_presentStats.averageFrameTime * static_cast<float>(m_presentStats.totalFrames - 1)) + frameTimeMs) /
                      static_cast<float>(m_presentStats.totalFrames);
        m_presentStats.frameRate = frameTimeMs > 0.0f ? 1000.0f / frameTimeMs : 0.0f;
    } else {
        m_presentStats.totalFrames = 1;
        m_presentStats.executionTime = 0.0f;
        m_presentStats.averageFrameTime = 0.0f;
        m_presentStats.minFrameTime = 0.0f;
        m_presentStats.maxFrameTime = 0.0f;
        m_presentStats.frameRate = 0.0f;
    }
    m_lastPresentTime = now;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_presentStats.droppedFrames += 1;
        return false;
    }
    return result == VK_SUCCESS;
}

bool VulkanSwapChain::Present() {
    return Present(VK_NULL_HANDLE);
}

bool VulkanSwapChain::SetMode(SwapChainMode mode) {
    m_mode = mode;
    return true;
}

bool VulkanSwapChain::SetHDR(bool enable) {
    m_hdrEnabled = enable;
    return true;
}

bool VulkanSwapChain::SetColorSpace(const char* colorSpace) {
    if (!colorSpace || colorSpace[0] == '\0') {
        return false;
    }
    m_colorSpace = colorSpace;
    return true;
}

void VulkanSwapChain::ResetStats() {
    m_presentStats = {};
    m_presentStats.minFrameTime = std::numeric_limits<float>::max();
    m_lastPresentTime = {};
}

bool VulkanSwapChain::SetFullscreen(bool fullscreen) {
    m_fullscreen = fullscreen;
    return true;
}

bool VulkanSwapChain::Screenshot(const std::string& filename, uint32_t bufferIndex) {
    std::ofstream stream(filename, std::ios::binary);
    if (!stream.is_open()) {
        return false;
    }

    stream << "Prisma Vulkan swapchain screenshot placeholder\n";
    stream << "buffer=" << bufferIndex << "\n";
    stream << "size=" << m_extent.width << "x" << m_extent.height << "\n";
    stream << "format=" << static_cast<int>(ToTextureFormat(m_format)) << "\n";
    return stream.good();
}

bool VulkanSwapChain::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return true;
    return Initialize(m_device->GetVkSurface(), width, height, true) == 0; // 暂时写死 vsync
}

} // namespace Prisma::Graphic::Vulkan
