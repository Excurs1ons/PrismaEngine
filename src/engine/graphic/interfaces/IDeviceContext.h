#pragma once

#include "RenderTypes.h"
#include "ITexture.h"
#include "IBuffer.h"
#include "ISampler.h"
#include <string>
#include <memory>

namespace Prisma::Graphic {

// 前置声明
class IRenderTarget;
class IDepthStencil;
class IPipelineState;

// 设备上下文抽象接口
/// 提供图形API无关的命令执行接口，用于逻辑 Pass 记录渲染命令
class IDeviceContext {
public:
    virtual ~IDeviceContext() = default;

    // === 渲染目标 ===

    // 设置渲染目标
    virtual void SetRenderTarget(IRenderTarget* renderTarget) = 0;

    // 设置渲染目标和深度模板
    virtual void SetRenderTarget(IRenderTarget* renderTarget, IDepthStencil* depthStencil) = 0;

    // 设置多个渲染目标（MRT）
    virtual void SetRenderTargets(IRenderTarget** renderTargets, uint32_t count, IDepthStencil* depthStencil) = 0;

    // === 视口和裁剪 ===

    // 设置视口
    virtual void SetViewport(float x, float y, float width, float height) = 0;

    // 设置多个视口
    virtual void SetViewports(const Viewport* viewports, uint32_t count) = 0;

    // 设置裁剪矩形
    virtual void SetScissorRect(const Rect& rect) = 0;

    // 设置多个裁剪矩形
    virtual void SetScissorRects(const Rect* rects, uint32_t count) = 0;

    // === 管线状态 ===

    // 设置管线状态
    virtual void SetPipelineState(IPipelineState* pipelineState) = 0;

    // === 资源绑定 ===

    // 设置顶点缓冲区
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t stride) = 0;

    // 设置索引缓冲区
    virtual void SetIndexBuffer(IBuffer* buffer, uint32_t offset, bool is32Bit) = 0;

    // 设置常量缓冲区
    virtual void SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size) = 0;

    // 设置纹理
    virtual void SetTexture(ITexture* texture, uint32_t slot) = 0;

    // 设置采样器
    virtual void SetSampler(ISampler* sampler, uint32_t slot) = 0;

    // === 直接数据设置（用于动态数据） ===

    // 设置顶点数据（动态上传）
    virtual void SetVertexData(const void* data, uint32_t size, uint32_t stride) = 0;

    // 设置索引数据（动态上传）
    virtual void SetIndexData(const void* data, uint32_t size, bool is32Bit) = 0;

    // 设置常量数据（动态上传）
    virtual void SetConstantData(uint32_t slot, const void* data, uint32_t size) = 0;

    // === 渲染原语 ===

    // 绘制
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex = 0) = 0;

    // 绘制索引
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) = 0;

    // 实例化绘制
    virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                             uint32_t startVertex = 0, uint32_t startInstance = 0) = 0;

    // 实例化索引绘制
    virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                    uint32_t startIndex = 0, int32_t baseVertex = 0,
                                    uint32_t startInstance = 0) = 0;

    // === 清除操作 ===

    // 清除渲染目标
    virtual void ClearRenderTarget(IRenderTarget* renderTarget, const float color[4]) = 0;

    // 清除渲染目标（浮点数组）
    virtual void ClearRenderTarget(IRenderTarget* renderTarget, float r, float g, float b, float a) = 0;

    // 清除深度模板
    virtual void ClearDepthStencil(IDepthStencil* depthStencil, float depth, uint8_t stencil) = 0;

    // === 屏障 ===

    // 执行内存屏障
    virtual void MemoryBarrier() = 0;

    // 执行 UAV 屏障
    virtual void UAVBarrier() = 0;

    // === 调试 ===

    // 开始调试标记
    virtual void BeginDebugMarker(const std::string& name) = 0;

    // 结束调试标记
    virtual void EndDebugMarker() = 0;

    // 插入调试标记
    virtual void InsertDebugMarker(const std::string& name) = 0;
};

} // namespace Prisma::Graphic
