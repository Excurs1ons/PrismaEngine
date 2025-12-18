#pragma once

#include "ISampler.h"
#include "IShader.h"
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
class IResource;
class IRenderDevice;

/// @brief 纹理描述
struct TextureDesc : public ResourceDesc {
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::RGBA8_UNorm;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    bool allowRenderTarget = false;
    bool allowUnorderedAccess = false;
    bool allowShaderResource = true;
    const void* initialData = nullptr;
    uint64_t dataSize = 0;
};

/// @brief 缓冲区描述
struct BufferDesc : public ResourceDesc {
    BufferType type = BufferType::Vertex;
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::Default;
    const void* initialData = nullptr;
    uint32_t stride = 0;  // 对于结构化缓冲区
};

/// @brief 着色器描述
struct ShaderDesc : public ResourceDesc {
    ShaderType type = ShaderType::Vertex;
    ShaderLanguage language = ShaderLanguage::HLSL;
    std::string entryPoint = "main";
    std::string source;
    std::string filename;  // 如果从文件加载
    std::vector<std::string> defines;
    std::string target;    // 如 "vs_5_0", "ps_5_0"
    uint64_t compileTimestamp = 0;
    uint64_t compileHash = 0;
    ShaderCompileOptions& compileOptions;
};

/// @brief 管线描述
struct PipelineDesc : public ResourceDesc {
    // 顶点输入布局
    struct VertexAttribute {
        std::string semanticName;
        uint32_t semanticIndex = 0;
        TextureFormat format = TextureFormat::RGBA32_Float;
        uint32_t inputSlot = 0;
        uint32_t alignedByteOffset = 0;
        uint32_t inputSlotClass = 0;  // 0=vertex, 1=instance
        uint32_t instanceDataStepRate = 0;
    };
    std::vector<VertexAttribute> vertexAttributes;

    // 着色器
    std::shared_ptr<IShader> vertexShader;
    std::shared_ptr<IShader> pixelShader;
    std::shared_ptr<IShader> geometryShader;
    std::shared_ptr<IShader> hullShader;
    std::shared_ptr<IShader> domainShader;
    std::shared_ptr<IShader> computeShader;

    // 渲染状态
    struct BlendState {
        bool blendEnable = false;
        bool srcBlendAlpha = false;
        uint32_t writeMask = 0xF;  // RGBA all enabled
    };
    BlendState blendState;

    struct RasterizerState {
        bool cullEnable = true;
        bool frontCounterClockwise = false;
        bool depthClipEnable = true;
        bool scissorEnable = false;
        bool multisampleEnable = false;
        bool antialiasedLineEnable = false;
        uint32_t fillMode = 0;  // 0=solid, 1=wireframe
        uint32_t cullMode = 2;  // 0=none, 1=front, 2=back
        float depthBias = 0.0f;
        float depthBiasClamp = 0.0f;
        float slopeScaledDepthBias = 0.0f;
    };
    RasterizerState rasterizerState;

    struct DepthStencilState {
        bool depthEnable = true;
        bool depthWriteEnable = true;
        bool stencilEnable = false;
        uint8_t depthFunc = 4;  // 4=less
        uint8_t frontStencilFailOp = 1;  // 1=keep
        uint8_t frontStencilDepthFailOp = 1;
        uint8_t frontStencilPassOp = 1;
        uint8_t frontStencilFunc = 8;    // 8=always
        uint8_t backStencilFailOp = 1;
        uint8_t backStencilDepthFailOp = 1;
        uint8_t backStencilPassOp = 1;
        uint8_t backStencilFunc = 8;
        uint8_t stencilReadMask = 0xFF;
        uint8_t stencilWriteMask = 0xFF;
    };
    DepthStencilState depthStencilState;

    // 渲染目标
    uint32_t numRenderTargets = 1;
    TextureFormat renderTargetFormats[8] = {TextureFormat::RGBA8_UNorm};
    TextureFormat depthStencilFormat = TextureFormat::D32_Float;

    // 多重采样
    uint32_t sampleCount = 1;
    uint32_t sampleQuality = 0;

    // 图元拓扑
    uint32_t primitiveTopology = 4;  // 4=trianglelist
};

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

    // === 采样器管理 ===

    /// @brief 创建采样器
    /// @param desc 采样器描述
    /// @return 采样器智能指针
    virtual std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc) = 0;

    /// @brief 获取默认采样器
    /// @return 默认采样器智能指针
    virtual std::shared_ptr<ISampler> GetDefaultSampler() = 0;

    // === 资源查询和管理 ===

    /// @brief 根据ID获取资源
    /// @param id 资源ID
    /// @return 资源智能指针
    virtual std::shared_ptr<IResource> GetResource(ResourceId id) = 0;

    /// @brief 根据名称获取资源
    /// @param name 资源名称
    /// @return 资源智能指针
    virtual std::shared_ptr<IResource> GetResource(const std::string& name) = 0;

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