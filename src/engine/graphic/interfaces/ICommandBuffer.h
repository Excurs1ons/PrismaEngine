#pragma once

#include "RenderTypes.h"
#include "IPipelineState.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IPipeline;
class IBuffer;
class ITexture;
class ISampler;
class IFence;

/// @brief 渲染通道描述
struct RenderPassDesc {
    ITexture* renderTarget = nullptr;
    ITexture* depthStencil = nullptr;
    Color clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    float clearDepth = 1.0f;
    uint8_t clearStencil = 0;
    Rect renderArea = {0, 0, 0, 0};
    bool clearRenderTarget = true;
    bool clearDepth = true;
    bool clearStencil = true;
};

/// @brief 命令缓冲区抽象接口
/// 提供后端无关的命令记录接口
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    // === 生命周期管理 ===

    /// @brief 开始记录命令
    virtual void Begin() = 0;

    /// @brief 结束记录命令
    virtual void End() = 0;

    /// @brief 重置命令缓冲区
    virtual void Reset() = 0;

    // === 渲染通道 ===

    /// @brief 开始渲染通道
    /// @param desc 渲染通道描述
    virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;

    /// @brief 结束渲染通道
    virtual void EndRenderPass() = 0;

    // === 管线状态 ===

    /// @brief 设置渲染管线状态对象(PSO)
    /// @param pipelineState 管线状态对象
    virtual void SetPipelineState(IPipelineState* pipelineState) = 0;

    // === 资源绑定 ===

    /// @brief 设置顶点缓冲区
    /// @param buffer 顶点缓冲区
    /// @param slot 缓冲区槽位
    /// @param offset 偏移量（字节）
    /// @param stride 顶点步长
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset = 0, uint32_t stride = 0) = 0;

    /// @brief 设置索引缓冲区
    /// @param buffer 索引缓冲区
    /// @param format 索引格式（16位或32位）
    /// @param offset 偏移量（字节）
    virtual void SetIndexBuffer(IBuffer* buffer, bool is32Bit = true, uint32_t offset = 0) = 0;

    /// @brief 设置常量缓冲区
    /// @param buffer 常量缓冲区
    /// @param slot 着色器寄存器槽位
    /// @param offset 偏移量（字节）
    /// @param size 大小（字节）
    virtual void SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset = 0, uint32_t size = 0) = 0;

    /// @brief 设置纹理
    /// @param texture 纹理
    /// @param slot 着色器寄存器槽位
    virtual void SetTexture(ITexture* texture, uint32_t slot) = 0;

    /// @brief 设置采样器
    /// @param sampler 采样器
    /// @param slot 着色器寄存器槽位
    virtual void SetSampler(ISampler* sampler, uint32_t slot) = 0;

    /// @brief 设置着色器资源视图（SRV）
    /// @param buffer 缓冲区
    /// @param slot 着色器寄存器槽位
    virtual void SetShaderResource(IBuffer* buffer, uint32_t slot) = 0;

    /// @brief 设置无序访问视图（UAV）
    /// @param buffer 缓冲区
    /// @param slot 着色器寄存器槽位
    virtual void SetUnorderedAccess(IBuffer* buffer, uint32_t slot) = 0;

    // === 视口和裁剪矩形 ===

    /// @brief 设置视口
    /// @param viewport 视口描述
    virtual void SetViewport(const Viewport& viewport) = 0;

    /// @brief 设置多个视口
    /// @param viewports 视口数组
    /// @param count 视口数量
    virtual void SetViewports(const Viewport* viewports, uint32_t count) = 0;

    /// @brief 设置裁剪矩形
    /// @param rect 裁剪矩形
    virtual void SetScissorRect(const Rect& rect) = 0;

    /// @brief 设置多个裁剪矩形
    /// @param rects 裁剪矩形数组
    /// @param count 矩形数量
    virtual void SetScissorRects(const Rect* rects, uint32_t count) = 0;

    // === 渲染原语 ===

    /// @brief 绘制非索引图元
    /// @param vertexCount 顶点数量
    /// @param startVertex 起始顶点索引
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;

    /// @brief 绘制索引图元
    /// @param indexCount 索引数量
    /// @param startIndex 起始索引
    /// @param baseVertex 顶点基址偏移
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;

    /// @brief 实例化绘制非索引图元
    /// @param vertexCount 每个实例的顶点数量
    /// @param instanceCount 实例数量
    /// @param startVertex 起始顶点索引
    /// @param startInstance 起始实例索引
    virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                              uint32_t startVertex = 0, uint32_t startInstance = 0) = 0;

    /// @brief 实例化绘制索引图元
    /// @param indexCount 每个实例的索引数量
    /// @param instanceCount 实例数量
    /// @param startIndex 起始索引
    /// @param baseVertex 顶点基址偏移
    /// @param startInstance 起始实例索引
    virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                     uint32_t startIndex = 0, int32_t baseVertex = 0,
                                     uint32_t startInstance = 0) = 0;

    /// @brief 间接绘制（非索引）
    /// @param indirectBuffer 间接参数缓冲区
    /// @param offset 缓冲区偏移量
    virtual void DrawIndirect(IBuffer* indirectBuffer, uint32_t offset = 0) = 0;

    /// @brief 间接绘制（索引）
    /// @param indirectBuffer 间接参数缓冲区
    /// @param offset 缓冲区偏移量
    virtual void DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset = 0) = 0;

    // === 计算着色器 ===

    /// @brief 分派计算着色器
    /// @param x X方向线程组数量
    /// @param y Y方向线程组数量
    /// @param z Z方向线程组数量
    virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;

    /// @brief 间接分派计算着色器
    /// @param indirectBuffer 间接参数缓冲区
    /// @param offset 缓冲区偏移量
    virtual void DispatchIndirect(IBuffer* indirectBuffer, uint32_t offset = 0) = 0;

    // === 资源操作 ===

    /// @brief 复制缓冲区
    /// @param dst 目标缓冲区
    /// @param src 源缓冲区
    virtual void CopyBuffer(IBuffer* dst, IBuffer* src) = 0;

    /// @brief 复制缓冲区区域
    /// @param dst 目标缓冲区
    /// @param dstOffset 目标偏移
    /// @param src 源缓冲区
    /// @param srcOffset 源偏移
    /// @param size 复制大小
    virtual void CopyBufferRegion(IBuffer* dst, uint64_t dstOffset,
                                 IBuffer* src, uint64_t srcOffset,
                                 uint64_t size) = 0;

    /// @brief 复制纹理
    /// @param dst 目标纹理
    /// @param src 源纹理
    virtual void CopyTexture(ITexture* dst, ITexture* src) = 0;

    /// @brief 更新缓冲区数据
    /// @param buffer 目标缓冲区
    /// @param data 数据指针
    /// @param size 数据大小
    /// @param offset 缓冲区偏移量
    virtual void UpdateBuffer(IBuffer* buffer, const void* data, uint64_t size, uint64_t offset = 0) = 0;

    /// @brief 更新纹理数据
    /// @param texture 目标纹理
    /// @param data 数据指针
    /// @param dataSize 数据大小
    /// @param mipLevel MIP层级
    /// @param arraySlice 数组切片
    virtual void UpdateTexture(ITexture* texture, const void* data, uint64_t dataSize,
                              uint32_t mipLevel = 0, uint32_t arraySlice = 0) = 0;

    // === 屏障 ===

    /// @brief 插入内存屏障
    /// @param src 访问前状态
    /// @param dst 访问后状态
    virtual void MemoryBarrier() = 0;

    /// @brief UAV屏障（仅DirectX12需要）
    virtual void UAVBarrier() = 0;

    // === 查询 ===

    /// @brief 开始时间戳查询
    /// @param queryPool 查询池
    /// @param queryIndex 查询索引
    virtual void BeginTimestampQuery(void* queryPool, uint32_t queryIndex) = 0;

    /// @brief 结束时间戳查询
    /// @param queryPool 查询池
    /// @param queryIndex 查询索引
    virtual void EndTimestampQuery(void* queryPool, uint32_t queryIndex) = 0;

    /// @brief 解析查询数据
    /// @param dstBuffer 目标缓冲区
    /// @param queryPool 查询池
    /// @param startQuery 起始查询
    /// @param queryCount 查询数量
    virtual void ResolveQueryData(IBuffer* dstBuffer, void* queryPool,
                                 uint32_t startQuery, uint32_t queryCount) = 0;

    // === 调试 ===

    /// @brief 插入调试标记
    /// @param name 标记名称
    virtual void InsertDebugMarker(const std::string& name) = 0;

    /// @brief 开始调试组
    /// @param name 组名称
    virtual void BeginDebugGroup(const std::string& name) = 0;

    /// @brief 结束调试组
    virtual void EndDebugGroup() = 0;
};

} // namespace PrismaEngine::Graphic