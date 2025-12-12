#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <DirectXMath.h>

namespace Engine {
namespace Graphic {

// 纹理格式
enum class TextureFormat : uint32_t {
    Unknown = 0,
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16,
    RG16,
    RGB16,
    RGBA16,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    D16,
    D24,
    D32,
    D24S8,
    D32S8,
    BC1,  // DXT1
    BC2,  // DXT3
    BC3,  // DXT5
    BC4,
    BC5,
    BC6H,
    BC7,
    ETC2,
    ASTC,
    Count
};

// 纹理类型
enum class TextureType : uint32_t {
    Texture2D = 0,
    TextureCube,
    Texture3D,
    Texture2DArray,
    TextureCubeArray,
    Count
};

// 纹理描述
struct TextureDesc {
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    bool generateMips = true;
    bool renderTarget = false;
    bool unorderedAccess = false;
    bool depthStencil = false;

    TextureDesc() = default;
};

// 纹理过滤模式
enum class TextureFilter : uint32_t {
    Point,
    Linear,
    Anisotropic,
    Count
};

// 纹理寻址模式
enum class TextureAddressMode : uint32_t {
    Wrap,
    Mirror,
    Clamp,
    Border,
    MirrorOnce,
    Count
};

// 采样器描述
struct SamplerDesc {
    TextureFilter filter = TextureFilter::Linear;
    TextureAddressMode addressU = TextureAddressMode::Wrap;
    TextureAddressMode addressV = TextureAddressMode::Wrap;
    TextureAddressMode addressW = TextureAddressMode::Wrap;
    float mipLODBias = 0.0f;
    uint32_t maxAnisotropy = 16;
    DirectX::XMFLOAT4 borderColor = DirectX::XMFLOAT4(0, 0, 0, 0);
    float minLOD = -FLT_MAX;
    float maxLOD = FLT_MAX;
};

// 纹理资源接口
class ITexture {
public:
    virtual ~ITexture() = default;

    // 获取纹理描述
    virtual const TextureDesc& GetDesc() const = 0;

    // 获取原生句柄
    virtual void* GetNativeHandle() const = 0;

    // 获取着色器资源视图
    virtual void* GetShaderResourceView() const = 0;

    // 获取渲染目标视图
    virtual void* GetRenderTargetView() const = 0;

    // 获取深度模板视图
    virtual void* GetDepthStencilView() const = 0;

    // 获取无序访问视图
    virtual void* GetUnorderedAccessView() const = 0;

    // 更新纹理数据
    virtual bool UpdateData(void* data, uint32_t size, uint32_t mipLevel = 0) = 0;

    // 生成Mipmap
    virtual bool GenerateMips() = 0;

    // 调整大小
    virtual bool Resize(uint32_t width, uint32_t height) = 0;

    // 保存到文件
    virtual bool SaveToFile(const std::string& filePath) = 0;

    // 检查是否有效
    virtual bool IsValid() const = 0;
};

// 采样器接口
class ISampler {
public:
    virtual ~ISampler() = default;

    // 获取采样器描述
    virtual const SamplerDesc& GetDesc() const = 0;

    // 获取原生句柄
    virtual void* GetNativeHandle() const = 0;
};

// 纹理管理器
class TextureManager {
public:
    static TextureManager& GetInstance();

    // 创建纹理
    std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc);

    // 从文件加载纹理
    std::shared_ptr<ITexture> LoadTexture(const std::string& filePath);

    // 创建立方体贴图
    std::shared_ptr<ITexture> LoadCubeMap(const std::string& filePath);

    // 创建渲染目标纹理
    std::shared_ptr<ITexture> CreateRenderTarget(uint32_t width, uint32_t height,
                                                 TextureFormat format = TextureFormat::RGBA8);

    // 创建深度缓冲纹理
    std::shared_ptr<ITexture> CreateDepthBuffer(uint32_t width, uint32_t height,
                                                TextureFormat format = TextureFormat::D32);

    // 创建采样器
    std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc);

    // 获取纹理
    std::shared_ptr<ITexture> GetTexture(const std::string& filePath);

    // 获取默认白色纹理
    std::shared_ptr<ITexture> GetWhiteTexture();

    // 获取默认黑色纹理
    std::shared_ptr<ITexture> GetBlackTexture();

    // 获取默认法线纹理
    std::shared_ptr<ITexture> GetNormalTexture();

    // 释放纹理
    void ReleaseTexture(const std::string& filePath);

    // 清理资源
    void Cleanup();

    // 设置纹理搜索路径
    void SetTextureSearchPath(const std::string& path);

    // 预加载纹理
    void PreloadTextures(const std::string& directory);

    // 获取纹理统计
    struct TextureStats {
        uint32_t totalTextures = 0;
        uint32_t totalSamplers = 0;
        uint32_t memoryUsage = 0;  // 以MB为单位
        uint32_t loadedTextures = 0;
        uint32_t renderTargets = 0;
    };

    TextureStats GetStats() const;

private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // 创建默认纹理
    void CreateDefaultTextures();

    // 计算纹理内存占用
    uint32_t CalculateTextureMemory(const TextureDesc& desc) const;

    // 线程安全
    mutable std::mutex m_mutex;

    // 纹理缓存
    std::unordered_map<std::string, std::shared_ptr<ITexture>> m_textures;

    // 采样器缓存
    std::unordered_map<size_t, std::shared_ptr<ISampler>> m_samplers;

    // 默认纹理
    std::shared_ptr<ITexture> m_whiteTexture;
    std::shared_ptr<ITexture> m_blackTexture;
    std::shared_ptr<ITexture> m_normalTexture;
    std::shared_ptr<ISampler> m_defaultSampler;

    // 搜索路径
    std::string m_searchPath = "textures/";

    // 统计信息
    mutable TextureStats m_stats;
};

// 便利函数
inline TextureManager& GetTextureManager() {
    return TextureManager::GetInstance();
}

} // namespace Graphic
} // namespace Engine