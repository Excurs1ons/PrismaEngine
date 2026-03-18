#pragma once

#include "interfaces/ISwapChain.h"
#include <vulkan/vulkan.h>
#include <Logger.h>

namespace Prisma::Graphic::Vulkan {

class RenderDeviceVulkan;

class VulkanSwapChain : public ISwapChain {
public:
    VulkanSwapChain(RenderDeviceVulkan* device);
    ~VulkanSwapChain() override;

    int Initialize(void* windowHandle, uint32_t width, uint32_t height, bool vsync);
    void Cleanup();

    uint32_t GetBufferCount() const override { return (uint32_t)m_images.size(); }
    uint32_t GetCurrentBufferIndex() const override { return m_currentImageIndex; }
    uint32_t GetWidth() const override { return m_extent.width; }
    uint32_t GetHeight() const override { return m_extent.height; }
    TextureFormat GetFormat() const override { return TextureFormat::RGBA8_UNorm; }
    SwapChainMode GetMode() const override { return SwapChainMode::VSync; }
    bool IsHDR() const override { return false; }

    ITexture* GetRenderTarget(uint32_t /*bufferIndex*/ = 0) override { return nullptr; }
    ITexture* GetCurrentRenderTarget() override { return nullptr; }

    // 获取用于 ImGui 的 RenderPass
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    VkFramebuffer GetCurrentFramebuffer() const { return m_framebuffers[m_currentImageIndex]; }
    VkExtent2D GetExtent() const { return m_extent; }

    bool AcquireNextImage(VkSemaphore semaphore, VkFence fence);
    bool Present(VkSemaphore waitSemaphore);

    bool Present() override { return false; } // 使用带信号量的版本
    bool SetMode(SwapChainMode /*mode*/) override { return true; }
    bool Resize(uint32_t width, uint32_t height) override;
    bool SetHDR(bool /*enable*/) override { return true; }

    const char* GetColorSpace() const override { return "sRGB"; }
    bool SetColorSpace(const char* /*colorSpace*/) override { return true; }

    float GetFrameRate() const override { return 60.0f; }
    float GetFrameTime() const override { return 16.6f; }
    PresentStats GetPresentStats() const override { return {}; }
    void ResetStats() override {}

    bool IsFullscreen() const override { return false; }
    bool SetFullscreen(bool /*fullscreen*/) override { return true; }

    bool Screenshot(const std::string& /*filename*/, uint32_t /*bufferIndex*/ = 0) override { return true; }
    void EnableDebugLayer(bool /*enable*/) override {}

private:
    RenderDeviceVulkan* m_device;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format;
    VkExtent2D m_extent;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    uint32_t m_currentImageIndex = 0;
};

} // namespace Prisma::Graphic::Vulkan