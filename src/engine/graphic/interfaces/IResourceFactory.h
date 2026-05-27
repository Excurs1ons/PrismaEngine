#pragma once

#include "RenderTypes.h"
#include "IResourceManager.h"
#include "IPipelineState.h"
#include "IComputePipeline.h"
#include "IDescriptorSet.h"
#include <memory>

namespace Prisma::Graphic {

// 前置声明
class IRenderDevice;

// 资源工厂抽象接口
/// 提供创建后端特定资源对象的功能
class IResourceFactory {
public:
    virtual ~IResourceFactory() = default;

    // 初始化工厂
    virtual bool Initialize(IRenderDevice* device) = 0;

    // 关闭工厂
    virtual void Shutdown() = 0;

    virtual void Reset() = 0;
    // === 纹理创建 ===

    // 创建纹理实现
    virtual std::unique_ptr<ITexture> CreateTextureImpl(const TextureDesc& desc) = 0;

    // 从文件创建纹理
    virtual std::unique_ptr<ITexture> CreateTextureFromFile(const std::string& filename,
                                                           const TextureDesc* desc = nullptr) = 0;

    // 从内存创建纹理
    virtual std::unique_ptr<ITexture> CreateTextureFromMemory(const void* data,
                                                            uint64_t dataSize,
                                                            const TextureDesc& desc) = 0;

    // === 缓冲区创建 ===

    // 创建缓冲区实现
    virtual std::unique_ptr<IBuffer> CreateBufferImpl(const BufferDesc& desc) = 0;

    // 创建动态缓冲区
    virtual std::unique_ptr<IBuffer> CreateDynamicBuffer(uint64_t size,
                                                        BufferType type,
                                                        BufferUsage usage) = 0;

    // === 着色器创建 ===

    // 创建着色器实现
    virtual std::unique_ptr<IShader> CreateShaderImpl(const ShaderDesc& desc,
                                                     const std::vector<uint8_t>& bytecode,
                                                     const ShaderReflection& reflection) = 0;

    // === 管线创建 ===

    // 创建管线实现
    virtual std::unique_ptr<IPipelineState> CreatePipelineStateImpl() = 0;

    // 创建计算管线实现
    virtual std::unique_ptr<IComputePipeline> CreateComputePipelineImpl() = 0;

    // === 采样器创建 ===

    // 创建采样器实现
    virtual std::unique_ptr<ISampler> CreateSamplerImpl(const SamplerDesc& desc) = 0;

    // === 交换链创建 ===

    // 创建交换链
    virtual std::unique_ptr<ISwapChain> CreateSwapChainImpl(void* windowHandle,
                                                            uint32_t width,
                                                            uint32_t height,
                                                            TextureFormat format,
                                                            uint32_t bufferCount,
                                                            PresentMode presentMode) = 0;

    // === 围栏创建 ===

    // 创建围栏
    virtual std::unique_ptr<IFence> CreateFenceImpl() = 0;

    // === 描述符集创建 ===
    virtual std::shared_ptr<IDescriptorSet> CreateDescriptorSet(IDescriptorSetLayout* layout) = 0;
    virtual std::shared_ptr<IDescriptorSetLayout> CreateDescriptorSetLayout(const std::vector<ShaderResource>& resources) = 0;

    // === 批量创建 ===

    // 批量创建纹理
    virtual std::vector<std::unique_ptr<ITexture>> CreateTexturesBatch(const TextureDesc* descs,
                                                                      uint32_t count) = 0;

    // 批量创建缓冲区
    virtual std::vector<std::unique_ptr<IBuffer>> CreateBuffersBatch(const BufferDesc* descs,
                                                                    uint32_t count) = 0;

    // === 资源池管理 ===

    // 获取或创建纹理池
    virtual uint64_t GetOrCreateTexturePool(TextureFormat format,
                                            uint32_t width,
                                            uint32_t height,
                                            uint32_t mipLevels,
                                            uint32_t arraySize) = 0;

    // 从纹理池分配纹理
    virtual std::unique_ptr<ITexture> AllocateFromTexturePool(uint64_t poolId) = 0;

    // 释放纹理到池
    virtual void DeallocateToTexturePool(uint64_t poolId, ITexture* texture) = 0;

    // 清理未使用的资源池
    virtual void CleanupResourcePools() = 0;

    // === 资源验证 ===

    // 验证纹理描述
    virtual bool ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) = 0;

    // 验证缓冲区描述
    virtual bool ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) = 0;

    // 验证着色器描述
    virtual bool ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) = 0;

  
    // === 内存管理 ===

    // 获取内存预算
    virtual void GetMemoryBudget(uint64_t& budget, uint64_t& usage) const = 0;

    // 设置内存限制
    virtual void SetMemoryLimit(uint64_t limit) = 0;

    // 检查是否超出内存限制
    virtual bool IsMemoryLimitExceeded() const = 0;

    // 强制垃圾回收
    virtual void ForceGarbageCollection() = 0;

    // === 调试信息 ===

    // 获取资源创建统计
    struct ResourceCreationStats {
        uint32_t texturesCreated = 0;
        uint32_t buffersCreated = 0;
        uint32_t shadersCreated = 0;
        uint32_t pipelinesCreated = 0;
        uint32_t samplersCreated = 0;
        uint32_t texturesPooled = 0;  // 从池中获取的纹理数
        uint64_t totalMemoryAllocated = 0;
        uint64_t peakMemoryUsage = 0;
    };
    virtual ResourceCreationStats GetCreationStats() const = 0;

    // 重置统计信息
    virtual void ResetStats() = 0;

    // === 工厂配置 ===

    // 启用资源池化
    virtual void EnableResourcePooling(bool enable) = 0;

    // 设置池化阈值
    virtual void SetPoolingThreshold(uint64_t threshold) = 0;

    // 启用资源延迟销毁
    virtual void EnableDeferredDestruction(bool enable, uint32_t delayFrames) = 0;

    // 处理延迟销毁的资源
    virtual void ProcessDeferredDestructions() = 0;
};

} // namespace Prisma::Graphic