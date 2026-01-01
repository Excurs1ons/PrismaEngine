#pragma once

#include "interfaces/RenderTypes.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/IResourceFactory.h"
#include "interfaces/ISwapChain.h"
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <memory>

namespace PrismaEngine::Graphic {

/// @brief Vulkan 渲染设备实现
class VulkanRenderDevice : public IRenderDevice {
public:
    VulkanRenderDevice();
    ~VulkanRenderDevice() override;

    // === IRenderDevice 接口实现 ===

    /// @brief 初始化设备
    bool Initialize(const DeviceDesc& desc) override;
    /// @brief 关闭设备
    void Shutdown() override;

    /// @brief 获取设备名称
    [[nodiscard]] std::string GetName() const override;
    /// @brief 获取API名称
    [[nodiscard]] std::string GetAPIName() const override;

    // 命令缓冲区管理
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override;
    void SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence = nullptr) override;
    void SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                              const std::vector<IFence*>& fences = {}) override;

    // 同步操作
    void WaitForIdle() override;
    std::unique_ptr<IFence> CreateFence() override;
    void WaitForFence(IFence* fence) override;

    // 资源管理
    [[nodiscard]] IResourceFactory* GetResourceFactory() const override;

    // 交换链管理
    std::unique_ptr<ISwapChain> CreateSwapChain(void* windowHandle,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 bool vsync = true) override;
    [[nodiscard]] ISwapChain* GetSwapChain() const override;

    // 帧管理
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    // 功能查询
    [[nodiscard]] bool SupportsMultiThreaded() const override;
    [[nodiscard]] bool SupportsBindlessTextures() const override;
    [[nodiscard]] bool SupportsComputeShader() const override;
    [[nodiscard]] bool SupportsRayTracing() const override;
    [[nodiscard]] bool SupportsMeshShader() const override;
    [[nodiscard]] bool SupportsVariableRateShading() const override;

    [[nodiscard]] GPUMemoryInfo GetGPUMemoryInfo() const override;
    [[nodiscard]] RenderStats GetRenderStats() const override;

    // 调试
    void BeginDebugMarker(const std::string& name) override;
    void EndDebugMarker() override;
    void SetDebugMarker(const std::string& name) override;

    // === Vulkan 特定方法 ===

    /// @brief 获取 Vulkan 实例
    [[nodiscard]] VkInstance GetVkInstance() const { return m_instance; }
    /// @brief 获取物理设备
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    /// @brief 获取逻辑设备
    [[nodiscard]] VkDevice GetVkDevice() const { return m_device; }
    /// @brief 获取图形队列
    [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    /// @brief 获取图形队列族索引
    [[nodiscard]] uint32_t GetGraphicsQueueFamily() const { return m_graphicsQueueFamily; }

    /// @brief 是否已初始化
    [[nodiscard]] bool IsInitialized() const { return m_initialized; }
    /// @brief 获取当前帧索引
    [[nodiscard]] uint32_t GetCurrentFrameIndex() const { return m_currentFrameIndex; }

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> computeFamily;
        std::optional<uint32_t> transferFamily;

        bool IsComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    // === Vulkan 初始化辅助方法 ===

    bool CreateInstance(const std::vector<const char*>& requiredExtensions);
    bool PickPhysicalDevice(const VkSurfaceKHR surface);
    bool CreateLogicalDevice();
    bool CreateSwapChain(uint32_t width, uint32_t height, bool vsync);
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateSyncObjects();

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // === Vulkan 核心对象 ===

    bool m_initialized = false;
    std::string m_deviceName = "Vulkan Device";

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;

    uint32_t m_graphicsQueueFamily = 0;
    uint32_t m_presentQueueFamily = 0;
    uint32_t m_computeQueueFamily = 0;
    uint32_t m_transferQueueFamily = 0;

    // 交换链
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D m_swapChainExtent = {0, 0};
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    // 渲染通道
    VkRenderPass m_renderPass = VK_NULL_HANDLE;

    // 命令
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    // 同步对象
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;

    uint32_t m_currentFrameIndex = 0;
    uint32_t m_currentImageIndex = 0;

    // 组件
    std::unique_ptr<IResourceFactory> m_resourceFactory;
    std::unique_ptr<ISwapChain> m_swapChainObj;

    // 统计
    RenderStats m_stats = {};
    uint32_t m_frameCount = 0;
};

} // namespace PrismaEngine::Graphic
