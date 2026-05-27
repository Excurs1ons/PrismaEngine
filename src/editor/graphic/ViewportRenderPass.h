#pragma once

#include <vulkan/vulkan.h>
#include <memory>

namespace Prisma::Graphic::Vulkan {

/**
 * Viewport 专用 RenderPass
 *
 * 用于将场景渲染到离屏纹理（Viewport Texture），而不是直接渲染到 SwapChain。
 * 这样 ImGui 可以在 SwapChain 上显示 UI，而 Viewport 窗口显示离屏渲染的场景。
 */
class ViewportRenderPass {
public:
    ViewportRenderPass();
    ~ViewportRenderPass();

    /**
     * 初始化 Viewport RenderPass
     */
    int Initialize(VkDevice device, VkImageView colorView, VkImageView depthView,
                 uint32_t width, uint32_t height);

    // 关闭并释放资源
    void Shutdown();

    /* 开始 Viewport RenderPass */
    void Begin(VkCommandBuffer cmd);

    /* 结束 Viewport RenderPass */
    void End(VkCommandBuffer cmd);

    // 获取 RenderPass 句柄
    VkRenderPass GetRenderPass() const { return m_renderPass; }

    // 检查是否已初始化
    bool IsInitialized() const { return m_initialized; }

    // 更新尺寸（当 Viewport 大小改变时调用）
    void Resize(uint32_t width, uint32_t height);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    VkImageView m_colorView = VK_NULL_HANDLE;
    VkImageView m_depthView = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_initialized = false;

    // 创建 RenderPass
    int CreateRenderPass();

    // 创建 Framebuffer
    int CreateFramebuffer();

    // 销毁 Framebuffer
    void DestroyFramebuffer();
};

} // namespace Prisma::Graphic::Vulkan
