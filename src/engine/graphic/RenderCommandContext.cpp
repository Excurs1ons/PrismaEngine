#include "RenderCommandContext.h"
#include "interfaces/IRenderTarget.h"
#include "interfaces/IPipelineState.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "interfaces/ISampler.h"

namespace Prisma::Graphic {

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
        m_stateCache.vertexBufferOffsets[slot] = offset;
        m_stateCache.vertexBufferStrides[slot] = stride;
    }
}

void RenderCommandContext::SetIndexBuffer(IBuffer* buffer, uint32_t offset, bool is32Bit) {
    m_stateCache.currentIndexBuffer = buffer;
    m_stateCache.indexBufferOffset = offset;
    m_stateCache.indexIs32Bit = is32Bit;
}

void RenderCommandContext::SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size) {
    if (slot < 16) {
        m_stateCache.currentConstantBuffers[slot] = buffer;
        m_stateCache.constantBufferOffsets[slot] = offset;
        m_stateCache.constantBufferSize[slot] = size;
    }
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
    const auto* begin = static_cast<const uint8_t*>(data);
    if (!begin || size == 0) {
        m_dynamicVertexData.clear();
        m_lastVertexStride = stride;
        return;
    }

    m_dynamicVertexData.assign(begin, begin + size);
    m_lastVertexStride = stride;
}

void RenderCommandContext::SetIndexData(const void* data, uint32_t size, bool is32Bit) {
    const auto* begin = static_cast<const uint8_t*>(data);
    if (!begin || size == 0) {
        m_dynamicIndexData.clear();
        m_lastIndexBufferIs32Bit = is32Bit;
        return;
    }

    m_dynamicIndexData.assign(begin, begin + size);
    m_lastIndexBufferIs32Bit = is32Bit;
}

void RenderCommandContext::SetConstantData(uint32_t slot, const void* data, uint32_t size) {
    if (slot >= m_dynamicConstantData.size()) {
        return;
    }

    const auto* begin = static_cast<const uint8_t*>(data);
    if (!begin || size == 0) {
        m_dynamicConstantData[slot].clear();
        return;
    }

    m_dynamicConstantData[slot].assign(begin, begin + size);
}

void RenderCommandContext::Draw(uint32_t vertexCount, uint32_t startVertex) {
    if (vertexCount == 0) {
        return;
    }

    m_namedResources["__last_draw_start_vertex"] = reinterpret_cast<void*>(static_cast<uintptr_t>(startVertex));
    ++m_drawCallCount;
}

void RenderCommandContext::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) {
    if (indexCount == 0) {
        return;
    }

    m_namedResources["__last_draw_start_index"] = reinterpret_cast<void*>(static_cast<uintptr_t>(startIndex));
    m_namedResources["__last_draw_base_vertex"] = reinterpret_cast<void*>(static_cast<intptr_t>(baseVertex));
    ++m_drawCallCount;
}

void RenderCommandContext::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                                     uint32_t startVertex, uint32_t startInstance) {
    if (vertexCount == 0 || instanceCount == 0) {
        return;
    }

    m_namedResources["__last_instance_start_vertex"] = reinterpret_cast<void*>(static_cast<uintptr_t>(startVertex));
    m_namedResources["__last_instance_start_instance"] = reinterpret_cast<void*>(static_cast<uintptr_t>(startInstance));
    ++m_drawCallCount;
}

void RenderCommandContext::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                              uint32_t startIndex, int32_t baseVertex,
                                              uint32_t startInstance) {
    if (indexCount == 0 || instanceCount == 0) {
        return;
    }

    m_namedResources["__last_indexed_instance_start_index"] = reinterpret_cast<void*>(static_cast<uintptr_t>(startIndex));
    m_namedResources["__last_indexed_instance_base_vertex"] = reinterpret_cast<void*>(static_cast<intptr_t>(baseVertex));
    m_namedResources["__last_indexed_instance_start_instance"] = reinterpret_cast<void*>(static_cast<uintptr_t>(startInstance));
    ++m_drawCallCount;
}

void RenderCommandContext::ClearRenderTarget(IRenderTarget* renderTarget, const float color[4]) {
    m_stateCache.currentRenderTarget = renderTarget;
    if (color) {
        m_lastClearColor[0] = color[0];
        m_lastClearColor[1] = color[1];
        m_lastClearColor[2] = color[2];
        m_lastClearColor[3] = color[3];
    }
}

void RenderCommandContext::ClearRenderTarget(IRenderTarget* renderTarget, float r, float g, float b, float a) {
    const float color[4] = {r, g, b, a};
    ClearRenderTarget(renderTarget, color);
}

void RenderCommandContext::ClearDepthStencil(IDepthStencil* depthStencil, float depth, uint8_t stencil) {
    m_stateCache.currentDepthStencil = depthStencil;
    m_lastDepthValue = depth;
    m_lastStencilValue = stencil;
}

void RenderCommandContext::GpuMemoryBarrier() {
}

void RenderCommandContext::UAVBarrier() {
}

void RenderCommandContext::BeginDebugMarker(const std::string& name) {
    m_debugMarkers.push_back(name);
}

void RenderCommandContext::EndDebugMarker() {
}

void RenderCommandContext::InsertDebugMarker(const std::string& name) {
    m_debugMarkers.push_back(name);
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
    m_namedResources["__legacy_pipeline_state"] = pso;
}

void RenderCommandContext::SetNativeRenderTarget(void* renderTarget) {
    m_nativeRenderTarget = renderTarget;
}

void RenderCommandContext::SetNativeDepthStencil(void* depthStencil) {
    m_nativeDepthStencil = depthStencil;
}

} // namespace Prisma::Graphic
