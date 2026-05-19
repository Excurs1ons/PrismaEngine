#pragma once

#include "Export.h"
#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceFactory.h"
#include "interfaces/ISwapChain.h"

// Vulkan headers
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Prisma::Graphic::Vulkan {

// 前置声明
class VulkanCommandBuffer;
class VulkanFence;
class VulkanSwapChain;
class VulkanResourceFactory;

/// @brief Vulkan渲染设备
/// 实现IRenderDevice接口，基于Vulkan 1.3+，使用 vk-bootstrap 和 VMA
class ENGINE_API RenderDeviceVulkan : public IRenderDevice {
public:
    RenderDeviceVulkan();
    ~RenderDeviceVulkan() override;

    // ========== IRenderDevice接口实现 ==========
    int Initialize(const DeviceDesc& desc) override;
    void Shutdown() override;
    std::string GetName() const override;
    std::string GetAPIName() const override;
    std::string GetGPUName() const override;

    // 命令缓冲区
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override;
    void SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence = nullptr) override;
    void SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                              const std::vector<IFence*>& fences = {}) override;

    // 同步
    void WaitForIdle() override;
    std::unique_ptr<IFence> CreateFence() override;
    void WaitForFence(IFence* fence) override;

    // 资源
    IResourceFactory* GetResourceFactory() const override;

    // 交换链
    std::unique_ptr<ISwapChain>
    CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, PresentMode presentMode = PresentMode::VSync) override;
    ISwapChain* GetSwapChain() const override;

    // 帧管理
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;
    void Resize(uint32_t width, uint32_t height) override;

    // 功能查询
    bool SupportsMultiThreaded() const override { return true; }
    bool SupportsBindlessTextures() const override { return m_deviceFeatures.supportsBindless; }
    bool SupportsComputeShader() const override { return true; }
    bool SupportsRayTracing() const override { return m_deviceFeatures.supportsRayTracing; }
    bool SupportsMeshShader() const override { return m_deviceFeatures.supportsMeshShading; }
    bool SupportsVariableRateShading() const override { return m_deviceFeatures.supportsVariableRateShading; }

    // 统计
    GPUMemoryInfo GetGPUMemoryInfo() const override;
    RenderStats GetRenderStats() const override;

    // 调试
    void BeginDebugMarker(const std::string& name) override;
    void EndDebugMarker() override;
    void SetDebugMarker(const std::string& name) override;
    
    // ========== Vulkan特定方法 (IRenderDevice 接口要求) ==========
    VkInstance GetVkInstance() const override { return m_instance; }
    VkSurfaceKHR GetVkSurface() const { return m_surface; }
    VkPhysicalDevice GetPhysicalDevice() const override { return m_physicalDevice; }
    VkDevice GetVkDevice() const override { return m_device; }
    VkQueue GetGraphicsQueue() const override { return m_graphicsQueue; }
    uint32_t GetGraphicsQueueFamily() const override { return m_graphicsQueueFamily; }
    VkDescriptorPool GetVkDescriptorPool() const { return m_descriptorPool; }
    VmaAllocator GetVmaAllocator() const override { return m_allocator; }
    VmaAllocator GetAllocator() const { return m_allocator; }
    bool IsInitialized() const override { return m_initialized; }
    uint32_t GetCurrentFrameIndex() const override { return m_currentFrameIndex; }

    // 获取用于附加渲染(UI等)的RenderPass（从交换链获取）
    VkRenderPass GetOverlayRenderPass() const override;

    // 离屏渲染支持
    bool IsSwapChainRenderPassActive() const override { return m_isDefaultRenderPassActive; }
    bool IsDefaultRenderPassActive() const { return m_isDefaultRenderPassActive; }

    void BeginSwapChainRenderPass() override;
    void EndSwapChainRenderPass() override;

    bool IsHeadless() const override { return m_headless; }

    bool ReadbackTexture(class ITexture* texture, uint32_t width, uint32_t height,
                         void* outBuffer, size_t bufferSize) override;

    // GPU 图像回读到 host 内存（用于头模式离屏输出）
    bool ReadbackImage(VkImage image, uint32_t width, uint32_t height, VkFormat format,
                       void* outBuffer, size_t bufferSize);

    // 获取当前帧的命令缓冲区（供 Viewport 渲染使用）
    ICommandBuffer* GetCurrentCommandBuffer() const {
        return m_frameActive ? (ICommandBuffer*)m_vulkanCommandBuffers[m_currentFrame].get() : nullptr;
    }

    // -----------------------------------------------------------------------
    // [修复] ImGui 跨 DLL 渲染回调
    //
    // 目的：
    //   解决 imgui_impl_vulkan.cpp 被同时编译进 Prisma.dll（Engine）和
    //   PrismaEditor.dll（Editor）两份，导致两侧的 ImGui 后端状态
    //   （BackendRendererUserData）互不相通的问题。
    //
    //   问题根源：
    //   ImGui_ImplVulkan_Init() 在 PrismaEditor.dll 侧调用，BackendRendererUserData
    //   注册在 PrismaEditor.dll 的静态数据段中。EndFrame() 在 Prisma.dll 侧调用
    //   ImGui_ImplVulkan_RenderDrawData，此时 ImGui_ImplVulkan_GetBackendData()
    //   拿到的是 Prisma.dll 侧未初始化的数据（内容全为 nullptr/0），
    //   访问字体纹理 TexID 得到 ImTextureID_Invalid（0xFFFFFFFFFFFFFFFF），
    //   传给 vkCmdBindDescriptorSets 引发访问冲突崩溃。
    //
    //   修复过程：
    //   将 ImGui_ImplVulkan_RenderDrawData 调用完全移出 Engine，
    //   改为让调用方（PrismaEditor.dll）在收到 VkCommandBuffer 句柄后
    //   在自己的 DLL 上下文中执行实际渲染，从而使用正确的后端数据。

    // -----------------------------------------------------------------------
    using OverlayRenderCallback = std::function<void(VkCommandBuffer)>;
    void SetOverlayRenderCallback(OverlayRenderCallback callback) { m_overlayRenderCallback = std::move(callback); }

    /**
     * @brief Capture current swapchain frame to host memory (Binary Raw)
     */
    void CaptureFrame(void* outBuffer, size_t* outSize);

private:
    // vk-bootstrap 核心
    vkb::Instance m_vkbInstance;
    vkb::PhysicalDevice m_vkbPhysicalDevice;
    vkb::Device m_vkbDevice;

    VkInstance m_instance             = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface            = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device                 = VK_NULL_HANDLE;

    // VMA
    VmaAllocator m_allocator = VK_NULL_HANDLE;

    // 队列
    VkQueue m_graphicsQueue        = VK_NULL_HANDLE;
    VkQueue m_presentQueue         = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;

    // 描述符池
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // 命令控制
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<std::unique_ptr<VulkanCommandBuffer>> m_vulkanCommandBuffers;

    // 同步
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    uint32_t m_currentFrame = 0;

    // 资源
    std::unique_ptr<VulkanSwapChain> m_swapChain;
    std::unique_ptr<VulkanResourceFactory> m_resourceFactory;

    bool m_headless = false;

    // 设备能力
    struct DeviceFeatures {
        bool supportsBindless            = false;
        bool supportsRayTracing          = false;
        bool supportsMeshShading         = false;
        bool supportsVariableRateShading = false;
    } m_deviceFeatures;

    RenderStats m_stats;
    DeviceDesc m_desc;
    std::string m_gpuName;
    bool m_initialized = false;
    uint32_t m_currentFrameIndex = 0;
    uint32_t m_pendingPresentImageIndex = 0;
    bool m_frameActive = false;
    bool m_hasPendingPresent = false;

    // 覆盖层渲染回调（指向 PrismaEditor.dll 的实际渲染函数）
    OverlayRenderCallback m_overlayRenderCallback;

    // 离屏渲染支持
    bool m_isDefaultRenderPassActive = false;
};

}  // namespace Prisma::Graphic::Vulkan
