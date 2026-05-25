#pragma once

#include "IPipelineState.h"
#include "ISampler.h"
#include "IShader.h"
#include "ITexture.h"
#include "RenderDesc.h"
#include "RenderTypes.h"

#include <memory>
#include <shared_mutex>
#include <string>

namespace Prisma::Graphic {

// 前置声明
class ITexture;
class IBuffer;
class IShader;
class IPipeline;
class IPipelineState;
class IResource;
class IRenderDevice;
// 获取资源统计信息
struct ResourceStats {
    uint32_t totalResources     = 0;
    uint32_t loadedResources    = 0;
    uint32_t loadingResources   = 0;
    uint64_t totalMemoryUsage   = 0;
    uint64_t textureMemoryUsage = 0;
    uint64_t bufferMemoryUsage  = 0;
    int textureCount            = 0;
    int bufferCount             = 0;
    int shaderCount             = 0;
    int pipelineCount           = 0;
    uint64_t gpuMemoryUsage     = 0;
    uint64_t cpuMemoryUsage     = 0;
};

// 注意：TextureFilter, TextureAddressMode, TextureComparisonFunc, SamplerDesc 已在 RenderTypes.h 中定义

// 资源管理器抽象接口
/// 提供统一的资源加载、创建和管理功能
class IRenderResourceManager {
public:
    virtual ~IRenderResourceManager() = default;
    // 初始化资源管理器
    virtual int Initialize(IRenderDevice* device) = 0;

    // 关闭资源管理器
    virtual void Shutdown() = 0;

    // === 纹理管理 ===

    // 从文件加载纹理
    virtual std::shared_ptr<ITexture> LoadTexture(const std::string& filename,
                                                 bool generateMips = true) = 0;

    // 创建纹理
    virtual std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc) = 0;

    // 从内存创建纹理
    virtual std::shared_ptr<ITexture> CreateTextureFromMemory(const void* data,
                                                            uint64_t dataSize,
                                                            const TextureDesc& desc) = 0;

    // === 缓冲区管理 ===

    // 创建缓冲区
    virtual std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;

    // 创建动态缓冲区（每帧更新）
    virtual std::shared_ptr<IBuffer> CreateDynamicBuffer(uint64_t size, BufferType type) = 0;

    // === 着色器管理 ===

    // 从文件加载着色器
    virtual std::shared_ptr<IShader> LoadShader(const std::string& filename,
                                               const std::string& entryPoint,
                                               const std::string& target,
                                               const std::vector<std::string>& defines = {}) = 0;

    // 从源码创建着色器
    virtual std::shared_ptr<IShader> CreateShader(const std::string& source,
                                                 const ShaderDesc& desc) = 0;

    // 编译着色器
    virtual bool CompileShader(const ShaderDesc& desc, std::string* errors = nullptr) = 0;

    // === 管线管理 ===

    // 创建渲染管线
    virtual std::shared_ptr<IPipeline> CreatePipeline(const PipelineDesc& desc) = 0;

    // 从文件加载管线配置
    virtual std::shared_ptr<IPipeline> LoadPipeline(const std::string& filename) = 0;

    // 创建管线状态对象
    virtual std::shared_ptr<IPipelineState> CreatePipelineState(const PipelineStateDesc& desc) = 0;

    // === 采样器管理 ===

    // 创建采样器
    virtual std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc) = 0;

    // 获取默认采样器
    virtual std::shared_ptr<ISampler> GetDefaultSampler() = 0;

    // === 资源查询和管理 ===

    // 释放资源
    virtual void ReleaseResource(ResourceId id) = 0;

    // 释放所有引用计数为0的资源
    virtual void GarbageCollect() = 0;

    // 强制释放所有资源
    virtual void ReleaseAllResources() = 0;

    // === 异步加载 ===

    // 异步加载纹理
    virtual ResourceId LoadTextureAsync(const std::string& filename) = 0;

    // 异步加载着色器
    virtual ResourceId LoadShaderAsync(const std::string& filename) = 0;

    // 检查异步加载是否完成
    virtual bool IsAsyncLoadingComplete(ResourceId id) = 0;

    // 同步加载着色器 (内部使用或特殊需求)
    virtual std::shared_ptr<IShader> LoadShaderSync(const std::string& filename,
                                                   const std::string& entryPoint = "main",
                                                   const std::string& target = "",
                                                   const std::vector<std::string>& defines = {}) = 0;

    // 根据名称获取已加载的着色器
    virtual std::shared_ptr<IShader> GetShader(const std::string& name) = 0;

    // === 统计信息 ===


    virtual ResourceStats GetResourceStats() const = 0;

    // === 资源热重载 ===

    // 启用资源热重载
    virtual void EnableHotReload(bool enable) = 0;

    // 检查并重载修改的资源
    virtual void CheckAndReloadResources() = 0;

    // === 线程安全 ===

    // 获取资源锁（用于多线程访问）
    virtual std::shared_mutex& GetResourceLock() = 0;
};

} // namespace Prisma::Graphic