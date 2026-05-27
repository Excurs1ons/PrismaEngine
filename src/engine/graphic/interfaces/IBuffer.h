#pragma once

#include "RenderTypes.h"
#include "IResource.h"
#include <memory>

namespace Prisma::Graphic {

// 前置声明
class IRenderDevice;
class ITexture;

// 缓冲区映射描述
struct BufferMapDesc {
    void* data = nullptr;      // 映射的内存指针
    uint64_t size = 0;         // 映射的大小
    uint64_t offset = 0;       // 映射的偏移量
};

// 缓冲区描述符类型
enum class BufferDescriptorType {
    ShaderResourceView,
    UnorderedAccessView,
    ConstantBufferView,
    VertexBufferView,
    IndexBufferView
};

// 缓冲区视图描述
struct BufferViewDesc {
    uint64_t offset = 0;       // 视图偏移量
    uint64_t size = 0;         // 视图大小
    uint32_t firstElement = 0; // 第一个元素索引
    uint32_t numElements = 0;  // 元素数量
    uint32_t stride = 0;       // 元素步长
};

// 缓冲区抽象接口
class IBuffer : public IResource {
public:
    virtual ~IBuffer() = default;

    // 获取缓冲区类型
    virtual BufferType GetBufferType() const = 0;

    // 获取缓冲区大小（字节）
    virtual uint64_t GetSize() const = 0;

    // 获取元素步长
    virtual uint32_t GetStride() const = 0;

    // 获取使用标记
    virtual BufferUsage GetUsage() const = 0;

    // 获取元素数量
    virtual uint32_t GetElementCount() const = 0;

    // 检查是否为动态缓冲区
    virtual bool IsDynamic() const = 0;

    // 检查是否为只读缓冲区
    virtual bool IsReadOnly() const = 0;

    // 检查是否支持着色器资源访问
    virtual bool IsShaderResource() const = 0;

    // 检查是否支持无序访问
    virtual bool IsUnorderedAccess() const = 0;

    // === 数据操作 ===

    // 映射缓冲区数据用于CPU访问
    virtual BufferMapDesc Map(uint64_t offset = 0, uint64_t size = 0, uint32_t mapType = 0) = 0;

    // 取消映射缓冲区数据
    virtual void Unmap(uint64_t offset = 0, uint64_t size = 0) = 0;

    // 更新缓冲区数据
    virtual void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) = 0;

    // 读取缓冲区数据
    virtual bool ReadData(void* dstBuffer, uint64_t size, uint64_t offset = 0) = 0;

    // 复制数据到另一个缓冲区

    virtual void CopyTo(IBuffer* dstBuffer,
                       uint64_t srcOffset = 0,
                       uint64_t dstOffset = 0,
                       uint64_t size = 0) = 0;

    // 从另一个缓冲区复制数据
    virtual void CopyFromBuffer(IBuffer* srcBuffer,
                                 uint64_t srcOffset = 0,
                                 uint64_t dstOffset = 0,
                                 uint64_t size = 0) = 0;

    // 填充缓冲区
    virtual void Fill(uint32_t value, uint64_t offset = 0, uint64_t size = 0) = 0;

    // 从纹理复制数据到缓冲区
    virtual void CopyFromTexture(ITexture* srcTexture,
                                uint32_t srcMipLevel = 0,
                                uint32_t srcArraySlice = 0) = 0;

    // 复制数据到纹理
    virtual void CopyToTexture(ITexture* dstTexture,
                              uint32_t dstMipLevel = 0,
                              uint32_t dstArraySlice = 0) = 0;

    // === 视图操作 ===

    // 创建缓冲区视图
    virtual uint64_t CreateView(BufferDescriptorType descType, const BufferViewDesc& desc = {}) = 0;

    // 获取默认着色器资源视图
    virtual uint64_t GetDefaultSRV() const = 0;

    // 获取默认无序访问视图
    virtual uint64_t GetDefaultUAV() const = 0;

    // 获取默认常量缓冲区视图
    virtual uint64_t GetDefaultCBV() const = 0;

    // 获取默认顶点缓冲区视图
    virtual uint64_t GetDefaultVBV() const = 0;

    // 获取默认索引缓冲区视图
    virtual uint64_t GetDefaultIBV() const = 0;

    // === 动态缓冲区操作 ===

    // 分配动态缓冲区空间
    virtual uint64_t AllocateDynamic(uint64_t size, uint64_t alignment = 256) = 0;

    // 重置动态缓冲区分配器
    virtual void ResetDynamicAllocation() = 0;

    // 获取当前动态缓冲区偏移量
    virtual uint64_t GetCurrentDynamicOffset() const = 0;

    // 获取可用动态缓冲区空间
    virtual uint64_t GetAvailableDynamicSpace() const = 0;

    // === 调试功能 ===

    // 调试缓冲区内容到文件
    virtual bool DebugSaveToFile(const std::string& filename,
                                const std::string& format = "hex",
                                uint64_t offset = 0,
                                uint64_t size = 0) = 0;

    // 验证缓冲区内容
    virtual bool DebugValidateContent(const void* expectedData,
                                      uint64_t size,
                                      uint64_t offset = 0) = 0;

    // 打印缓冲区信息
    virtual void DebugPrintInfo() const = 0;

    // === 内存管理 ===

    // 丢弃资源内容
    virtual void Discard(uint64_t offset = 0, uint64_t size = 0) = 0;

    // 预分配内存
    virtual void Reserve(uint64_t size) = 0;

    // 压缩缓冲区以节省内存
    virtual void Compact() = 0;

    // 获取内存占用
    virtual uint64_t GetMemoryUsage() const = 0;

    // 获取GPU内存占用
    virtual uint64_t GetGPUMemoryUsage() const = 0;
};

} // namespace Prisma::Graphic