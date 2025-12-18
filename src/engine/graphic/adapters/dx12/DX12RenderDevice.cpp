#include "DX12RenderDevice.h"
#include "DX12CommandBuffer.h"
#include "DX12Fence.h"
#include "DX12SwapChain.h"
#include "DX12ResourceFactory.h"
#include "DX12Texture.h"
#include "DX12Buffer.h"
#include "DX12Shader.h"
#include "DX12PipelineState.h"
#include "DX12Sampler.h"

#include <dxgi1_6.h>
#include <directx/d3dx12.h>
#include <sstream>
#include <iomanip>

namespace PrismaEngine::Graphic::DX12 {

DX12RenderDevice::DX12RenderDevice(RenderBackendDirectX12* backend)
    : m_backend(backend) {
    // 创建资源工厂
    m_resourceFactory = std::make_unique<DX12ResourceFactory>(this);

    // 创建交换链适配器
    m_swapChain = std::make_unique<DX12SwapChain>(backend);

    // 初始化统计信息
    m_stats.frameCount = 0;
    m_stats.drawCalls = 0;
    m_stats.triangles = 0;
    m_stats.frameTime = 0.0f;
}

DX12RenderDevice::~DX12RenderDevice() {
    Shutdown();
}

bool DX12RenderDevice::Initialize(const DeviceDesc& desc) {
    if (m_initialized) {
        return true;
    }

    // 启用调试层（如果请求）
    if (desc.enableDebug) {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
            m_debugController = debugController;

            // 配置断点
            ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)))) {
                debugController1->SetEnableGPUBasedValidation(true);
            }
        }
    }

    // 后端已经在Engine中初始化，这里只需要验证
    if (!m_backend) {
        return false;
    }

    // 设置调试信息队列
    if (desc.enableDebug && m_debugController) {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(GetD3D12Device()->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
            m_infoQueue = infoQueue;

            // 设置严重性级别
            D3D12_MESSAGE_SEVERITY severities[] = {
                D3D12_MESSAGE_SEVERITY_INFO,
                D3D12_MESSAGE_SEVERITY_WARNING,
                D3D12_MESSAGE_SEVERITY_ERROR,
                D3D12_MESSAGE_SEVERITY_CORRUPTION
            };

            // 过滤某些消息
            D3D12_MESSAGE_ID denyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGRECT,
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumSeverities = ARRAYSIZE(severities);
            filter.DenyList.pSeverityList = severities;
            filter.DenyList.NumIDs = ARRAYSIZE(denyIds);
            filter.DenyList.pIDList = denyIds;

            infoQueue->PushStorageFilter(&filter);
        }
    }

    m_initialized = true;
    return true;
}

void DX12RenderDevice::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // 等待所有GPU操作完成
    WaitForIdle();

    // 清理资源
    m_resourceFactory.reset();
    m_swapChain.reset();

    // 清理调试对象
    m_infoQueue.Reset();
    m_debugController.Reset();

    m_initialized = false;
}

std::string DX12RenderDevice::GetName() const {
    if (m_backend) {
        return "DirectX12 RenderDevice";
    }
    return "Invalid Device";
}

std::string DX12RenderDevice::GetAPIName() const {
    return "DirectX12";
}

std::unique_ptr<ICommandBuffer> DX12RenderDevice::CreateCommandBuffer(CommandBufferType type) {
    if (!m_initialized) {
        return nullptr;
    }

    // DirectX12中所有命令缓冲区都是图形命令列表
    auto cmdBuffer = std::make_unique<DX12CommandBuffer>(this);
    if (cmdBuffer && cmdBuffer->Initialize()) {
        return cmdBuffer;
    }
    return nullptr;
}

void DX12RenderDevice::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {
    if (!cmdBuffer || !m_initialized) {
        return;
    }

    // 转换为DX12命令缓冲区
    DX12CommandBuffer* dx12CmdBuffer = static_cast<DX12CommandBuffer*>(cmdBuffer);
    dx12CmdBuffer->Close();

    // 执行命令列表
    ID3D12GraphicsCommandList* cmdList = dx12CmdBuffer->GetCommandList();
    ID3D12CommandList* cmdLists[] = { cmdList };
    GetCommandQueue()->ExecuteCommandLists(1, cmdLists);

    // 如果提供了围栏，设置信号
    if (fence) {
        DX12Fence* dx12Fence = static_cast<DX12Fence*>(fence);
        dx12Fence->Signal();
    }

    // 更新统计信息
    m_stats.drawCalls += dx12CmdBuffer->GetDrawCallCount();
    m_stats.triangles += dx12CmdBuffer->GetTriangleCount();
}

void DX12RenderDevice::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                                           const std::vector<IFence*>& fences) {
    if (cmdBuffers.empty() || !m_initialized) {
        return;
    }

    // 准备命令列表数组
    std::vector<ID3D12CommandList*> commandLists;
    commandLists.reserve(cmdBuffers.size());

    uint32_t totalDrawCalls = 0;
    uint32_t totalTriangles = 0;

    // 收集所有命令列表
    for (ICommandBuffer* cmdBuffer : cmdBuffers) {
        if (!cmdBuffer) continue;

        DX12CommandBuffer* dx12CmdBuffer = static_cast<DX12CommandBuffer*>(cmdBuffer);
        dx12CmdBuffer->Close();
        commandLists.push_back(dx12CmdBuffer->GetCommandList());

        totalDrawCalls += dx12CmdBuffer->GetDrawCallCount();
        totalTriangles += dx12CmdBuffer->GetTriangleCount();
    }

    // 批量执行
    GetCommandQueue()->ExecuteCommandLists(static_cast<UINT>(commandLists.size()),
                                          commandLists.data());

    // 设置围栏信号
    if (fences.size() > 0 && fences[0]) {
        DX12Fence* dx12Fence = static_cast<DX12Fence*>(fences[0]);
        dx12Fence->Signal();
    }

    // 更新统计信息
    m_stats.drawCalls += totalDrawCalls;
    m_stats.triangles += totalTriangles;
}

void DX12RenderDevice::WaitForIdle() {
    if (m_backend) {
        m_backend->WaitForPreviousFrame();
    }
}

std::unique_ptr<IFence> DX12RenderDevice::CreateFence() {
    if (!m_initialized || !GetD3D12Device()) {
        return nullptr;
    }

    ComPtr<ID3D12Fence> fence;
    if (FAILED(GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        return nullptr;
    }

    return std::make_unique<DX12Fence>(fence);
}

void DX12RenderDevice::WaitForFence(IFence* fence) {
    if (!fence) return;

    DX12Fence* dx12Fence = static_cast<DX12Fence*>(fence);
    dx12Fence->Wait();
}

IResourceFactory* DX12RenderDevice::GetResourceFactory() const {
    return m_resourceFactory.get();
}

std::unique_ptr<ISwapChain> DX12RenderDevice::CreateSwapChain(void* windowHandle,
                                                           uint32_t width,
                                                           uint32_t height,
                                                           bool vsync) {
    // 目前使用后端的交换链，返回现有实例的副本
    if (m_swapChain) {
        return m_swapChain->Clone();
    }
    return nullptr;
}

ISwapChain* DX12RenderDevice::GetSwapChain() const {
    return m_swapChain.get();
}

bool DX12RenderDevice::SupportsMultiThreaded() const {
    return m_backend ? m_backend->Supports(RendererFeature::MultiThreaded) : false;
}

bool DX12RenderDevice::SupportsBindlessTextures() const {
    return m_backend ? m_backend->Supports(RendererFeature::BindlessTextures) : false;
}

bool DX12RenderDevice::SupportsComputeShader() const {
    // DirectX12始终支持计算着色器
    return GetD3D12Device() != nullptr;
}

bool DX12RenderDevice::SupportsRayTracing() const {
    // 检查DXR支持
    if (!GetD3D12Device()) return false;

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if (SUCCEEDED(GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
        return options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    }
    return false;
}

bool DX12RenderDevice::SupportsMeshShader() const {
    if (!GetD3D12Device()) return false;

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    if (SUCCEEDED(GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
        return options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    }
    return false;
}

bool DX12RenderDevice::SupportsVariableRateShading() const {
    if (!GetD3D12Device()) return false;

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    if (SUCCEEDED(GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
        return options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
    }
    return false;
}

DX12RenderDevice::GPUMemoryInfo DX12RenderDevice::GetGPUMemoryInfo() const {
    GPUMemoryInfo info = {};

    // 查询DXGI适配器
    ComPtr<IDXGIFactory6> factory;
    if (SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        ComPtr<IDXGIAdapter1> adapter;
        if (SUCCEEDED(factory->EnumAdapters1(0, &adapter))) {
            DXGI_ADAPTER_DESC1 desc;
            if (SUCCEEDED(adapter->GetDesc1(&desc))) {
                info.totalMemory = desc.DedicatedVideoMemory;

                // 使用Windows API查询可用内存
                // 这里简化处理，实际应用中可以使用更精确的方法
                info.usedMemory = info.totalMemory / 2; // 简化估算
                info.availableMemory = info.totalMemory - info.usedMemory;
            }
        }
    }

    return info;
}

DX12RenderDevice::RenderStats DX12RenderDevice::GetRenderStats() const {
    RenderStats stats = m_stats;

    // 获取GPU时间（简化实现）
    if (m_backend) {
        // 这里需要从后端获取实际的帧时间
        stats.frameTime = 16.67f; // 60 FPS示例值
    }

    return stats;
}

void DX12RenderDevice::BeginDebugMarker(const std::string& name) {
    if (!GetCommandQueue() || !m_debugController) return;

    // 将UTF-8字符串转换为宽字符
    std::wstring wideName(name.begin(), name.end());
    GetCommandQueue()->BeginEvent(0, wideName.c_str(), static_cast<UINT>(wideName.length() * sizeof(wchar_t)));
}

void DX12RenderDevice::EndDebugMarker() {
    if (!GetCommandQueue() || !m_debugController) return;
    GetCommandQueue()->EndEvent();
}

void DX12RenderDevice::SetDebugMarker(const std::string& name) {
    if (!GetCommandQueue() || !m_debugController) return;

    std::wstring wideName(name.begin(), name.end());
    GetCommandQueue()->SetMarker(0, wideName.c_str(), static_cast<UINT>(wideName.length() * sizeof(wchar_t)));
}

// === DirectX12特定方法 ===

ID3D12Device* DX12RenderDevice::GetD3D12Device() const {
    // 需要从后端获取设备指针
    // 这里假设后端暴露了设备访问方法
    // 实际实现可能需要在RenderBackendDirectX12中添加GetDevice()方法
    return nullptr; // 暂时返回null，需要在后端添加访问方法
}

ID3D12CommandQueue* DX12RenderDevice::GetCommandQueue() const {
    // 同样需要从后端获取
    return nullptr; // 暂时返回null
}

ID3D12RootSignature* DX12RenderDevice::GetRootSignature() const {
    // 需要从后端获取
    return nullptr;
}

ID3D12PipelineState* DX12RenderDevice::GetPipelineState() const {
    // 需要从后端获取
    return nullptr;
}

ID3D12DescriptorHeap* DX12RenderDevice::GetRTVHeap() const {
    // 需要从后端获取
    return nullptr;
}

ID3D12DescriptorHeap* DX12RenderDevice::GetDSVHeap() const {
    // 需要从后端获取
    return nullptr;
}

UINT DX12RenderDevice::GetRTVDescriptorSize() const {
    // 需要从后端获取
    return 0;
}

UINT DX12RenderDevice::GetCurrentFrameIndex() const {
    // 需要从后端获取
    return 0;
}

UINT DX12RenderDevice::GetFrameCount() const {
    // DirectX12适配器使用双缓冲
    return 2;
}

void DX12RenderDevice::WaitForPreviousFrame() {
    if (m_backend) {
        m_backend->WaitForPreviousFrame();
    }
}

void DX12RenderDevice::BeginFrame() {
    if (m_backend) {
        m_backend->BeginFrame();
    }
}

void DX12RenderDevice::EndFrame() {
    if (m_backend) {
        m_backend->EndFrame();
    }
}

void DX12RenderDevice::Present() {
    if (m_backend) {
        m_backend->Present();
    }
}

} // namespace PrismaEngine::Graphic::DX12