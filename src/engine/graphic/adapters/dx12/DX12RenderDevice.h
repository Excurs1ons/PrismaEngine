#pragma once


#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/ISwapChain.h"
#include <memory>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12CommandBuffer;
class DX12Fence;
class DX12SwapChain;
class DX12ResourceFactory;

/// @brief DirectX12渲染设备适配器
/// 实现IRenderDevice接口，独立DirectX 12实现
class DX12RenderDevice : public IRenderDevice {
public:
    /// @brief 构造函数
    DX12RenderDevice();

    /// @brief 析构函数
    ~DX12RenderDevice() override;

    // IRenderDevice接口实现
    bool Initialize(const DeviceDesc& desc) override;
    void Shutdown() override;
    std::string GetName() const override;
    std::string GetAPIName() const override;

    // 命令缓冲区管理
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override;
    void SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence = nullptr) override;
    void SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                             const std::vector<IFence*>& fences = {}) override;

    // 同步操作
    void WaitForIdle() override;
    std::unique_ptr<IFence> CreateFence() override;
    void WaitForFence(IFence* fence) override;

    // 资源管理
    IResourceFactory* GetResourceFactory() const override;

    // 交换链管理
    std::unique_ptr<ISwapChain> CreateSwapChain(void* windowHandle,
                                               uint32_t width,
                                               uint32_t height,
                                               bool vsync = true) override;
    ISwapChain* GetSwapChain() const override;

    // 帧管理
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    // 查询支持的功能
    bool SupportsMultiThreaded() const override;
    bool SupportsBindlessTextures() const override;
    bool SupportsComputeShader() const override;
    bool SupportsRayTracing() const override;
    bool SupportsMeshShader() const override;
    bool SupportsVariableRateShading() const override;

    // 渲染统计
    GPUMemoryInfo GetGPUMemoryInfo() const override;
    RenderStats GetRenderStats() const override;

    // 调试功能
    void BeginDebugMarker(const std::string& name) override;
    void EndDebugMarker() override;
    void SetDebugMarker(const std::string& name) override;

    // === DirectX12特定方法 ===

    /// @brief 获取DirectX12设备
    /// @return D3D12设备指针
    ID3D12Device* GetD3D12Device() const;

    /// @brief 获取命令队列
    /// @return 命令队列指针
    ID3D12CommandQueue* GetCommandQueue() const;

    /// @brief 获取根签名
    /// @return 根签名指针
    ID3D12RootSignature* GetRootSignature() const;

    /// @brief 获取管线状态对象
    /// @return PSO指针
    ID3D12PipelineState* GetPipelineState() const;

    /// @brief 获取RTV堆
    /// @return RTV描述符堆指针
    ID3D12DescriptorHeap* GetRTVHeap() const;

    /// @brief 获取DSV堆
    /// @return DSV描述符堆指针
    ID3D12DescriptorHeap* GetDSVHeap() const;

    /// @brief 获取RTV描述符大小
    /// @return RTV描述符大小
    UINT GetRTVDescriptorSize() const;

    /// @brief 获取帧索引
    /// @return 当前帧索引
    UINT GetCurrentFrameIndex() const;

    /// @brief 获取帧数
    /// @return 帧缓冲区数量
    UINT GetFrameCount() const;

    /// @brief 等待前一帧完成
    void WaitForPreviousFrame();

    /// @brief 获取DXGI交换链
    /// @return DXGI交换链指针
    IDXGISwapChain3* GetDXGISwapChain() const;

    /// @brief 获取渲染目标
    /// @param bufferIndex 缓冲区索引
    /// @return 渲染目标资源
    ID3D12Resource* GetRenderTarget(UINT bufferIndex) const;

    /// @brief 获取当前帧索引
    /// @return 当前帧索引（与GetCurrentFrameIndex相同）
    UINT GetCurrentFrame() const { return m_frameIndex; }

    /// @brief 获取视口
    /// @return 视口结构
    const D3D12_VIEWPORT& GetViewport() const { return m_viewport; }

    /// @brief 呈现帧（带参数版本）
    /// @param syncInterval 同步间隔
    /// @param flags 呈现标志
    /// @return HRESULT
    HRESULT PresentWithParams(UINT syncInterval, UINT flags);

    /// @brief 获取交换链描述
    /// @param desc 输出描述
    /// @return 是否成功
    bool GetSwapChainDesc(DXGI_SWAP_CHAIN_DESC* desc) const;

    /// @brief 调整交换链缓冲区
    /// @param bufferCount 缓冲区数量
    /// @param width 宽度
    /// @param height 高度
    /// @param format 格式
    /// @param flags 标志
    /// @return HRESULT
    HRESULT ResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT format, UINT flags);

    /// @brief 设置全屏状态
    /// @param fullscreen 是否全屏
    /// @param target 目标输出
    /// @return HRESULT
    HRESULT SetFullscreenState(BOOL fullscreen, IDXGIOutput* target);

    /// @brief 获取交换链缓冲区
    /// @param bufferIndex 缓冲区索引
    /// @param riid 接口ID
    /// @param ppSurface 输出表面
    /// @return HRESULT
    HRESULT GetSwapChainBuffer(UINT bufferIndex, REFIID riid, void** ppSurface);

    /// @brief 获取绘制命令签名
    /// @return 命令签名指针
    ID3D12CommandSignature* GetCommandSignature();

    /// @brief 获取索引绘制命令签名
    /// @return 索引命令签名指针
    ID3D12CommandSignature* GetIndexedCommandSignature();

    /// @brief 获取计算命令签名
    /// @return 计算命令签名指针
    ID3D12CommandSignature* GetDispatchCommandSignature();

private:
    // DirectX 12 核心组件
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    // 交换链和渲染目标
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Resource> m_renderTargets[2];
    ComPtr<ID3D12Resource> m_depthStencil;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    // 同步对象
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent = nullptr;
    uint64_t m_fenceValue = 0;
    uint32_t m_frameIndex = 0;
    static const uint32_t FrameCount = 2;

    // 动态缓冲区（每帧上传用）
    ComPtr<ID3D12Resource> m_dynamicVertexBuffer;
    ComPtr<ID3D12Resource> m_dynamicIndexBuffer;
    ComPtr<ID3D12Resource> m_dynamicConstantBuffer;
    uint8_t* m_dynamicVBCPUAddress = nullptr;
    uint8_t* m_dynamicIBCPUAddress = nullptr;
    uint8_t* m_dynamicCBCPUAddress = nullptr;
    uint64_t m_dynamicVBSize = 0;
    uint64_t m_dynamicIBSize = 0;
    uint64_t m_dynamicCBSize = 0;
    uint64_t m_dynamicVBOffset = 0;
    uint64_t m_dynamicIBOffset = 0;
    uint64_t m_dynamicCBOffset = 0;

    // 视口和裁剪矩形
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    // 描述符大小
    UINT m_rtvDescriptorSize = 0;

    // 窗口句柄
    void* m_windowHandle = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // 自定义组件
    std::unique_ptr<DX12ResourceFactory> m_resourceFactory;
    std::unique_ptr<DX12SwapChain> m_swapChainAdapter;
    bool m_initialized = false;
    uint64_t m_currentFenceValue = 1;

    // 渲染统计
    mutable RenderStats m_stats = {};

    // 调试标记堆
    ComPtr<ID3D12Debug> m_debugController;
    ComPtr<ID3D12InfoQueue> m_infoQueue;

    // 命令签名
    ComPtr<ID3D12CommandSignature> m_commandSignature;
    ComPtr<ID3D12CommandSignature> m_indexedCommandSignature;
    ComPtr<ID3D12CommandSignature> m_dispatchCommandSignature;
};

} // namespace PrismaEngine::Graphic::DX12