#pragma once

#include "RenderTypes.h"
#include "ITexture.h"
#include "IBuffer.h"
#include "ISampler.h"
#include <string>
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderTarget;
class IDepthStencil;
class IPipelineState;

/// @brief 设备上下文抽象接口
/// 提供图形API无关的命令执行接口，用于逻辑 Pass 记录渲染命令
class IDeviceContext {
public:
    virtual ~IDeviceContext() = default;

    // === 渲染目标 ===

    /// @brief 设置渲染目标
    /// @param renderTarget 渲染目标接口指针
    virtual void SetRenderTarget(IRenderTarget* renderTarget) = 0;

    /// @brief 设置渲染目标和深度模板
    /// @param renderTarget 渲染目标接口指针
    /// @param depthStencil 深度模板接口指针
    virtual void SetRenderTarget(IRenderTarget* renderTarget, IDepthStencil* depthStencil) = 0;

    /// @brief 设置多个渲染目标（MRT）
    /// @param renderTargets 渲染目标数组
    /// @param count 渲染目标数量
    /// @param depthStencil 深度模板接口指针
    virtual void SetRenderTargets(IRenderTarget** renderTargets, uint32_t count, IDepthStencil* depthStencil) = 0;

    // === 视口和裁剪 ===

    /// @brief 设置视口
    /// @param x 左上角 X 坐标
    /// @param y 左上角 Y 坐标
    /// @param width 宽度
    /// @param height 高度
    virtual void SetViewport(float x, float y, float width, float height) = 0;

    /// @brief 设置多个视口
    /// @param viewports 视口数组
    /// @param count 视口数量
    virtual void SetViewports(const Viewport* viewports, uint32_t count) = 0;

    /// @brief 设置裁剪矩形
    /// @param rect 裁剪矩形
    virtual void SetScissorRect(const Rect& rect) = 0;

    /// @brief 设置多个裁剪矩形
    /// @param rects 裁剪矩形数组
    /// @param count 裁剪矩形数量
    virtual void SetScissorRects(const Rect* rects, uint32_t count) = 0;

    // === 管线状态 ===

    /// @brief 设置管线状态
    /// @param pipelineState 管线状态对象
    virtual void SetPipelineState(IPipelineState* pipelineState) = 0;

    // === 资源绑定 ===

    /// @brief 设置顶点缓冲区
    /// @param buffer 缓冲区接口指针
    /// @param slot 槽位
    /// @param offset 偏移量（字节）
    /// @param stride 步长（字节）
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t stride) = 0;

    /// @brief 设置索引缓冲区
    /// @param buffer 缓冲区接口指针
    /// @param offset 偏移量（字节）
    /// @param is32Bit 是否使用 32 位索引
    virtual void SetIndexBuffer(IBuffer* buffer, uint32_t offset, bool is32Bit) = 0;

    /// @brief 设置常量缓冲区
    /// @param buffer 缓冲区接口指针
    /// @param slot 槽位
    /// @param offset 偏移量（字节）
    /// @param size 数据大小
    virtual void SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size) = 0;

    /// @brief 设置纹理
    /// @param texture 纹理接口指针
    /// @param slot 槽位
    virtual void SetTexture(ITexture* texture, uint32_t slot) = 0;

    /// @brief 设置采样器
    /// @param sampler 采样器接口指针
    /// @param slot 槽位
    virtual void SetSampler(ISampler* sampler, uint32_t slot) = 0;

    // === 直接数据设置（用于动态数据） ===

    /// @brief 设置顶点数据（动态上传）
    /// @param data 数据指针
    /// @param size 数据大小（字节）
    /// @param stride 步长（字节）
    virtual void SetVertexData(const void* data, uint32_t size, uint32_t stride) = 0;

    /// @brief 设置索引数据（动态上传）
    /// @param data 数据指针
    /// @param size 数据大小（字节）
    /// @param is32Bit 是否使用 32 位索引
    virtual void SetIndexData(const void* data, uint32_t size, bool is32Bit) = 0;

    /// @brief 设置常量数据（动态上传）
    /// @param slot 槽位
    /// @param data 数据指针
    /// @param size 数据大小（字节）
    virtual void SetConstantData(uint32_t slot, const void* data, uint32_t size) = 0;

    // === 渲染原语 ===

    /// @brief 绘制
    /// @param vertexCount 顶点数量
    /// @param startVertex 起始顶点
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;

    /// @brief 绘制索引
    /// @param indexCount 索引数量
    /// @param startIndex 起始索引
    /// @param baseVertex 基础顶点偏移
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;

    /// @brief 实例化绘制
    /// @param vertexCount 顶点数量
    /// @param instanceCount 实例数量
    /// @param startVertex 起始顶点
    /// @param startInstance 起始实例
    virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                             uint32_t startVertex = 0, uint32_t startInstance = 0) = 0;

    /// @brief 实例化索引绘制
    /// @param indexCount 索引数量
    /// @param instanceCount 实例数量
    /// @param startIndex 起始索引
    /// @param baseVertex 基础顶点偏移
    /// @param startInstance 起始实例
    virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                    uint32_t startIndex = 0, int32_t baseVertex = 0,
                                    uint32_t startInstance = 0) = 0;

    // === 清除操作 ===

    /// @brief 清除渲染目标
    /// @param renderTarget 渲染目标接口指针
    /// @param color 清除颜色
    virtual void ClearRenderTarget(IRenderTarget* renderTarget, const float color[4]) = 0;

    /// @brief 清除渲染目标（浮点数组）
    /// @param renderTarget 渲染目标接口指针
    /// @param r 红色分量
    /// @param g 绿色分量
    /// @param b 蓝色分量
    /// @param a Alpha 分量
    virtual void ClearRenderTarget(IRenderTarget* renderTarget, float r, float g, float b, float a) = 0;

    /// @brief 清除深度模板
    /// @param depthStencil 深度模板接口指针
    /// @param depth 深度值
    /// @param stencil 模板值
    virtual void ClearDepthStencil(IDepthStencil* depthStencil, float depth, uint8_t stencil) = 0;

    // === 屏障 ===

    /// @brief 执行内存屏障
    virtual void MemoryBarrier() = 0;

    /// @brief 执行 UAV 屏障
    virtual void UAVBarrier() = 0;

    // === 调试 ===

    /// @brief 开始调试标记
    /// @param name 标记名称
    virtual void BeginDebugMarker(const std::string& name) = 0;

    /// @brief 结束调试标记
    virtual void EndDebugMarker() = 0;

    /// @brief 插入调试标记
    /// @param name 标记名称
    virtual void InsertDebugMarker(const std::string& name) = 0;
};

} // namespace PrismaEngine::Graphic
