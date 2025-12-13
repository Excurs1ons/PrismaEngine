#pragma once
#include "RenderBackend.h"
#include "RenderCommandContext.h"
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
namespace Engine {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    bool IsComplete() { return graphicsFamily.has_value(); }
};

// 前向声明
class VulkanRenderCommandContext;

class RenderBackendVulkan : public RenderBackend {
    friend class RenderBackend;

public:
    RenderBackendVulkan();
    ~RenderBackendVulkan() override {}

    // 添加带参数的初始化方法
    bool Initialize(Platform* platform, void* windowHandle, void* surface, uint32_t width, uint32_t height) override;
    void Shutdown() override;

    void SetGuiRenderCallback(GuiRenderCallback callback) override { m_guiRenderCallback = callback; }

    void BeginFrame() override;
    void EndFrame() override;

    void Resize(uint32_t width, uint32_t height) override;

    void SubmitRenderCommand(const RenderCommand& cmd) override;

    bool Supports(RendererFeature feature) const override;

    void Present() override;
    
    // 实现CreateCommandContext方法
    RenderCommandContext* CreateCommandContext() override;

    // 获取默认渲染目标和深度缓冲
    void* GetDefaultRenderTarget() override;
    void* GetDefaultDepthBuffer() override;

    // 获取当前渲染目标尺寸
    void GetRenderTargetSize(uint32_t& width, uint32_t& height) override;

    bool isInitialized = false;
    bool isFrameActive = false; // 跟踪帧状态，防止EndFrame在BeginFrame前调用
    VkInstance GetVulkanInstance() const { return instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
    VkDevice GetDevice() const { return device; }
    VkQueue GetGraphicsQueue() const { return graphicsQueue; }
    uint32_t GetGraphicsQueueFamily() const { return graphicsQueueFamily; }
    VkRenderPass GetRenderPass() const { return renderPass; }
    uint32_t GetMinImageCount() const { return 2; } // 通常为2
    uint32_t GetImageCount() const { return (uint32_t)swapChainImages.size(); }

    bool CreateInstance(const char* const* extensions, uint32_t extCount);

protected:
    const RendererFeature m_support = RendererFeature::None;
    GuiRenderCallback m_guiRenderCallback;

private:
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

    // Vulkan核心对象
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkRenderPass renderPass;
    uint32_t graphicsQueueFamily;
    // VkDescriptorPool descriptorPool; // 移除，由 Editor 自己管理

    // 添加交换链相关成员
    void* m_windowHandle = nullptr;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkFormat swapChainImageFormat;
    VkExtent2D m_swapchainExtent;

    // 命令缓冲相关
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // 同步对象
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    
    friend class VulkanRenderCommandContext;
};

}  // namespace Engine