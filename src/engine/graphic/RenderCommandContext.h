#pragma once

#include "interfaces/IDeviceContext.h"
#include "math/MathTypes.h"
#include <string>
#include <map>

namespace PrismaEngine::Graphic {

/// @brief 渲染命令上下文实现
/// 实现 IDeviceContext 接口，提供命令执行功能
/// 注意：这是一个临时适配器，后续应由具体后端（DX12/Vulkan）实现
class RenderCommandContext : public IDeviceContext {
public:
    RenderCommandContext();
    virtual ~RenderCommandContext();

    // === IDeviceContext 接口实现 ===

    // 渲染目标
    void SetRenderTarget(IRenderTarget* renderTarget) override;
    void SetRenderTarget(IRenderTarget* renderTarget, IDepthStencil* depthStencil) override;
    void SetRenderTargets(IRenderTarget** renderTargets, uint32_t count, IDepthStencil* depthStencil) override;

    // 视口和裁剪
    void SetViewport(float x, float y, float width, float height) override;
    void SetViewports(const Viewport* viewports, uint32_t count) override;
    void SetScissorRect(const Rect& rect) override;
    void SetScissorRects(const Rect* rects, uint32_t count) override;

    // 管线状态
    void SetPipelineState(IPipelineState* pipelineState) override;

    // 资源绑定
    void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t stride) override;
    void SetIndexBuffer(IBuffer* buffer, uint32_t offset, bool is32Bit) override;
    void SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size) override;
    void SetTexture(ITexture* texture, uint32_t slot) override;
    void SetSampler(ISampler* sampler, uint32_t slot) override;

    // 直接数据设置（动态上传）
    void SetVertexData(const void* data, uint32_t size, uint32_t stride) override;
    void SetIndexData(const void* data, uint32_t size, bool is32Bit) override;
    void SetConstantData(uint32_t slot, const void* data, uint32_t size) override;

    // 渲染原语
    void Draw(uint32_t vertexCount, uint32_t startVertex = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0) override;
    void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                       uint32_t startVertex = 0, uint32_t startInstance = 0) override;
    void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                              uint32_t startIndex = 0, int32_t baseVertex = 0,
                              uint32_t startInstance = 0) override;

    // 清除操作
    void ClearRenderTarget(IRenderTarget* renderTarget, const float color[4]) override;
    void ClearRenderTarget(IRenderTarget* renderTarget, float r, float g, float b, float a) override;
    void ClearDepthStencil(IDepthStencil* depthStencil, float depth, uint8_t stencil) override;

    // 屏障
    void MemoryBarrier() override;
    void UAVBarrier() override;

    // 调试
    void BeginDebugMarker(const std::string& name) override;
    void EndDebugMarker() override;
    void InsertDebugMarker(const std::string& name) override;

    // === 兼容旧 API 的方法（待废弃） ===

    /// @deprecated 使用 SetConstantData 替代
    void SetConstantBuffer(const std::string& name, const PrismaMath::mat4& matrix);

    /// @deprecated 使用 SetConstantData 替代
    void SetConstantBuffer(const std::string& name, const float* data, size_t size);

    /// @deprecated 使用 SetVertexData 替代
    void SetVertexBuffer(const void* data, uint32_t sizeInBytes, uint32_t strideInBytes);

    /// @deprecated 使用 SetIndexData 替代
    void SetIndexBuffer(const void* data, uint32_t sizeInBytes, bool use16BitIndices = true);

    /// @deprecated 使用 SetTexture 替代
    void SetShaderResource(const std::string& name, void* resource);

    /// @deprecated 使用 SetSampler 替代
    void SetSampler(const std::string& name, void* sampler);

    /// @deprecated 使用 SetPipelineState 替代
    void SetPipelineState(void* pso);

    /// @brief 设置原生渲染目标（用于兼容旧代码）
    /// @deprecated 使用 SetRenderTarget(IRenderTarget*) 替代
    void SetNativeRenderTarget(void* renderTarget);

    /// @brief 设置原生深度模板（用于兼容旧代码）
    /// @deprecated 使用 SetRenderTarget(..., IDepthStencil*) 替代
    void SetNativeDepthStencil(void* depthStencil);

private:
    // 当前状态缓存
    struct StateCache {
        IRenderTarget* currentRenderTarget = nullptr;
        IDepthStencil* currentDepthStencil = nullptr;
        IPipelineState* currentPipelineState = nullptr;
        IBuffer* currentVertexBuffers[16] = {nullptr};
        IBuffer* currentIndexBuffer = nullptr;
        ITexture* currentTextures[16] = {nullptr};
        ISampler* currentSamplers[16] = {nullptr};
        Viewport currentViewport = {0, 0, 0, 0, 0, 1};
        Rect currentScissor = {0, 0, 0, 0};
    } m_stateCache;

    // 原生资源句柄（用于兼容旧代码）
    void* m_nativeRenderTarget = nullptr;
    void* m_nativeDepthStencil = nullptr;

    // 命名资源缓存（用于兼容旧 API）
    std::map<std::string, void*> m_namedResources;
};

} // namespace PrismaEngine::Graphic
