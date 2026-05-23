#pragma once

#include "interfaces/ISwapChain.h"
#include <vulkan/vulkan.h>
#include <Logger.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Graphic::Vulkan {

class RenderDeviceVulkan;

class VulkanSwapChain : public ISwapChain {
public:
    VulkanSwapChain(RenderDeviceVulkan* device);
    ~VulkanSwapChain() override;

    int Initialize(void* windowHandle, uint32_t width, uint32_t height, PresentMode presentMode);
    void Cleanup();

    uint32_t GetBufferCount() const override { return (uint32_t)m_images.size(); }
    uint32_t GetCurrentBufferIndex() const override { return m_currentImageIndex; }
    uint32_t GetWidth() const override { return m_extent.width; }
    uint32_t GetHeight() const override { return m_extent.height; }
    TextureFormat GetFormat() const override { return TextureFormat::RGBA8_UNorm; }
    PresentMode GetMode() const override { return m_mode; }
    bool IsHDR() const override { return m_hdrEnabled; }

    ITexture* GetRenderTarget(uint32_t bufferIndex = 0) override;
    ITexture* GetCurrentRenderTarget() override;

    // 获取用于 ImGui 的 RenderPass
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    VkFramebuffer GetCurrentFramebuffer() const { return m_framebuffers[m_currentImageIndex]; }
    VkExtent2D GetExtent() const { return m_extent; }

    bool AcquireNextImage(VkSemaphore semaphore, VkFence fence);
    bool Present(VkSemaphore waitSemaphore);

    bool Present() override;
    bool SetMode(PresentMode mode) override;
    bool Resize(uint32_t width, uint32_t height) override;
    bool SetHDR(bool enable) override;

    const char* GetColorSpace() const override { return m_colorSpace.c_str(); }
    bool SetColorSpace(const char* colorSpace) override;

    float GetFrameRate() const override { return m_presentStats.frameRate; }
    float GetFrameTime() const override { return m_presentStats.executionTime; }
    PresentStats GetPresentStats() const override { return m_presentStats; }
    void ResetStats() override;

    bool IsFullscreen() const override { return m_fullscreen; }
    bool SetFullscreen(bool fullscreen) override;

    bool Screenshot(const std::string& filename, uint32_t bufferIndex = 0) override;
    void EnableDebugLayer(bool enable) override { m_debugLayerEnabled = enable; }

    const std::vector<VkImage>& GetImages() const { return m_images; }

private:
    RenderDeviceVulkan* m_device;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format;
    VkExtent2D m_extent;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<std::unique_ptr<ITexture>> m_renderTargets;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;

    // 深度缓冲
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;

    uint32_t m_currentImageIndex = 0;
    PresentMode m_mode = PresentMode::VSync;
    bool m_hdrEnabled = false;
    bool m_fullscreen = false;
    bool m_debugLayerEnabled = false;
    PresentStats m_presentStats{};
    std::chrono::steady_clock::time_point m_lastPresentTime{};
    std::string m_colorSpace = "sRGB";
};

} // namespace Prisma::Graphic::Vulkan
