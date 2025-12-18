#pragma once

#include "RenderTypes.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;
class ITexture;

/// @brief 缓冲区映射描述
struct BufferMapDesc {
    void* data = nullptr;      // 映射的内存指针
    uint64_t size = 0;         // 映射的大小
    uint64_t offset = 0;       // 映射的偏移量
};

/// @brief 缓冲区描述符类型
enum class BufferDescriptorType {
    ShaderResourceView,
    UnorderedAccessView,
    ConstantBufferView,
    VertexBufferView,
    IndexBufferView
};

/// @brief 缓冲区视图描述
struct BufferViewDesc {
    uint64_t offset = 0;       // 视图偏移量
    uint64_t size = 0;         // 视图大小
    uint32_t firstElement = 0; // 第一个元素索引
    uint32_t numElements = 0;  // 元素数量
    uint32_t stride = 0;       // 元素步长
};

/// @brief 缓冲区抽象接口
class IBuffer {
public:
    virtual ~IBuffer() = default;

    /// @brief 获取缓冲区类型
    /// @return 缓冲区类型
    virtual BufferType GetBufferType() const = 0;

    /// @brief 获取缓冲区大小（字节）
    /// @return 缓冲区大小
    virtual uint64_t GetSize() const = 0;

    /// @brief 获取元素步长
    /// @return 元素步长
    virtual uint32_t GetStride() const = 0;

    /// @brief 获取使用标记
    /// @return 使用标记
    virtual BufferUsage GetUsage() const = 0;

    /// @brief 获取元素数量
    /// @return 元素数量
    virtual uint32_t GetElementCount() const = 0;

    /// @brief 检查是否为动态缓冲区
    /// @return 是否为动态缓冲区
    virtual bool IsDynamic() const = 0;

    /// @brief 检查是否为只读缓冲区
    /// @return 是否为只读缓冲区
    virtual bool IsReadOnly() const = 0;

    /// @brief 检查是否支持着色器资源访问
    /// @return 是否支持着色器资源访问
    virtual bool IsShaderResource() const = 0;

    /// @brief 检查是否支持无序访问
    /// @return 是否支持无序访问
    virtual bool IsUnorderedAccess() const = 0;

    // === 数据操作 ===

    /// @brief 映射缓冲区数据用于CPU访问
    /// @param offset 映射偏移量
    /// @param size 映射大小
    /// @param mapType 映射类型（0=write_discard, 1=write_no_overwrite, 2=read）
    /// @return 映射描述
    virtual BufferMapDesc Map(uint64_t offset = 0, uint64_t size = 0, uint32_t mapType = 0) = 0;

    /// @brief 取消映射缓冲区数据
    /// @param offset 取消映射的偏移量
    /// @param size 取消映射的大小
    virtual void Unmap(uint64_t offset = 0, uint64_t size = 0) = 0;

    /// @brief 更新缓冲区数据
    /// @param data 数据指针
    /// @param size 数据大小
    /// @param offset 缓冲区偏移量
    virtual void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) = 0;

    /// @brief 读取缓冲区数据
    /// @param dstBuffer 目标缓冲区
    /// @param size 读取大小
    /// @param offset 源缓冲区偏移量
    /// @return 是否成功
    virtual bool ReadData(void* dstBuffer, uint64_t size, uint64_t offset = 0) = 0;

    /// @brief 复制数据到另一个缓冲区
    /// @param dstBuffer 目标缓冲区
    /// @param srcOffset 源偏移量
    /// @param dstOffset 目标偏移量
    /// @param size 复制大小
    virtual void CopyTo(IBuffer* dstBuffer,
                       uint64_t srcOffset = 0,
                       uint64_t dstOffset = 0,
                       uint64_t size = 0) = 0;

    /// @brief 填充缓冲区
    /// @param value 填充值（4字节）
    /// @param offset 偏移量
    /// @param size 填充大小
    virtual void Fill(uint32_t value, uint64_t offset = 0, uint64_t size = 0) = 0;

    /// @brief 从纹理复制数据到缓冲区
    /// @param srcTexture 源纹理
    /// @param srcMipLevel 源MIP级别
    /// @param srcArraySlice 源数组切片
    virtual void CopyFromTexture(ITexture* srcTexture,
                                uint32_t srcMipLevel = 0,
                                uint32_t srcArraySlice = 0) = 0;

    /// @brief 复制数据到纹理
    /// @param dstTexture 目标纹理
    /// @param dstMipLevel 目标MIP级别
    /// @param dstArraySlice 目标数组切片
    virtual void CopyToTexture(ITexture* dstTexture,
                              uint32_t dstMipLevel = 0,
                              uint32_t dstArraySlice = 0) = 0;

    // === 视图操作 ===

    /// @brief 创建缓冲区视图
    /// @param descType 描述符类型
    /// @param desc 视图描述
    /// @return 描述符句柄
    virtual uint64_t CreateView(BufferDescriptorType descType, const BufferViewDesc& desc = {}) = 0;

    /// @brief 获取默认着色器资源视图
    /// @return 描述符句柄
    virtual uint64_t GetDefaultSRV() const = 0;

    /// @brief 获取默认无序访问视图
    /// @return 描述符句柄
    virtual uint64_t GetDefaultUAV() const = 0;

    /// @brief 获取默认常量缓冲区视图
    /// @return 描述符句柄
    virtual uint64_t GetDefaultCBV() const = 0;

    /// @brief 获取默认顶点缓冲区视图
    /// @return 描述符句柄
    virtual uint64_t GetDefaultVBV() const = 0;

    /// @brief 获取默认索引缓冲区视图
    /// @return 描述符句柄
    virtual uint64_t GetDefaultIBV() const = 0;

    // === 动态缓冲区操作 ===

    /// @brief 分配动态缓冲区空间
    /// @param size 需要的大小
    /// @param alignment 对齐要求
    /// @return 分配的偏移量
    virtual uint64_t AllocateDynamic(uint64_t size, uint64_t alignment = 256) = 0;

    /// @brief 重置动态缓冲区分配器
    virtual void ResetDynamicAllocation() = 0;

    /// @brief 获取当前动态缓冲区偏移量
    /// @return 当前偏移量
    virtual uint64_t GetCurrentDynamicOffset() const = 0;

    /// @brief 获取可用动态缓冲区空间
    /// @return 可用空间
    virtual uint64_t GetAvailableDynamicSpace() const = 0;

    // === 调试功能 ===

    /// @brief 调试缓冲区内容到文件
    /// @param filename 文件名
    /// @param format 数据格式（hex, float, int等）
    /// @param offset 数据偏移量
    /// @param size 数据大小
    /// @return 是否成功
    virtual bool DebugSaveToFile(const std::string& filename,
                                const std::string& format = "hex",
                                uint64_t offset = 0,
                                uint64_t size = 0) = 0;

    /// @brief 验证缓冲区内容
    /// @param expectedData 期望的数据
    /// @param size 数据大小
    /// @param offset 偏移量
    /// @return 是否匹配
    virtual bool DebugValidateContent(const void* expectedData,
                                      uint64_t size,
                                      uint64_t offset = 0) = 0;

    /// @brief 打印缓冲区信息
    virtual void DebugPrintInfo() const = 0;

    // === 内存管理 ===

    /// @brief 丢弃资源内容
    /// @param offset 丢弃偏移量
    /// @param size 丢弃大小
    virtual void Discard(uint64_t offset = 0, uint64_t size = 0) = 0;

    /// @brief 预分配内存
    /// @param size 预分配大小
    virtual void Reserve(uint64_t size) = 0;

    /// @brief 压缩缓冲区以节省内存
    virtual void Compact() = 0;

    /// @brief 获取内存占用
    /// @return 内存占用（字节）
    virtual uint64_t GetMemoryUsage() const = 0;

    /// @brief 获取GPU内存占用
    /// @return GPU内存占用（字节）
    virtual uint64_t GetGPUMemoryUsage() const = 0;
};

} // namespace PrismaEngine::Graphic