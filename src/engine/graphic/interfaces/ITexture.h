#pragma once

#include "RenderTypes.h"
#include "IResource.h"


namespace Prisma::Graphic {

// 前置声明
class IRenderDevice;
class IBuffer;

// 纹理子资源数据
struct TextureSubResourceData {
    const void* data = nullptr;
    uint64_t rowPitch = 0;      // 每行的字节数
    uint64_t slicePitch = 0;    // 每个切片的字节数
};

// 纹理映射描述
struct TextureMapDesc {
    void* data = nullptr;
    uint64_t rowPitch = 0;
    uint64_t depthPitch = 0;
    uint64_t size = 0;
    uint64_t offset = 0;
};

// 纹理描述符
enum class TextureDescriptorType {
    ShaderResourceView,
    UnorderedAccessView,
    RenderTargetView,
    DepthStencilView
};

// 纹理描述
struct TextureDesc : public ResourceDesc {
    TextureType type = TextureType::Texture2D;
    TextureFormat format = TextureFormat::RGBA8_UNorm;
    uint64_t width = 1;
    uint64_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    bool allowRenderTarget = false;
    bool allowUnorderedAccess = false;
    bool allowShaderResource = true;
    bool allowDepthStencil = false;  // 是否允许作为深度模板缓冲区
    const void* initialData = nullptr;
    uint64_t dataSize = 0;
    std::string filename;  // 文件名（用于从文件加载）
    uint32_t sampleCount = 1;    // 多重采样数量
    uint32_t sampleQuality = 0;  // 多重采样质量
    bool exportable = false;     // 是否可导出到外部（Vulkan→D3D11 interop）
};

// 纹理抽象接口
class ITexture : public IResource {
public:
    virtual ~ITexture() = default;

    // 获取纹理类型
    virtual TextureType GetTextureType() const = 0;

    // 获取纹理格式
    virtual TextureFormat GetFormat() const = 0;

    // 获取宽度
    virtual float GetWidth() const = 0;

    // 获取高度
    virtual float GetHeight() const = 0;

    // 获取深度
    virtual uint32_t GetDepth() const = 0;

    // 获取MIP级别数量
    virtual uint32_t GetMipLevels() const = 0;

    // 获取数组大小
    virtual uint32_t GetArraySize() const = 0;

    // 获取采样数量
    virtual uint32_t GetSampleCount() const = 0;

    // 获取采样质量
    virtual uint32_t GetSampleQuality() const = 0;

    // 检查是否为渲染目标
    virtual bool IsRenderTarget() const = 0;

    // 检查是否为深度模板缓冲区
    virtual bool IsDepthStencil() const = 0;

    // 检查是否支持着色器资源访问
    virtual bool IsShaderResource() const = 0;

    // 检查是否支持无序访问
    virtual bool IsUnorderedAccess() const = 0;

    // 获取每个像素的字节数
    virtual uint64_t GetBytesPerPixel() const = 0;

    // 获取指定MIP级别的子资源大小
    virtual uint64_t GetSubresourceSize(uint32_t mipLevel) const = 0;

    // === 数据操作 ===

    // 映射纹理数据用于CPU访问
    virtual TextureMapDesc Map(uint32_t mipLevel = 0, uint32_t arraySlice = 0,
                               uint32_t mapType = 0) = 0;

    // 取消映射纹理数据
    virtual void Unmap(uint32_t mipLevel = 0, uint32_t arraySlice = 0) = 0;

    // 更新纹理数据
    virtual void UpdateData(const void* data, uint64_t dataSize,
                   uint32_t mipLevel, uint32_t arraySlice,
                   uint32_t left, uint32_t top, uint32_t front,
                   uint64_t width, uint64_t height, uint64_t depth) = 0;

    // 生成MIP映射
    virtual void GenerateMips() = 0;

    // 复制纹理
    virtual void CopyFrom(ITexture* srcTexture,
                         uint32_t srcMipLevel = 0, uint32_t srcArraySlice = 0,
                         uint32_t dstMipLevel = 0, uint32_t dstArraySlice = 0) = 0;

    // 读取纹理数据到CPU缓冲区
    virtual bool ReadData(uint32_t mipLevel, uint32_t arraySlice,
                         void* dstBuffer, uint64_t bufferSize) = 0;

    // === 采样器相关 ===

    // 创建采样器视图
    virtual uint64_t CreateDescriptor(TextureDescriptorType descType,
                                     TextureFormat format = TextureFormat::Unknown,
                                     uint32_t mipLevel = 0,
                                     uint32_t arraySize = 0) = 0;

    // 获取默认着色器资源视图描述符
    virtual uint64_t GetDefaultSRV() const = 0;

    // 获取默认渲染目标视图描述符
    virtual uint64_t GetDefaultRTV() const = 0;

    // 获取默认深度模板视图描述符
    virtual uint64_t GetDefaultDSV() const = 0;

    // 获取默认无序访问视图描述符
    virtual uint64_t GetDefaultUAV() const = 0;

    // === 渲染目标操作 ===

    // 清除纹理
    virtual void Clear(const Color& color,
                      uint32_t mipLevel = 0,
                      uint32_t arraySlice = 0) = 0;

    // 清除深度模板缓冲区
    virtual void ClearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) = 0;

    // 解析多重采样纹理
    virtual void ResolveMultisampled(ITexture* dstTexture,
                                    TextureFormat format = TextureFormat::Unknown) = 0;

    // === 内存管理 ===

    // 丢弃资源内容
    virtual void Discard(uint32_t mipLevel = 0, uint32_t arraySlice = 0) = 0;

    // 紧缩资源以节省内存
    virtual void Compact() = 0;

    // 获取内存占用
    virtual uint64_t GetMemoryUsage() const = 0;

    // === 调试功能 ===

    // 调试纹理数据到文件
    virtual bool DebugSaveToFile(const std::string& filename,
                                uint32_t mipLevel = 0,
                                uint32_t arraySlice = 0) = 0;

    // 验证纹理内容
    virtual bool Validate() = 0;
};

} // namespace Prisma::Graphic