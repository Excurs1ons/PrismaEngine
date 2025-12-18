#pragma once

#include "DX12Adapters.h"
#include "RenderBackendDirectX12.h"
#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceFactory.h"
#include "interfaces/ISwapChain.h"
#include <d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_4.h>
#include <memory>
#include <vector>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12CommandBuffer;
class DX12Fence;
class DX12SwapChain;
class DX12ResourceFactory;

/// @brief DirectX12渲染设备适配器
/// 实现IRenderDevice接口，包装现有的RenderBackendDirectX12
class DX12RenderDevice : public IRenderDevice {
public:
    /// @brief 构造函数
    /// @param backend 现有的DirectX12后端
    explicit DX12RenderDevice(RenderBackendDirectX12* backend);

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

private:
    RenderBackendDirectX12* m_backend;  // 原始后端实现
    std::unique_ptr<DX12ResourceFactory> m_resourceFactory;
    std::unique_ptr<DX12SwapChain> m_swapChain;
    bool m_initialized = false;

    // 渲染统计
    mutable RenderStats m_stats = {};

    // 调试标记堆
    ComPtr<ID3D12Debug> m_debugController;
    ComPtr<ID3D12InfoQueue> m_infoQueue;
};

} // namespace PrismaEngine::Graphic::DX12