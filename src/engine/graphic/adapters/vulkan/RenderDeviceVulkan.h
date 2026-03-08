#pragma once

#include "interfaces/IRenderDevice.h"
#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/ISwapChain.h"
#include "interfaces/IResourceFactory.h"

// Vulkan headers
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <array>

namespace PrismaEngine::Graphic::Vulkan {

// 前置声明
class VulkanCommandBuffer;
class VulkanFence;
class VulkanSwapChain;
class VulkanResourceFactory;

/// @brief Vulkan渲染设备
/// 实现IRenderDevice接口，基于Vulkan 1.3+，使用 vk-bootstrap 和 VMA
class RenderDeviceVulkan : public IRenderDevice {
public:
    RenderDeviceVulkan();
    ~RenderDeviceVulkan() override;

    // ========== IRenderDevice接口实现 ==========
    bool Initialize(const DeviceDesc& desc) override;
    void Shutdown() override;
    std::string GetName() const override;
    std::string GetAPIName() const override;

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
    std::unique_ptr<ISwapChain> CreateSwapChain(void* windowHandle,
                                               uint32_t width,
                                               uint32_t height,
                                               bool vsync = true) override;
    ISwapChain* GetSwapChain() const override;

    // 帧管理
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

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

    // ImGui 集成
#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
    bool InitializeImGui() override;
    void ShutdownImGui() override;
#else
    bool InitializeImGui() override { return true; }
    void ShutdownImGui() override {}
#endif

    // 调试
    void BeginDebugMarker(const std::string& name) override;
    void EndDebugMarker() override;
    void SetDebugMarker(const std::string& name) override;

    // ========== Vulkan特定方法 ==========
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetDevice() const { return m_device; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VmaAllocator GetAllocator() const { return m_allocator; }

private:
    // vk-bootstrap 核心
    vkb::Instance m_vkbInstance;
    vkb::PhysicalDevice m_vkbPhysicalDevice;
    vkb::Device m_vkbDevice;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    // VMA
    VmaAllocator m_allocator = VK_NULL_HANDLE;

    // 队列
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;

    // 资源
    std::unique_ptr<VulkanSwapChain> m_swapChain;
    std::unique_ptr<VulkanResourceFactory> m_resourceFactory;

    // ImGui
#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
#endif

    // 设备能力
    struct DeviceFeatures {
        bool supportsBindless = false;
        bool supportsRayTracing = false;
        bool supportsMeshShading = false;
        bool supportsVariableRateShading = false;
    } m_deviceFeatures;

    RenderStats m_stats;
    DeviceDesc m_desc;
    bool m_initialized = false;
};

} // namespace PrismaEngine::Graphic::Vulkan