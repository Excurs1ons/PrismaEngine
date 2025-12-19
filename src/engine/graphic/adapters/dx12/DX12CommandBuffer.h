#pragma once

#include "interfaces/ICommandBuffer.h"
#include "interfaces/IPipelineState.h"
#include <directx/d3dx12.h>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;
class DX12PipelineState;

/// @brief DirectX12命令缓冲区适配器
/// 实现ICommandBuffer接口，包装ID3D12GraphicsCommandList
class DX12CommandBuffer : public ICommandBuffer {
public:
    /// @brief 构造函数
    /// @param device DirectX12渲染设备
    explicit DX12CommandBuffer(DX12RenderDevice* device);

    /// @brief 析构函数
    ~DX12CommandBuffer() override;

    /// @brief 初始化命令缓冲区
    /// @return 是否初始化成功
    bool Initialize();

    // ICommandBuffer接口实现
    void Begin() override;
    void End() override;
    bool Reset() override;

    void BeginRenderPass(const RenderPassDesc& desc) override;
    void EndRenderPass() override;

    void SetPipelineState(IPipelineState* pipelineState) override;

    void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t stride) override;
    void SetIndexBuffer(IBuffer* buffer, bool is32Bit, uint32_t offset) override;
    void SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size) override;
    void SetTexture(ITexture* texture, uint32_t slot) override;
    void SetSampler(ISampler* sampler, uint32_t slot) override;
    void SetShaderResource(IBuffer* buffer, uint32_t slot) override;
    void SetUnorderedAccess(IBuffer* buffer, uint32_t slot) override;

    void SetViewport(const Viewport& viewport) override;
    void SetViewports(const Viewport* viewports, uint32_t count) override;
    void SetScissorRect(const Rect& rect) override;
    void SetScissorRects(const Rect* rects, uint32_t count) override;

    void Draw(uint32_t vertexCount, uint32_t startVertex) override;
    void DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) override;
    void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                      uint32_t startVertex, uint32_t startInstance) override;
    void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                             uint32_t startIndex, int32_t baseVertex,
                             uint32_t startInstance) override;
    void DrawIndirect(IBuffer* indirectBuffer, uint32_t offset) override;
    void DrawIndexedIndirect(IBuffer* indirectBuffer, uint32_t offset) override;

    void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;
    void DispatchIndirect(IBuffer* indirectBuffer, uint32_t offset) override;

    void CopyBuffer(IBuffer* dst, IBuffer* src) override;
    void CopyBufferRegion(IBuffer* dst, uint64_t dstOffset,
                         IBuffer* src, uint64_t srcOffset,
                         uint64_t size) override;
    void CopyTexture(ITexture* dst, ITexture* src) override;
    void UpdateBuffer(IBuffer* buffer, const void* data, uint64_t size, uint64_t offset) override;
    void UpdateTexture(ITexture* texture, const void* data, uint64_t dataSize,
                      uint32_t mipLevel, uint32_t arraySlice) override;

    void MemoryBarrier() override;
    void UAVBarrier() override;

    void BeginTimestampQuery(void* queryPool, uint32_t queryIndex) override;
    void EndTimestampQuery(void* queryPool, uint32_t queryIndex) override;
    void ResolveQueryData(IBuffer* dstBuffer, void* queryPool,
                         uint32_t startQuery, uint32_t queryCount) override;

    void InsertDebugMarker(const std::string& name) override;
    void BeginDebugGroup(const std::string& name) override;
    void EndDebugGroup() override;

    // === DirectX12特定方法 ===

    /// @brief 获取命令列表
    /// @return 命令列表指针
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

    /// @brief 获取命令分配器
    /// @return 命令分配器指针
    ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocator.Get(); }

    /// @brief 关闭命令缓冲区
    void Close() override;

    /// @brief 获取绘制调用次数
    /// @return 绘制调用次数
    uint32_t GetDrawCallCount() const { return m_drawCallCount; }

    /// @brief 获取三角形数量
    /// @return 三角形数量
    uint32_t GetTriangleCount() const { return m_triangleCount; }

    /// @brief 重置统计信息
    void ResetStats() {
        m_drawCallCount = 0;
        m_triangleCount = 0;
    }

private:
    DX12RenderDevice* m_device;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

    // 状态跟踪
    bool m_isOpen = false;
    bool m_isRecording = false;
    IPipelineState* m_currentPipelineState = nullptr;

    // 统计信息
    uint32_t m_drawCallCount = 0;
    uint32_t m_triangleCount = 0;

    // 当前渲染目标
    struct RenderTargetState {
        ITexture* renderTarget = nullptr;
        ITexture* depthStencil = nullptr;
        bool isValid = false;
    } m_currentRenderTarget;

    // 辅助方法
    void ValidateAndSetPipelineState();
    void ConvertToD3D12Viewport(const Viewport& viewport, D3D12_VIEWPORT& outViewport);
    void ConvertToD3D12Rect(const Rect& rect, D3D12_RECT& outRect);
    D3D12_RESOURCE_BARRIER CreateResourceBarrier(ID3D12Resource* resource,
                                               D3D12_RESOURCE_STATES before,
                                               D3D12_RESOURCE_STATES after);
    void UpdateDrawStats(uint32_t primitiveType, uint32_t primitiveCount);
};

} // namespace PrismaEngine::Graphic::DX12