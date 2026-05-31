#pragma once

#include "RenderTypes.h"
#include <memory>
#include <string>
#include <vector>
// vk-bootstrap - Vulkan 初始化库
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
#include <VkBootstrap.h>
#endif

// VMA - Vulkan Memory Allocator
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
#include <vk_mem_alloc.h>
#endif
namespace Prisma::Graphic {

// 前置声明
class ICommandBuffer;
class IFence;
class IResourceFactory;
class ISwapChain;
class IPipeline;
class IResource;

// 渲染设备抽象接口
/// 提供设备级别的图形API抽象
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // 初始化设备
    virtual int Initialize(const DeviceDesc& desc) = 0;

    // 关闭设备
    virtual void Shutdown() = 0;

    // 获取设备名称
    virtual  std::string GetName() const = 0;

    // 获取API名称 (DirectX12, Vulkan等)
    virtual  std::string GetAPIName() const = 0;

    // 获取 GPU 名称
    virtual std::string GetGPUName() const = 0;

    // 获取设备是否支持光线追踪（设备可能因不支持而回退）
    virtual bool IsRayTracingSupported() const = 0;

    // === 命令缓冲区管理 ===

    // 创建命令缓冲区
    virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) = 0;

    // 提交命令缓冲区
    virtual void SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence = nullptr) = 0;

    // 批量提交命令缓冲区
    virtual void SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                                      const std::vector<IFence*>& fences = {}) = 0;

    // === 同步操作 ===

    // 等待设备空闲
    virtual void WaitForIdle() = 0;

    // 创建围栏
    virtual std::unique_ptr<IFence> CreateFence() = 0;

    // 等待围栏
    virtual void WaitForFence(IFence* fence) = 0;

    // === 资源管理 ===

    // 获取资源工厂
    virtual IResourceFactory* GetResourceFactory() const = 0;

    // === 交换链管理 ===

    // 创建交换链
    virtual std::unique_ptr<ISwapChain>
    CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, PresentMode presentMode = PresentMode::VSync) = 0;

    // 获取当前交换链
    virtual  ISwapChain* GetSwapChain() const = 0;

    // === 帧管理 ===

    // 获取当前帧的命令缓冲区
    virtual ICommandBuffer* GetCurrentCommandBuffer() const = 0;

    // 开始帧
    virtual void BeginFrame() = 0;

    // 结束帧
    virtual void EndFrame() = 0;

    // 呈现帧
    virtual void Present() = 0;

    // 调整交换链大小
    virtual void Resize(uint32_t width, uint32_t height) = 0;

    // === 查询支持的功能 ===

    // 检查是否支持多线程
    virtual  bool SupportsMultiThreaded() const = 0;

    // 检查是否支持绑定less纹理
    virtual  bool SupportsBindlessTextures() const = 0;

    // 检查是否支持计算着色器
    virtual  bool SupportsComputeShader() const = 0;

    // 检查是否支持光线追踪
    virtual  bool SupportsRayTracing() const = 0;

    // 检查是否支持网格着色器
    virtual  bool SupportsMeshShader() const = 0;

    // 检查是否支持可变速率着色
    virtual  bool SupportsVariableRateShading() const = 0;

    // === 渲染统计 ===

    // 获取GPU内存使用信息
    struct GPUMemoryInfo {
        uint64_t totalMemory     = 0;
        uint64_t usedMemory      = 0;
        uint64_t availableMemory = 0;
    };

    // 获取GPU内存信息
    virtual  GPUMemoryInfo GetGPUMemoryInfo() const = 0;

    // 获取渲染统计信息
    struct RenderStats {
        uint32_t frameCount     = 0;
        float frameTime         = 0.0f;
        float fps               = 0.0f;
        uint32_t drawCalls      = 0;
        uint32_t triangles      = 0;
        uint64_t gpuMemoryUsage = 0;
        uint64_t cpuMemoryUsage = 0;
    };

    // 获取渲染统计
    virtual  RenderStats GetRenderStats() const = 0;

    // === 调试功能 ===

    // 开始调试标记
    virtual void BeginDebugMarker(const std::string& name) = 0;

    // 结束调试标记
    virtual void EndDebugMarker() = 0;

    // 设置调试标记
    virtual void SetDebugMarker(const std::string& name) = 0;

        // === Vulkan 特定方法 ===

    // 获取 Vulkan 实例
    virtual VkInstance GetVkInstance() const = 0;
    // 获取物理设备
    virtual VkPhysicalDevice GetPhysicalDevice() const = 0;
    // 获取逻辑设备
    virtual VkDevice GetVkDevice() const = 0;
    // 获取 VMA 分配器
    virtual VmaAllocator GetVmaAllocator() const = 0;
    // 获取图形队列
    virtual VkQueue GetGraphicsQueue() const = 0;
    // 获取图形队列族索引
    virtual uint32_t GetGraphicsQueueFamily() const = 0;
    // 获取异步计算队列（可能回退到图形队列）
    virtual VkQueue GetComputeQueue() const { return GetGraphicsQueue(); }
    // 获取计算队列族索引
    virtual uint32_t GetComputeQueueFamily() const { return GetGraphicsQueueFamily(); }

    // 是否无头模式
    virtual bool IsHeadless() const { return false; }

    // 从 GPU 纹理回读像素数据
    virtual bool ReadbackTexture(class ITexture* texture, uint32_t width, uint32_t height,
                                 void* outBuffer, size_t bufferSize) { return false; }

    // 是否已初始化
    virtual bool IsInitialized() const = 0;
    // 获取当前帧索引
    virtual uint32_t GetCurrentFrameIndex() const = 0;

    // 获取附加渲染(UI等)的 RenderPass
    virtual VkRenderPass GetOverlayRenderPass() const = 0;

    // === 交换链 RenderPass 生命周期 ===

    // 开始交换链 RenderPass（Pipeline 显式调用，不再由 BeginFrame 自动开）
    virtual void BeginSwapChainRenderPass(const Prisma::Vector4& clearColor = {0.1f, 0.1f, 0.1f, 1.0f}) {}

    // 结束交换链 RenderPass（替代旧 SuspendDefaultRenderPass）
    virtual void EndSwapChainRenderPass() {}

    // 当前交换链 RenderPass 是否活跃
    virtual bool IsSwapChainRenderPassActive() const { return false; }
};

}  // namespace Prisma::Graphic