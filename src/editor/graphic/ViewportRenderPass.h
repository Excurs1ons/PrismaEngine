#pragma once

#include <vulkan/vulkan.h>
#include <memory>

namespace Prisma::Graphic::Vulkan {

/**
 * @brief Viewport 专用 RenderPass
 *
 * 用于将场景渲染到离屏纹理（Viewport Texture），而不是直接渲染到 SwapChain。
 * 这样 ImGui 可以在 SwapChain 上显示 UI，而 Viewport 窗口显示离屏渲染的场景。
 */
class ViewportRenderPass {
public:
    ViewportRenderPass();
    ~ViewportRenderPass();

    /**
     * @brief 初始化 Viewport RenderPass
     * @param device Vulkan 设备
     * @param colorView 颜色附件的 ImageView（Viewport Texture）
     * @param depthView 深度附件的 ImageView
     * @param width 视口宽度
     * @param height 视口高度
     * @return 0 成功，非 0 失败
     */
    int Initialize(VkDevice device, VkImageView colorView, VkImageView depthView,
                 uint32_t width, uint32_t height);

    /**
     * @brief 关闭并释放资源
     */
    void Shutdown();

    /**
     * @brief 开始 Viewport RenderPass
     * @param cmd 命令缓冲区
     */
    void Begin(VkCommandBuffer cmd);

    /**
     * @brief 结束 Viewport RenderPass
     * @param cmd 命令缓冲区
     */
    void End(VkCommandBuffer cmd);

    /**
     * @brief 获取 RenderPass 句柄
     */
    VkRenderPass GetRenderPass() const { return m_renderPass; }

    /**
     * @brief 检查是否已初始化
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * @brief 更新尺寸（当 Viewport 大小改变时调用）
     */
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

    /**
     * @brief 创建 RenderPass
     */
    int CreateRenderPass();

    /**
     * @brief 创建 Framebuffer
     */
    int CreateFramebuffer();

    /**
     * @brief 销毁 Framebuffer
     */
    void DestroyFramebuffer();
};

} // namespace Prisma::Graphic::Vulkan
