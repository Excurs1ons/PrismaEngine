#pragma once
#include <directx/d3dx12.h>
#include "Platform.h"
#include "RenderBackend.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d12.h>
#include <dxgi1_4.h>
using WindowHandle = void*;
using namespace DirectX;
using namespace Microsoft::WRL;
namespace Engine {
class RenderBackendDirectX12 : public RenderBackend {

public:
    RenderBackendDirectX12(std::wstring name);
    bool Initialize(Platform* platform, WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height) override;

    ~RenderBackendDirectX12() override;

    void OnRender();

    void Shutdown() override;

    void BeginFrame() override;
    void EndFrame() override;

    void SubmitRenderCommand(const RenderCommand& cmd) override;

    bool Supports(RendererFeature feature) const override;

    void Present() override;

protected:
    const RendererFeature m_support =
        static_cast<RendererFeature>(RendererFeature::MultiThreaded | RendererFeature::BindlessTextures);

public:
    // 上传并绑定顶点缓冲区数据到指定命令列表
    void UploadAndBindVertexBuffer(ID3D12GraphicsCommandList* cmdList,
                                   const void* data,
                                   uint32_t sizeInBytes,
                                   uint32_t strideInBytes);

    // 上传并绑定索引缓冲区数据到指定命令列表
    void UploadAndBindIndexBuffer(ID3D12GraphicsCommandList* cmdList,
                                  const void* data,
                                  uint32_t sizeInBytes,
                                  bool use16BitIndices = true);

private:
    bool LoadPipeline();
    bool LoadAssets();
    void WaitForPreviousFrame();
    static const UINT FrameCount = 2;

    // Pipeline objects.
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // 动态顶点上传缓冲区（每帧小批量网格数据）
    ComPtr<ID3D12Resource> m_dynamicVertexBuffer;
    uint8_t* m_dynamicVBCPUAddress = nullptr;
    uint64_t m_dynamicVBSize       = 0;
    uint64_t m_dynamicVBOffset     = 0;

    // 动态索引上传缓冲区（每帧小批量索引数据）
    ComPtr<ID3D12Resource> m_dynamicIndexBuffer;
    uint8_t* m_dynamicIBCPUAddress = nullptr;
    uint64_t m_dynamicIBSize       = 0;
    uint64_t m_dynamicIBOffset     = 0;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    // Base members.
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;
    bool m_useWarpDevice = false;
};

}  // namespace Engine
