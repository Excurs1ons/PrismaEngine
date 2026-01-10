#ifndef MY_APPLICATION_RENDERERVULKAN_H
#define MY_APPLICATION_RENDERERVULKAN_H

#include "RendererAPI.h"
#include "Scene.h"
#include "VulkanContext.h"
#include "renderer/RenderPipeline.h"
#include <memory>

// vk-bootstrap - Vulkan 初始化库
#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include <vk_bootstrap.hpp>
#endif

// VMA - Vulkan Memory Allocator
#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include <vk_mem_alloc.h>
#endif

struct android_app;


class RendererVulkan : public RendererAPI {
public:
    RendererVulkan(android_app *pApp);
    virtual ~RendererVulkan();

    void init() override;
    void render() override;
    void onConfigChanged() override;  // 处理屏幕旋转
    void handleInput() override;     // 处理输入（包括 UI 交互）

    // SwapChain 重建相关函数（处理屏幕旋转）
    void cleanupSwapChain();      // 清理旧的 SwapChain 相关资源
    void recreateSwapChain();     // 重建 SwapChain 以适应新的屏幕尺寸

private:
    // === vk-bootstrap 简化的初始化方法 ===
    bool createInstanceWithVkBootstrap();
    bool createDeviceWithVkBootstrap();
    bool createSwapChainWithVkBootstrap(uint32_t width, uint32_t height);
    bool createVMAAllocator();

#ifdef PRISMA_ENABLE_RENDER_VULKAN
    // === VMA 简化的缓冲区创建辅助方法 ===
    struct BufferWithMemory {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
    };

    bool createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage vmaUsage,
        BufferWithMemory& outBuffer);

    bool createStagingBuffer(
        VkDeviceSize size,
        const void* data,
        BufferWithMemory& outBuffer);

    bool createDeviceLocalBuffer(
        VkDeviceSize size,
        const void* data,
        VkBufferUsageFlags usage,
        BufferWithMemory& outBuffer);
#endif

    // === 初始化方法 ===
    void createScene();
    void createUIComponents();       // 创建 UI 组件
    void createRenderPipeline();     // 创建逻辑渲染管线（封装 Pass）
    void createFramebuffers();
    void createCommandPool();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSetLayout();  // 创建描述符集布局（需要在 allocate descriptor sets 之前调用）
    void createDescriptorSets();
    void createSkyboxDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    // === 渲染方法 ===
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    android_app *app_;
    VulkanContext vulkanContext_;
    std::shared_ptr<Scene> scene_;  // 改为 shared_ptr，从 GameManager 获取，不随 Renderer 销毁

    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    // 帧时间跟踪
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrameTime_;

#ifdef PRISMA_ENABLE_RENDER_VULKAN
    // vk-bootstrap 对象
    std::unique_ptr<vkb::Instance> vkBootstrapInstance_;
    vkb::Device vkBootstrapDevice_;
    vkb::Swapchain vkBootstrapSwapchain_;

    // VMA 分配器
    VmaAllocator vmaAllocator_ = nullptr;

    // 队列族索引（由 vk-bootstrap 填充）
    uint32_t graphicsQueueFamily_ = 0;
    uint32_t presentQueueFamily_ = 0;
#endif





    // === 逻辑渲染管线 ===
    std::unique_ptr<RenderPipeline> renderPipeline_;
    UIPass* uiPass_ = nullptr;  // 非拥有指针（指向 renderPipeline_ 中的 UIPass）

    // === 临时数据（初始化后会移动到 Pass 中） ===
    std::vector<RenderObjectData> renderObjects;
    SkyboxRenderData skyboxData_;
    ClearColorData clearColorData_;

    // Vulkan resources for the scene
    // （Pipeline 创建已迁移到 Pass，但 descriptorSetLayout 仍保留用于创建 descriptor sets）
    VkDescriptorSetLayout descriptorSetLayout;        // MeshRenderer 用的 descriptor set layout
    VkDescriptorSetLayout skyboxDescriptorSetLayout;  // Skybox 用的 descriptor set layout

    VkDescriptorPool descriptorPool;

    // 光源 Uniform Buffer（全局共享）
    VkBuffer lightUniformBuffer;
    VkDeviceMemory lightUniformBufferMemory;

    // 状态栏高度（从 contentRect.top 缓存，用于坐标转换）
    int32_t statusBarHeight_ = 0;

};

#endif //MY_APPLICATION_RENDERERVULKAN_H