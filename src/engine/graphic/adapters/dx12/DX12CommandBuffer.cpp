#include "DX12CommandBuffer.h"
#include "DX12RenderDevice.h"
#include "DX12Texture.h"
#include "DX12Buffer.h"
#include "DX12PipelineState.h"
#include "DX12Sampler.h"

#include <directx/d3dx12.h>
#include <sstream>
#include <algorithm>

namespace PrismaEngine::Graphic::DX12 {

DX12CommandBuffer::DX12CommandBuffer(DX12RenderDevice* device)
    : m_device(device) {
}

DX12CommandBuffer::~DX12CommandBuffer() {
    if (m_isOpen) {
        Close();
    }
}

bool DX12CommandBuffer::Initialize() {
    if (!m_device || !m_device->GetD3D12Device()) {
        return false;
    }

    ID3D12Device* device = m_device->GetD3D12Device();

    // 创建命令分配器
    HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               IID_PPV_ARGS(&m_commandAllocator));
    if (FAILED(hr)) {
        return false;
    }

    // 创建命令列表
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                   m_commandAllocator.Get(), nullptr,
                                   IID_PPV_ARGS(&m_commandList));
    if (FAILED(hr)) {
        return false;
    }

    // 命令列表初始状态为关闭
    m_commandList->Close();
    m_isOpen = false;
    m_isRecording = false;

    ResetStats();
    return true;
}

void DX12CommandBuffer::Begin() {
    if (m_isOpen || m_isRecording) {
        return;
    }

    // 重置命令分配器和命令列表
    Reset();
    m_isOpen = true;
    m_isRecording = true;

    // 记录调试信息
    InsertDebugMarker("CommandBuffer Begin");
}

void DX12CommandBuffer::End() {
    if (!m_isOpen || !m_isRecording) {
        return;
    }

    m_isRecording = false;
    m_isOpen = false;
}

bool DX12CommandBuffer::Close() {
    if (!m_isOpen) {
        return true;
    }

    if (m_isRecording) {
        End();
    }

    HRESULT hr = m_commandList->Close();
    return SUCCEEDED(hr);
}

bool DX12CommandBuffer::Reset() {
    // 重置命令分配器
    HRESULT hr = m_commandAllocator->Reset();
    if (FAILED(hr)) {
        return false;
    }

    // 重置命令列表
    hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
    if (FAILED(hr)) {
        return false;
    }

    // 重置状态
    m_currentPipeline = nullptr;
    m_currentRenderTarget = {};
    ResetStats();

    return true;
}

void DX12CommandBuffer::BeginRenderPass(const RenderPassDesc& desc) {
    if (!m_isRecording) return;

    // 转换渲染目标
    DX12Texture* dx12RenderTarget = static_cast<DX12Texture*>(desc.renderTarget);
    DX12Texture* dx12DepthStencil = static_cast<DX12Texture*>(desc.depthStencil);

    if (dx12RenderTarget) {
        // 设置渲染目标屏障
        ID3D12Resource* rtResource = dx12RenderTarget->GetResource();
        auto barrier = CreateResourceBarrier(rtResource,
                                           D3D12_RESOURCE_STATE_PRESENT,
                                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &barrier);

        // 设置渲染目标
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = dx12RenderTarget->GetRTV();
        m_commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        // 清除渲染目标
        if (desc.clearRenderTarget) {
            m_commandList->ClearRenderTargetView(rtv,
                                                reinterpret_cast<const float*>(&desc.clearColor),
                                                0, nullptr);
        }

        // 设置视口和裁剪矩形
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(dx12RenderTarget->GetWidth());
        viewport.Height = static_cast<float>(dx12RenderTarget->GetHeight());
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissor = {};
        scissor.right = dx12RenderTarget->GetWidth();
        scissor.bottom = dx12RenderTarget->GetHeight();
        m_commandList->RSSetScissorRects(1, &scissor);
    }

    // 设置深度模板缓冲区
    if (dx12DepthStencil) {
        ID3D12Resource* dsResource = dx12DepthStencil->GetResource();
        auto barrier = CreateResourceBarrier(dsResource,
                                           D3D12_RESOURCE_STATE_COMMON,
                                           D3D12_RESOURCE_STATE_DEPTH_WRITE);
        m_commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dx12DepthStencil->GetDSV();
        m_commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

        // 清除深度模板缓冲区
        if (desc.clearDepth || desc.clearStencil) {
            UINT clearFlags = 0;
            if (desc.clearDepth) clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
            if (desc.clearStencil) clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

            m_commandList->ClearDepthStencilView(dsv, clearFlags,
                                                desc.clearDepth, desc.clearStencil,
                                                0, nullptr);
        }
    }

    // 记录当前渲染目标
    m_currentRenderTarget.renderTarget = desc.renderTarget;
    m_currentRenderTarget.depthStencil = desc.depthStencil;
    m_currentRenderTarget.isValid = true;

    BeginDebugGroup("Render Pass");
}

void DX12CommandBuffer::EndRenderPass() {
    if (!m_isRecording) return;

    EndDebugGroup();

    // 转换回显示状态
    if (m_currentRenderTarget.renderTarget) {
        DX12Texture* dx12RenderTarget = static_cast<DX12Texture*>(m_currentRenderTarget.renderTarget);
        ID3D12Resource* rtResource = dx12RenderTarget->GetResource();
        auto barrier = CreateResourceBarrier(rtResource,
                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                           D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &barrier);
    }

    // 重置渲染目标状态
    m_currentRenderTarget = {};
}

void DX12CommandBuffer::SetPipeline(IPipeline* pipeline) {
    if (!m_isRecording) return;

    m_currentPipeline = pipeline;
}

void DX12CommandBuffer::ValidateAndSetPipeline() {
    if (m_currentPipeline) {
        DX12Pipeline* dx12Pipeline = static_cast<DX12Pipeline*>(m_currentPipeline);
        ID3D12PipelineState* pso = dx12Pipeline->GetPipelineState();
        ID3D12RootSignature* rootSig = dx12Pipeline->GetRootSignature();

        m_commandList->SetPipelineState(pso);
        m_commandList->SetGraphicsRootSignature(rootSig);
    }
}

void DX12CommandBuffer::SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t stride) {
    if (!m_isRecording || !buffer) return;

    ValidateAndSetPipeline();

    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(buffer);
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = dx12Buffer->GetGPUAddress() + offset;
    vbv.SizeInBytes = static_cast<UINT>(dx12Buffer->GetSize() - offset);
    vbv.StrideInBytes = stride > 0 ? stride : static_cast<UINT>(dx12Buffer->GetStride());

    m_commandList->IASetVertexBuffers(slot, 1, &vbv);
}

void DX12CommandBuffer::SetIndexBuffer(IBuffer* buffer, bool is32Bit, uint32_t offset) {
    if (!m_isRecording || !buffer) return;

    ValidateAndSetPipeline();

    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(buffer);
    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation = dx12Buffer->GetGPUAddress() + offset;
    ibv.SizeInBytes = static_cast<UINT>(dx12Buffer->GetSize() - offset);
    ibv.Format = is32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

    m_commandList->IASetIndexBuffer(&ibv);
}

void DX12CommandBuffer::SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size) {
    if (!m_isRecording || !buffer) return;

    ValidateAndSetPipeline();

    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(buffer);
    D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = dx12Buffer->GetGPUAddress() + offset;

    // 假设使用根签名中的常量缓冲区槽
    m_commandList->SetGraphicsRootConstantBufferView(slot, cbvAddress);
}

void DX12CommandBuffer::SetTexture(ITexture* texture, uint32_t slot) {
    if (!m_isRecording || !texture) return;

    ValidateAndSetPipeline();

    DX12Texture* dx12Texture = static_cast<DX12Texture*>(texture);
    D3D12_GPU_DESCRIPTOR_HANDLE srv = dx12Texture->GetSRV();

    // 假设使用描述符表
    m_commandList->SetGraphicsRootDescriptorTable(slot, srv);
}

void DX12CommandBuffer::SetSampler(ISampler* sampler, uint32_t slot) {
    if (!m_isRecording || !sampler) return;

    ValidateAndSetPipeline();

    DX12Sampler* dx12Sampler = static_cast<DX12Sampler*>(sampler);
    D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = dx12Sampler->GetHandle();

    // 假设使用描述符表
    m_commandList->SetGraphicsRootDescriptorTable(slot, samplerHandle);
}

void DX12CommandBuffer::SetShaderResource(IBuffer* buffer, uint32_t slot) {
    if (!m_isRecording || !buffer) return;

    ValidateAndSetPipeline();

    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(buffer);
    D3D12_GPU_VIRTUAL_ADDRESS srvAddress = dx12Buffer->GetGPUAddress();

    m_commandList->SetGraphicsRootShaderResourceView(slot, srvAddress);
}

void DX12CommandBuffer::SetUnorderedAccess(IBuffer* buffer, uint32_t slot) {
    if (!m_isRecording || !buffer) return;

    ValidateAndSetPipeline();

    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(buffer);
    D3D12_GPU_VIRTUAL_ADDRESS uavAddress = dx12Buffer->GetGPUAddress();

    m_commandList->SetGraphicsRootUnorderedAccessView(slot, uavAddress);
}

void DX12CommandBuffer::SetViewport(const Viewport& viewport) {
    if (!m_isRecording) return;

    D3D12_VIEWPORT d3dViewport;
    ConvertToD3D12Viewport(viewport, d3dViewport);
    m_commandList->RSSetViewports(1, &d3dViewport);
}

void DX12CommandBuffer::SetViewports(const Viewport* viewports, uint32_t count) {
    if (!m_isRecording || !viewports || count == 0) return;

    std::vector<D3D12_VIEWPORT> d3dViewports(count);
    for (uint32_t i = 0; i < count; ++i) {
        ConvertToD3D12Viewport(viewports[i], d3dViewports[i]);
    }
    m_commandList->RSSetViewports(count, d3dViewports.data());
}

void DX12CommandBuffer::SetScissorRect(const Rect& rect) {
    if (!m_isRecording) return;

    D3D12_RECT d3dRect;
    ConvertToD3D12Rect(rect, d3dRect);
    m_commandList->RSSetScissorRects(1, &d3dRect);
}

void DX12CommandBuffer::SetScissorRects(const Rect* rects, uint32_t count) {
    if (!m_isRecording || !rects || count == 0) return;

    std::vector<D3D12_RECT> d3dRects(count);
    for (uint32_t i = 0; i < count; ++i) {
        ConvertToD3D12Rect(rects[i], d3dRects[i]);
    }
    m_commandList->RSSetScissorRects(count, d3dRects.data());
}

void DX12CommandBuffer::Draw(uint32_t vertexCount, uint32_t startVertex) {
    if (!m_isRecording) return;

    ValidateAndSetPipeline();
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->DrawInstanced(vertexCount, 1, startVertex, 0);

    UpdateDrawStats(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertexCount / 3);
}

void DX12CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) {
    if (!m_isRecording) return;

    ValidateAndSetPipeline();
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->DrawIndexedInstanced(indexCount, 1, startIndex, baseVertex, 0);

    UpdateDrawStats(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, indexCount / 3);
}

void DX12CommandBuffer::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                                     uint32_t startVertex, uint32_t startInstance) {
    if (!m_isRecording) return;

    ValidateAndSetPipeline();
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);

    UpdateDrawStats(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, (vertexCount / 3) * instanceCount);
}

void DX12CommandBuffer::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                            uint32_t startIndex, int32_t baseVertex,
                                            uint32_t startInstance) {
    if (!m_isRecording) return;

    ValidateAndSetPipeline();
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);

    UpdateDrawStats(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, (indexCount / 3) * instanceCount);
}

void DX12CommandBuffer::DrawIndirect(IBuffer* indirectBuffer, uint32_t offset) {
    // 实现间接绘制
    if (!m_isRecording || !indirectBuffer) return;

    ValidateAndSetPipeline();
    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(indirectBuffer);

    m_commandList->ExecuteIndirect(
        m_device->GetCommandSignature(),  // 需要创建命令签名
        1,
        dx12Buffer->GetResource(),
        offset,
        nullptr,
        0);
}

void DX12CommandBuffer::DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset) {
    // 实现间接索引绘制
    if (!m_isRecording || !indirectBuffer) return;

    ValidateAndSetPipeline();
    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(indirectBuffer);

    m_commandList->ExecuteIndirect(
        m_device->GetIndexedCommandSignature(),  // 需要创建索引命令签名
        1,
        dx12Buffer->GetResource(),
        offset,
        nullptr,
        0);
}

void DX12CommandBuffer::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
    if (!m_isRecording) return;

    ValidateAndSetPipeline();
    m_commandList->Dispatch(x, y, z);
}

void DX12CommandBuffer::DispatchIndirect(IBuffer* indirectBuffer, uint32_t offset) {
    if (!m_isRecording || !indirectBuffer) return;

    ValidateAndSetPipeline();
    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(indirectBuffer);

    m_commandList->ExecuteIndirect(
        m_device->GetDispatchCommandSignature(),  // 需要创建计算命令签名
        1,
        dx12Buffer->GetResource(),
        offset,
        nullptr,
        0);
}

void DX12CommandBuffer::CopyBuffer(IBuffer* dst, IBuffer* src) {
    if (!m_isRecording || !dst || !src) return;

    DX12Buffer* dxDst = static_cast<DX12Buffer*>(dst);
    DX12Buffer* dxSrc = static_cast<DX12Buffer*>(src);

    m_commandList->CopyResource(dxDst->GetResource(), dxSrc->GetResource());
}

void DX12CommandBuffer::CopyBufferRegion(IBuffer* dst, uint64_t dstOffset,
                                        IBuffer* src, uint64_t srcOffset,
                                        uint64_t size) {
    if (!m_isRecording || !dst || !src) return;

    DX12Buffer* dxDst = static_cast<DX12Buffer*>(dst);
    DX12Buffer* dxSrc = static_cast<DX12Buffer*>(src);

    m_commandList->CopyBufferRegion(dxDst->GetResource(), dstOffset,
                                   dxSrc->GetResource(), srcOffset,
                                   size);
}

void DX12CommandBuffer::CopyTexture(ITexture* dst, ITexture* src) {
    if (!m_isRecording || !dst || !src) return;

    DX12Texture* dxDst = static_cast<DX12Texture*>(dst);
    DX12Texture* dxSrc = static_cast<DX12Texture*>(src);

    m_commandList->CopyResource(dxDst->GetResource(), dxSrc->GetResource());
}

void DX12CommandBuffer::UpdateBuffer(IBuffer* buffer, const void* data, uint64_t size, uint64_t offset) {
    // 使用动态上传统计
    if (!m_isRecording || !buffer || !data) return;

    DX12Buffer* dx12Buffer = static_cast<DX12Buffer*>(buffer);

    // 这里应该使用设备的动态上传缓冲区
    // 简化实现，实际需要更复杂的资源状态管理
    void* mappedData = nullptr;
    D3D12_RANGE range = { offset, offset + size };
    if (SUCCEEDED(dx12Buffer->GetResource()->Map(0, &range, &mappedData))) {
        memcpy(static_cast<uint8_t*>(mappedData) + offset, data, size);
        dx12Buffer->GetResource()->Unmap(0, nullptr);
    }
}

void DX12CommandBuffer::UpdateTexture(ITexture* texture, const void* data, uint64_t dataSize,
                                    uint32_t mipLevel, uint32_t arraySlice) {
    // 实现纹理更新
    if (!m_isRecording || !texture || !data) return;

    DX12Texture* dx12Texture = static_cast<DX12Texture*>(texture);

    // 需要实现更复杂的纹理更新逻辑
    // 包括放置纹理、状态转换等
}

void DX12CommandBuffer::MemoryBarrier() {
    if (!m_isRecording) return;

    // 创建通用内存屏障
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = nullptr;  // 所有UAV资源

    m_commandList->ResourceBarrier(1, &barrier);
}

void DX12CommandBuffer::UAVBarrier() {
    if (!m_isRecording) return;

    // Direct3D 12中UAV屏障就是UAV类型的资源屏障
    MemoryBarrier();
}

void DX12CommandBuffer::BeginTimestampQuery(void* queryPool, uint32_t queryIndex) {
    // 实现时间戳查询
    if (!m_isRecording) return;
}

void DX12CommandBuffer::EndTimestampQuery(void* queryPool, uint32_t queryIndex) {
    // 实现时间戳查询
    if (!m_isRecording) return;
}

void DX12CommandBuffer::ResolveQueryData(IBuffer* dstBuffer, void* queryPool,
                                        uint32_t startQuery, uint32_t queryCount) {
    // 实现查询数据解析
    if (!m_isRecording) return;
}

void DX12CommandBuffer::InsertDebugMarker(const std::string& name) {
    if (!m_isRecording) return;

    std::wstring wideName(name.begin(), name.end());
    m_commandList->SetMarker(0, wideName.c_str(), static_cast<UINT>(wideName.length() * sizeof(wchar_t)));
}

void DX12CommandBuffer::BeginDebugGroup(const std::string& name) {
    if (!m_isRecording) return;

    std::wstring wideName(name.begin(), name.end());
    m_commandList->BeginEvent(0, wideName.c_str(), static_cast<UINT>(wideName.length() * sizeof(wchar_t)));
}

void DX12CommandBuffer::EndDebugGroup() {
    if (!m_isRecording) return;
    m_commandList->EndEvent();
}

// === 辅助方法 ===

void DX12CommandBuffer::ConvertToD3D12Viewport(const Viewport& viewport, D3D12_VIEWPORT& outViewport) {
    outViewport.TopLeftX = viewport.x;
    outViewport.TopLeftY = viewport.y;
    outViewport.Width = viewport.width;
    outViewport.Height = viewport.height;
    outViewport.MinDepth = viewport.minDepth;
    outViewport.MaxDepth = viewport.maxDepth;
}

void DX12CommandBuffer::ConvertToD3D12Rect(const Rect& rect, D3D12_RECT& outRect) {
    outRect.left = rect.x;
    outRect.top = rect.y;
    outRect.right = rect.x + rect.width;
    outRect.bottom = rect.y + rect.height;
}

D3D12_RESOURCE_BARRIER DX12CommandBuffer::CreateResourceBarrier(ID3D12Resource* resource,
                                                              D3D12_RESOURCE_STATES before,
                                                              D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    return barrier;
}

void DX12CommandBuffer::UpdateDrawStats(uint32_t primitiveType, uint32_t primitiveCount) {
    m_drawCallCount++;

    // 根据图元类型计算三角形数量
    switch (primitiveType) {
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
            m_triangleCount += primitiveCount;
            break;
        case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
        case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
            // 线条不产生三角形
            break;
        case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
            // 点不产生三角形
            break;
    }
}

} // namespace PrismaEngine::Graphic::DX12