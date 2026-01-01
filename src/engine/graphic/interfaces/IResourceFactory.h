#pragma once

#include "RenderTypes.h"
#include "IResourceManager.h"
#include "IPipelineState.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;

/// @brief 资源工厂抽象接口
/// 提供创建后端特定资源对象的功能
class IResourceFactory {
public:
    virtual ~IResourceFactory() = default;

    /// @brief 初始化工厂
    /// @param device 渲染设备
    /// @return 是否初始化成功
    virtual bool Initialize(IRenderDevice* device) = 0;

    /// @brief 关闭工厂
    virtual void Shutdown() = 0;

    virtual void Reset() = 0;
    // === 纹理创建 ===

    /// @brief 创建纹理实现
    /// @param desc 纹理描述
    /// @return 纹理智能指针
    virtual std::unique_ptr<ITexture> CreateTextureImpl(const TextureDesc& desc) = 0;

    /// @brief 从文件创建纹理
    /// @param filename 文件路径
    /// @param desc 纹理描述（可选）
    /// @return 纹理智能指针
    virtual std::unique_ptr<ITexture> CreateTextureFromFile(const std::string& filename,
                                                           const TextureDesc* desc = nullptr) = 0;

    /// @brief 从内存创建纹理
    /// @param data 数据指针
    /// @param dataSize 数据大小
    /// @param desc 纹理描述
    /// @return 纹理智能指针
    virtual std::unique_ptr<ITexture> CreateTextureFromMemory(const void* data,
                                                            uint64_t dataSize,
                                                            const TextureDesc& desc) = 0;

    // === 缓冲区创建 ===

    /// @brief 创建缓冲区实现
    /// @param desc 缓冲区描述
    /// @return 缓冲区智能指针
    virtual std::unique_ptr<IBuffer> CreateBufferImpl(const BufferDesc& desc) = 0;

    /// @brief 创建动态缓冲区
    /// @param size 缓冲区大小
    /// @param type 缓冲区类型
    /// @param usage 使用标记
    /// @return 缓冲区智能指针
    virtual std::unique_ptr<IBuffer> CreateDynamicBuffer(uint64_t size,
                                                        BufferType type,
                                                        BufferUsage usage) = 0;

    // === 着色器创建 ===

    /// @brief 创建着色器实现
    /// @param desc 着色器描述
    /// @param bytecode 编译后的字节码
    /// @param reflection 反射信息
    /// @return 着色器智能指针
    virtual std::unique_ptr<IShader> CreateShaderImpl(const ShaderDesc& desc,
                                                     const std::vector<uint8_t>& bytecode,
                                                     const ShaderReflection& reflection) = 0;

    // === 管线创建 ===

    /// @brief 创建管线实现
    /// @param desc 管线描述
    /// @return 管线智能指针
    virtual std::unique_ptr<IPipelineState> CreatePipelineStateImpl() = 0;

    // === 采样器创建 ===

    /// @brief 创建采样器实现
    /// @param desc 采样器描述
    /// @return 采样器智能指针
    virtual std::unique_ptr<ISampler> CreateSamplerImpl(const SamplerDesc& desc) = 0;

    // === 交换链创建 ===

    /// @brief 创建交换链
    /// @param windowHandle 窗口句柄
    /// @param width 宽度
    /// @param height 高度
    /// @param format 格式
    /// @param bufferCount 缓冲区数量
    /// @param vsync 是否启用垂直同步
    /// @return 交换链智能指针
    virtual std::unique_ptr<ISwapChain> CreateSwapChainImpl(void* windowHandle,
                                                            uint32_t width,
                                                            uint32_t height,
                                                            TextureFormat format,
                                                            uint32_t bufferCount,
                                                            bool vsync) = 0;

    // === 围栏创建 ===

    /// @brief 创建围栏
    /// @return 围栏智能指针
    virtual std::unique_ptr<IFence> CreateFenceImpl() = 0;

    // === 批量创建 ===

    /// @brief 批量创建纹理
    /// @param descs 描述数组
    /// @param count 数量
    /// @return 纹理智能指针数组
    virtual std::vector<std::unique_ptr<ITexture>> CreateTexturesBatch(const TextureDesc* descs,
                                                                      uint32_t count) = 0;

    /// @brief 批量创建缓冲区
    /// @param descs 描述数组
    /// @param count 数量
    /// @return 缓冲区智能指针数组
    virtual std::vector<std::unique_ptr<IBuffer>> CreateBuffersBatch(const BufferDesc* descs,
                                                                    uint32_t count) = 0;

    // === 资源池管理 ===

    /// @brief 获取或创建纹理池
    /// @param format 纹理格式
    /// @param width 宽度
    /// @param height 高度
    /// @param mipLevels MIP级别
    /// @param arraySize 数组大小
    /// @return 池标识符
    virtual uint64_t GetOrCreateTexturePool(TextureFormat format,
                                            uint32_t width,
                                            uint32_t height,
                                            uint32_t mipLevels,
                                            uint32_t arraySize) = 0;

    /// @brief 从纹理池分配纹理
    /// @param poolId 池标识符
    /// @return 纹理智能指针
    virtual std::unique_ptr<ITexture> AllocateFromTexturePool(uint64_t poolId) = 0;

    /// @brief 释放纹理到池
    /// @param poolId 池标识符
    /// @param texture 纹理
    virtual void DeallocateToTexturePool(uint64_t poolId, ITexture* texture) = 0;

    /// @brief 清理未使用的资源池
    virtual void CleanupResourcePools() = 0;

    // === 资源验证 ===

    /// @brief 验证纹理描述
    /// @param desc 描述
    /// @param[out] errorMsg 错误信息
    /// @return 是否有效
    virtual bool ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) = 0;

    /// @brief 验证缓冲区描述
    /// @param desc 描述
    /// @param[out] errorMsg 错误信息
    /// @return 是否有效
    virtual bool ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) = 0;

    /// @brief 验证着色器描述
    /// @param desc 描述
    /// @param[out] errorMsg 错误信息
    /// @return 是否有效
    virtual bool ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) = 0;

  
    // === 内存管理 ===

    /// @brief 获取内存预算
    /// @param[out] budget 预算
    /// @param[out] usage 使用量
    virtual void GetMemoryBudget(uint64_t& budget, uint64_t& usage) const = 0;

    /// @brief 设置内存限制
    /// @param limit 内存限制
    virtual void SetMemoryLimit(uint64_t limit) = 0;

    /// @brief 检查是否超出内存限制
    /// @return 是否超出限制
    virtual bool IsMemoryLimitExceeded() const = 0;

    /// @brief 强制垃圾回收
    virtual void ForceGarbageCollection() = 0;

    // === 调试信息 ===

    /// @brief 获取资源创建统计
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

    /// @brief 重置统计信息
    virtual void ResetStats() = 0;

    // === 工厂配置 ===

    /// @brief 启用资源池化
    /// @param enable 是否启用
    virtual void EnableResourcePooling(bool enable) = 0;

    /// @brief 设置池化阈值
    /// @param threshold 阈值
    virtual void SetPoolingThreshold(uint64_t threshold) = 0;

    /// @brief 启用资源延迟销毁
    /// @param enable 是否启用
    /// @param delayFrames 延迟帧数
    virtual void EnableDeferredDestruction(bool enable, uint32_t delayFrames) = 0;

    /// @brief 处理延迟销毁的资源
    virtual void ProcessDeferredDestructions() = 0;
};

} // namespace PrismaEngine::Graphic