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

namespace PrismaEngine::Graphic {

// 前置声明
class ITexture;
class IBuffer;
class IShader;
class IPipeline;
class IPipelineState;
class IResource;
class IRenderDevice;


// 注意：TextureFilter, TextureAddressMode, TextureComparisonFunc, SamplerDesc 已在 RenderTypes.h 中定义

/// @brief 资源管理器抽象接口
/// 提供统一的资源加载、创建和管理功能
class IResourceManager {
public:
    virtual ~IResourceManager() = default;

    /// @brief 初始化资源管理器
    /// @param device 渲染设备
    /// @return 是否初始化成功
    virtual bool Initialize(IRenderDevice* device) = 0;

    /// @brief 关闭资源管理器
    virtual void Shutdown() = 0;

    // === 纹理管理 ===

    /// @brief 从文件加载纹理
    /// @param filename 文件路径
    /// @param generateMips 是否生成MIP映射
    /// @return 纹理智能指针
    virtual std::shared_ptr<ITexture> LoadTexture(const std::string& filename,
                                                 bool generateMips = true) = 0;

    /// @brief 创建纹理
    /// @param desc 纹理描述
    /// @return 纹理智能指针
    virtual std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc) = 0;

    /// @brief 从内存创建纹理
    /// @param data 图像数据
    /// @param dataSize 数据大小
    /// @param desc 纹理描述
    /// @return 纹理智能指针
    virtual std::shared_ptr<ITexture> CreateTextureFromMemory(const void* data,
                                                            uint64_t dataSize,
                                                            const TextureDesc& desc) = 0;

    // === 缓冲区管理 ===

    /// @brief 创建缓冲区
    /// @param desc 缓冲区描述
    /// @return 缓冲区智能指针
    virtual std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;

    /// @brief 创建动态缓冲区（每帧更新）
    /// @param size 缓冲区大小
    /// @param type 缓冲区类型
    /// @return 缓冲区智能指针
    virtual std::shared_ptr<IBuffer> CreateDynamicBuffer(uint64_t size, BufferType type) = 0;

    // === 着色器管理 ===

    /// @brief 从文件加载着色器
    /// @param filename 文件路径
    /// @param entryPoint 入口点函数
    /// @param target 编译目标
    /// @param defines 预处理器定义
    /// @return 着色器智能指针
    virtual std::shared_ptr<IShader> LoadShader(const std::string& filename,
                                               const std::string& entryPoint,
                                               const std::string& target,
                                               const std::vector<std::string>& defines = {}) = 0;

    /// @brief 从源码创建着色器
    /// @param source 着色器源码
    /// @param desc 着色器描述
    /// @return 着色器智能指针
    virtual std::shared_ptr<IShader> CreateShader(const std::string& source,
                                                 const ShaderDesc& desc) = 0;

    /// @brief 编译着色器
    /// @param desc 着色器描述
    /// @param[out] errors 编译错误信息
    /// @return 是否编译成功
    virtual bool CompileShader(const ShaderDesc& desc, std::string* errors = nullptr) = 0;

    // === 管线管理 ===

    /// @brief 创建渲染管线
    /// @param desc 管线描述
    /// @return 管线智能指针
    virtual std::shared_ptr<IPipeline> CreatePipeline(const PipelineDesc& desc) = 0;

    /// @brief 从文件加载管线配置
    /// @param filename 配置文件路径
    /// @return 管线智能指针
    virtual std::shared_ptr<IPipeline> LoadPipeline(const std::string& filename) = 0;

    /// @brief 创建管线状态对象
    /// @param desc 管线状态描述
    /// @return 管线状态智能指针
    virtual std::shared_ptr<IPipelineState> CreatePipelineState(const PipelineStateDesc& desc) = 0;

    // === 采样器管理 ===

    /// @brief 创建采样器
    /// @param desc 采样器描述
    /// @return 采样器智能指针
    virtual std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc) = 0;

    /// @brief 获取默认采样器
    /// @return 默认采样器智能指针
    virtual std::shared_ptr<ISampler> GetDefaultSampler() = 0;

    // === 资源查询和管理 ===

    /// @brief 释放资源
    /// @param id 资源ID
    virtual void ReleaseResource(ResourceId id) = 0;

    /// @brief 释放所有引用计数为0的资源
    virtual void GarbageCollect() = 0;

    /// @brief 强制释放所有资源
    virtual void ReleaseAllResources() = 0;

    // === 异步加载 ===

    /// @brief 异步加载纹理
    /// @param filename 文件路径
    /// @return 资源ID，用于后续查询
    virtual ResourceId LoadTextureAsync(const std::string& filename) = 0;

    /// @brief 异步加载着色器
    /// @param filename 文件路径
    /// @return 资源ID，用于后续查询
    virtual ResourceId LoadShaderAsync(const std::string& filename) = 0;

    /// @brief 检查异步加载是否完成
    /// @param id 资源ID
    /// @return 是否加载完成
    virtual bool IsAsyncLoadingComplete(ResourceId id) = 0;

    // === 统计信息 ===

    /// @brief 获取资源统计信息
    struct ResourceStats {
        uint32_t totalResources = 0;
        uint32_t loadedResources = 0;
        uint32_t loadingResources = 0;
        uint64_t totalMemoryUsage = 0;
        uint64_t textureMemoryUsage = 0;
        uint64_t bufferMemoryUsage = 0;
        int textureCount = 0;
        int bufferCount = 0;
        int shaderCount = 0;
        int pipelineCount = 0;
        uint64_t gpuMemoryUsage = 0;
        uint64_t cpuMemoryUsage = 0;
    };
    virtual ResourceStats GetResourceStats() const = 0;

    // === 资源热重载 ===

    /// @brief 启用资源热重载
    /// @param enable 是否启用
    virtual void EnableHotReload(bool enable) = 0;

    /// @brief 检查并重载修改的资源
    virtual void CheckAndReloadResources() = 0;

    // === 线程安全 ===

    /// @brief 获取资源锁（用于多线程访问）
    /// @return 读写锁
    virtual std::shared_mutex& GetResourceLock() = 0;
};

} // namespace PrismaEngine::Graphic