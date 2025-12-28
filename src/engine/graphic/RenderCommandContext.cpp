#include "RenderCommandContext.h"
#include "interfaces/IRenderTarget.h"
#include "interfaces/IPipelineState.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "interfaces/ISampler.h"

namespace PrismaEngine::Graphic {

RenderCommandContext::RenderCommandContext() = default;

RenderCommandContext::~RenderCommandContext() = default;

// === IDeviceContext 接口实现 ===

void RenderCommandContext::SetRenderTarget(IRenderTarget* renderTarget) {
    m_stateCache.currentRenderTarget = renderTarget;
    m_nativeRenderTarget = renderTarget ? renderTarget->GetNativeHandle() : nullptr;
}

void RenderCommandContext::SetRenderTarget(IRenderTarget* renderTarget, IDepthStencil* depthStencil) {
    m_stateCache.currentRenderTarget = renderTarget;
    m_stateCache.currentDepthStencil = depthStencil;
    m_nativeRenderTarget = renderTarget ? renderTarget->GetNativeHandle() : nullptr;
    m_nativeDepthStencil = depthStencil ? depthStencil->GetNativeHandle() : nullptr;
}

void RenderCommandContext::SetRenderTargets(IRenderTarget** renderTargets, uint32_t count, IDepthStencil* depthStencil) {
    if (count > 0 && renderTargets != nullptr) {
        m_stateCache.currentRenderTarget = renderTargets[0];
        m_nativeRenderTarget = renderTargets[0] ? renderTargets[0]->GetNativeHandle() : nullptr;
    }
    m_stateCache.currentDepthStencil = depthStencil;
    m_nativeDepthStencil = depthStencil ? depthStencil->GetNativeHandle() : nullptr;
}

void RenderCommandContext::SetViewport(float x, float y, float width, float height) {
    m_stateCache.currentViewport = {x, y, width, height, 0.0f, 1.0f};
}

void RenderCommandContext::SetViewports(const Viewport* viewports, uint32_t count) {
    if (count > 0 && viewports != nullptr) {
        m_stateCache.currentViewport = viewports[0];
    }
}

void RenderCommandContext::SetScissorRect(const Rect& rect) {
    m_stateCache.currentScissor = rect;
}

void RenderCommandContext::SetScissorRects(const Rect* rects, uint32_t count) {
    if (count > 0 && rects != nullptr) {
        m_stateCache.currentScissor = rects[0];
    }
}

void RenderCommandContext::SetPipelineState(IPipelineState* pipelineState) {
    m_stateCache.currentPipelineState = pipelineState;
}

void RenderCommandContext::SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t stride) {
    if (slot < 16) {
        m_stateCache.currentVertexBuffers[slot] = buffer;
    }
}

void RenderCommandContext::SetIndexBuffer(IBuffer* buffer, uint32_t offset, bool is32Bit) {
    m_stateCache.currentIndexBuffer = buffer;
}

void RenderCommandContext::SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size) {
    // 实现常量缓冲区绑定
    (void)buffer; (void)slot; (void)offset; (void)size;
}

void RenderCommandContext::SetTexture(ITexture* texture, uint32_t slot) {
    if (slot < 16) {
        m_stateCache.currentTextures[slot] = texture;
    }
}

void RenderCommandContext::SetSampler(ISampler* sampler, uint32_t slot) {
    if (slot < 16) {
        m_stateCache.currentSamplers[slot] = sampler;
    }
}

void RenderCommandContext::SetVertexData(const void* data, uint32_t size, uint32_t stride) {
    // 动态上传顶点数据
    (void)data; (void)size; (void)stride;
}

void RenderCommandContext::SetIndexData(const void* data, uint32_t size, bool is32Bit) {
    // 动态上传索引数据
    (void)data; (void)size; (void)is32Bit;
}

void RenderCommandContext::SetConstantData(uint32_t slot, const void* data, uint32_t size) {
    // 动态上传常量数据
    (void)slot; (void)data; (void)size;
}

void RenderCommandContext::Draw(uint32_t vertexCount, uint32_t startVertex) {
    (void)vertexCount; (void)startVertex;
}

void RenderCommandContext::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) {
    (void)indexCount; (void)startIndex; (void)baseVertex;
}

void RenderCommandContext::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                                     uint32_t startVertex, uint32_t startInstance) {
    (void)vertexCount; (void)instanceCount; (void)startVertex; (void)startInstance;
}

void RenderCommandContext::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                              uint32_t startIndex, int32_t baseVertex,
                                              uint32_t startInstance) {
    (void)indexCount; (void)instanceCount; (void)startIndex; (void)baseVertex; (void)startInstance;
}

void RenderCommandContext::ClearRenderTarget(IRenderTarget* renderTarget, const float color[4]) {
    (void)renderTarget; (void)color;
}

void RenderCommandContext::ClearRenderTarget(IRenderTarget* renderTarget, float r, float g, float b, float a) {
    (void)renderTarget; (void)r; (void)g; (void)b; (void)a;
}

void RenderCommandContext::ClearDepthStencil(IDepthStencil* depthStencil, float depth, uint8_t stencil) {
    (void)depthStencil; (void)depth; (void)stencil;
}

void RenderCommandContext::MemoryBarrier() {
}

void RenderCommandContext::UAVBarrier() {
}

void RenderCommandContext::BeginDebugMarker(const std::string& name) {
    (void)name;
}

void RenderCommandContext::EndDebugMarker() {
}

void RenderCommandContext::InsertDebugMarker(const std::string& name) {
    (void)name;
}

// === 兼容旧 API 的方法（待废弃） ===

void RenderCommandContext::SetConstantBuffer(const std::string& name, const PrismaMath::mat4& matrix) {
    m_namedResources[name] = nullptr;
    SetConstantData(0, &matrix, sizeof(PrismaMath::mat4));
}

void RenderCommandContext::SetConstantBuffer(const std::string& name, const float* data, size_t size) {
    m_namedResources[name] = nullptr;
    SetConstantData(0, data, static_cast<uint32_t>(size));
}

void RenderCommandContext::SetVertexBuffer(const void* data, uint32_t sizeInBytes, uint32_t strideInBytes) {
    SetVertexData(data, sizeInBytes, strideInBytes);
}

void RenderCommandContext::SetIndexBuffer(const void* data, uint32_t sizeInBytes, bool use16BitIndices) {
    SetIndexData(data, sizeInBytes, !use16BitIndices);
}

void RenderCommandContext::SetShaderResource(const std::string& name, void* resource) {
    m_namedResources[name] = resource;
}

void RenderCommandContext::SetSampler(const std::string& name, void* sampler) {
    m_namedResources[name] = sampler;
}

void RenderCommandContext::SetPipelineState(void* pso) {
    (void)pso;
}

void RenderCommandContext::SetNativeRenderTarget(void* renderTarget) {
    m_nativeRenderTarget = renderTarget;
}

void RenderCommandContext::SetNativeDepthStencil(void* depthStencil) {
    m_nativeDepthStencil = depthStencil;
}

} // namespace PrismaEngine::Graphic
