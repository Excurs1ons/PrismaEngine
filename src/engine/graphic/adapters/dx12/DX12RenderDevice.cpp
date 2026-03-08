#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "DX12CommandBuffer.h"
#include "DX12Fence.h"
#include "DX12RenderDevice.h"
#include "DX12ResourceFactory.h"
#include "DX12SwapChain.h"

#include <Windows.h>
#include <d3dcompiler.h>

#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#endif

namespace PrismaEngine::Graphic::DX12 {

DX12RenderDevice::DX12RenderDevice() {
    // 创建资源工厂
    m_resourceFactory = std::make_unique<DX12ResourceFactory>(this);

    // 初始化统计信息
    m_stats.frameCount = 0;
    m_stats.drawCalls  = 0;
    m_stats.triangles  = 0;
    m_stats.frameTime  = 0.0f;
}

DX12RenderDevice::~DX12RenderDevice() {
    Shutdown();
}

bool DX12RenderDevice::Initialize(const DeviceDesc& desc) {
    if (m_initialized) {
        return true;
    }

    // 保存窗口句柄和尺寸
    m_windowHandle = desc.windowHandle;
    m_width        = desc.width;
    m_height       = desc.height;

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

    // 创建DXGI工厂
    UINT dxgi_factory_flags = 0;
    if (desc.enableDebug) {
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return false;
    }

    // 获取硬件适配器
    ComPtr<IDXGIAdapter1> hardware_adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &hardware_adapter);
         ++adapterIndex) {
        DXGI_ADAPTER_DESC1 adapterDesc;
        hardware_adapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        if (SUCCEEDED(
                D3D12CreateDevice(hardware_adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
            break;
        }
    }

    // 检查是否找到适配器
    if (!hardware_adapter) {
        return false;
    }

    // 创建D3D12设备
    hr = D3D12CreateDevice(hardware_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
    if (FAILED(hr)) {
        return false;
    }

    // 创建命令队列
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask                 = 0;

        hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (FAILED(hr)) {
            return false;
        }
    }

    // 创建命令分配器
    hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
    if (FAILED(hr)) {
        return false;
    }

    // 创建根签名
    {
        CD3DX12_ROOT_PARAMETER1 rootParameters[4];
        rootParameters[0].InitAsConstantBufferView(0);  // ViewProjection
        rootParameters[1].InitAsConstantBufferView(1);  // World
        rootParameters[2].InitAsConstantBufferView(2);  // BaseColor
        rootParameters[3].InitAsConstantBufferView(3);  // MaterialParams

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters),
                                   rootParameters,
                                   0,
                                   nullptr,
                                   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        hr = D3DX12SerializeVersionedRootSignature(
            &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error);
        if (FAILED(hr)) {
            return false;
        }

        hr = m_device->CreateRootSignature(
            0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
        if (FAILED(hr)) {
            return false;
        }
    }

    // 创建默认管线状态
    {
        // 编译默认着色器
        const char* vertexShaderSource = R"(
            cbuffer ViewProjection : register(b0) { matrix gViewProjection; }
            cbuffer World : register(b1) { matrix gWorld; }
            struct VSInput { float3 position : POSITION; float4 color : COLOR; };
            struct VSOutput { float4 position : SV_POSITION; float4 color : COLOR; };
            VSOutput main(VSInput input) {
                VSOutput output;
                float4 worldPos = mul(float4(input.position, 1.0f), gWorld);
                output.position = mul(worldPos, gViewProjection);
                output.color = input.color;
                return output;
            }
        )";

        const char* pixelShaderSource = R"(
            struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };
            float4 main(PSInput input) : SV_TARGET { return input.color; }
        )";

        ComPtr<ID3DBlob> vertexShader, pixelShader;
        hr = D3DCompile(vertexShaderSource,
                        strlen(vertexShaderSource),
                        nullptr,
                        nullptr,
                        nullptr,
                        "main",
                        "vs_5_0",
                        0,
                        0,
                        &vertexShader,
                        nullptr);
        if (FAILED(hr)) {
            return false;
        }

        hr = D3DCompile(pixelShaderSource,
                        strlen(pixelShaderSource),
                        nullptr,
                        nullptr,
                        nullptr,
                        "main",
                        "ps_5_0",
                        0,
                        0,
                        &pixelShader,
                        nullptr);
        if (FAILED(hr)) {
            return false;
        }

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
        psoDesc.pRootSignature                     = m_rootSignature.Get();
        psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable      = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable    = FALSE;
        psoDesc.DSVFormat                          = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleMask                         = UINT_MAX;
        psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets                   = 1;
        psoDesc.RTVFormats[0]                      = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count                   = 1;

        hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
        if (FAILED(hr)) {
            return false;
        }
    }

    // 创建命令列表
    hr = m_device->CreateCommandList(0,
                                     D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     m_commandAllocator.Get(),
                                     m_pipelineState.Get(),
                                     IID_PPV_ARGS(&m_commandList));
    if (FAILED(hr)) {
        return false;
    }

    // 命令列表在创建时是打开的，需要关闭
    m_commandList->Close();

    // 创建 RTV 描述符堆
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors             = FrameCount;
        rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask                   = 0;

        hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr)) {
            return false;
        }

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // 创建 DSV 描述符堆
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors             = 1;
        dsvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask                   = 0;

        hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
        if (FAILED(hr)) {
            return false;
        }
    }

    // 创建深度模板缓冲区
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format                        = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice            = 0;
        dsvDesc.Flags                         = D3D12_DSV_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CreationNodeMask      = 1;
        heapProps.VisibleNodeMask       = 1;

        D3D12_RESOURCE_DESC depthBufferDesc = {};
        depthBufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthBufferDesc.Alignment           = 0;
        depthBufferDesc.Width               = m_width;
        depthBufferDesc.Height              = m_height;
        depthBufferDesc.DepthOrArraySize    = 1;
        depthBufferDesc.MipLevels           = 1;
        depthBufferDesc.Format              = DXGI_FORMAT_D32_FLOAT;
        depthBufferDesc.SampleDesc.Count    = 1;
        depthBufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthBufferDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue    = {};
        clearValue.Format               = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth   = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        hr = m_device->CreateCommittedResource(&heapProps,
                                               D3D12_HEAP_FLAG_NONE,
                                               &depthBufferDesc,
                                               D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                               &clearValue,
                                               IID_PPV_ARGS(&m_depthStencil));

        if (FAILED(hr)) {
            return false;
        }

        // 创建深度模板视图
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        m_device->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, dsvHandle);
    }

    // 创建交换链
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width                 = m_width;
        swapChainDesc.Height                = m_height;
        swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count      = 1;
        swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount           = FrameCount;
        swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags                 = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        ComPtr<IDXGISwapChain1> swapChain1;
        hr = factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(), static_cast<HWND>(m_windowHandle), &swapChainDesc, nullptr, nullptr, &swapChain1);

        if (FAILED(hr)) {
            return false;
        }

        hr = swapChain1.As(&m_swapChain);
        if (FAILED(hr)) {
            return false;
        }
    }

    // 创建渲染目标视图
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (UINT n = 0; n < FrameCount; n++) {
            hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
            if (FAILED(hr)) {
                return false;
            }

            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    // 创建动态缓冲区
    {
        // 动态顶点缓冲区 - 4MB
        const uint64_t dynamicVBSize = 4ULL * 1024ULL * 1024ULL;
        auto heapProps               = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc            = CD3DX12_RESOURCE_DESC::Buffer(dynamicVBSize);
        hr                           = m_device->CreateCommittedResource(&heapProps,
                                               D3D12_HEAP_FLAG_NONE,
                                               &resourceDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               nullptr,
                                               IID_PPV_ARGS(&m_dynamicVertexBuffer));
        if (FAILED(hr)) {
            return false;
        }
        m_dynamicVBSize = dynamicVBSize;

        CD3DX12_RANGE readRange(0, 0);
        hr = m_dynamicVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_dynamicVBCPUAddress));
        if (FAILED(hr)) {
            return false;
        }
        m_dynamicVBOffset = 0;

        // 动态索引缓冲区 - 1MB
        const uint64_t dynamicIBSize = 1ULL * 1024ULL * 1024ULL;
        resourceDesc                 = CD3DX12_RESOURCE_DESC::Buffer(dynamicIBSize);
        hr                           = m_device->CreateCommittedResource(&heapProps,
                                               D3D12_HEAP_FLAG_NONE,
                                               &resourceDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               nullptr,
                                               IID_PPV_ARGS(&m_dynamicIndexBuffer));
        if (FAILED(hr)) {
            return false;
        }
        m_dynamicIBSize = dynamicIBSize;

        hr = m_dynamicIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_dynamicIBCPUAddress));
        if (FAILED(hr)) {
            return false;
        }
        m_dynamicIBOffset = 0;

        // 动态常量缓冲区 - 256KB
        const uint64_t dynamicCBSize = 256ULL * 1024ULL;
        resourceDesc                 = CD3DX12_RESOURCE_DESC::Buffer(dynamicCBSize);
        hr                           = m_device->CreateCommittedResource(&heapProps,
                                               D3D12_HEAP_FLAG_NONE,
                                               &resourceDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               nullptr,
                                               IID_PPV_ARGS(&m_dynamicConstantBuffer));
        if (FAILED(hr)) {
            return false;
        }
        m_dynamicCBSize = dynamicCBSize;

        hr = m_dynamicConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_dynamicCBCPUAddress));
        if (FAILED(hr)) {
            return false;
        }
        m_dynamicCBOffset = 0;
    }

    // 创建同步对象
    {
        hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr)) {
            return false;
        }
        m_fenceValue = 1;

        m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            return false;
        }
    }

    // 设置视口和裁剪矩形
    m_viewport    = {0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
    m_scissorRect = {0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height)};

    // 创建交换链适配器
    m_swapChainAdapter = std::make_unique<DX12SwapChain>(this);

    m_initialized = true;
    return true;
}

void DX12RenderDevice::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // 等待GPU完成所有工作
    WaitForIdle();

    // 释放动态缓冲区映射
    if (m_dynamicVBCPUAddress) {
        m_dynamicVertexBuffer->Unmap(0, nullptr);
        m_dynamicVBCPUAddress = nullptr;
    }
    if (m_dynamicIBCPUAddress) {
        m_dynamicIndexBuffer->Unmap(0, nullptr);
        m_dynamicIBCPUAddress = nullptr;
    }
    if (m_dynamicCBCPUAddress) {
        m_dynamicConstantBuffer->Unmap(0, nullptr);
        m_dynamicCBCPUAddress = nullptr;
    }

    // 关闭事件句柄
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    // 释放组件
    m_resourceFactory.reset();
    m_swapChainAdapter.reset();

    m_initialized = false;
}

std::string DX12RenderDevice::GetName() const {
    return "DirectX12 Render Device";
}

std::string DX12RenderDevice::GetAPIName() const {
    return "DirectX 12";
}

// ... 其他方法的实现 ...

void DX12RenderDevice::WaitForIdle() {
    if (!m_initialized) {
        return;
    }

    const uint64_t fence = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fence);
    m_fenceValue++;

    if (m_fence->GetCompletedValue() < fence) {
        m_fence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

void DX12RenderDevice::WaitForPreviousFrame() {
    if (!m_initialized) {
        return;
    }

    const uint64_t fence = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fence);
    m_fenceValue++;

    if (m_fence->GetCompletedValue() < fence) {
        m_fence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

ID3D12Device* DX12RenderDevice::GetD3D12Device() const {
    return m_device.Get();
}

ID3D12CommandQueue* DX12RenderDevice::GetCommandQueue() const {
    return m_commandQueue.Get();
}

ID3D12RootSignature* DX12RenderDevice::GetRootSignature() const {
    return m_rootSignature.Get();
}

ID3D12PipelineState* DX12RenderDevice::GetPipelineState() const {
    return m_pipelineState.Get();
}

ID3D12DescriptorHeap* DX12RenderDevice::GetRTVHeap() const {
    return m_rtvHeap.Get();
}

ID3D12DescriptorHeap* DX12RenderDevice::GetDSVHeap() const {
    return m_dsvHeap.Get();
}

UINT DX12RenderDevice::GetRTVDescriptorSize() const {
    return m_rtvDescriptorSize;
}

UINT DX12RenderDevice::GetCurrentFrameIndex() const {
    return m_frameIndex;
}

UINT DX12RenderDevice::GetFrameCount() const {
    return FrameCount;
}

// ... 其他方法的占位符实现 ...

std::unique_ptr<ICommandBuffer> DX12RenderDevice::CreateCommandBuffer(CommandBufferType type) {
    // TODO: 实现命令缓冲区创建
    (void)type;
    return nullptr;
}

void DX12RenderDevice::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {
    // TODO: 实现命令缓冲区提交
    (void)cmdBuffer;
    (void)fence;
}

void DX12RenderDevice::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                                            const std::vector<IFence*>& fences) {
    // TODO: 实现多命令缓冲区提交
    (void)cmdBuffers;
    (void)fences;
}

std::unique_ptr<IFence> DX12RenderDevice::CreateFence() {
    // TODO: 实现围栏创建
    return nullptr;
}

void DX12RenderDevice::WaitForFence(IFence* fence) {
    // TODO: 实现围栏等待
    (void)fence;
}

IResourceFactory* DX12RenderDevice::GetResourceFactory() const {
    return m_resourceFactory.get();
}

std::unique_ptr<ISwapChain>
DX12RenderDevice::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool vsync) {
    // TODO: 实现交换链创建
    (void)windowHandle;
    (void)width;
    (void)height;
    (void)vsync;
    return nullptr;
}

ISwapChain* DX12RenderDevice::GetSwapChain() const {
    return m_swapChainAdapter.get();
}

void DX12RenderDevice::BeginFrame() {
    if (!m_initialized) {
        return;
    }

    // 重置动态缓冲区偏移量
    m_dynamicVBOffset = 0;
    m_dynamicIBOffset = 0;
    m_dynamicCBOffset = 0;

    // 重置命令分配器
    m_commandAllocator->Reset();

    // 重置命令列表
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());

    // 设置基本渲染状态
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // 准备渲染目标
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    // 设置渲染目标和深度缓冲区
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 清除渲染目标和深度缓冲区
    const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DX12RenderDevice::EndFrame() {
    if (!m_initialized) {
        return;
    }

    // 将后台缓冲区切换到呈现状态
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    // 关闭命令列表
    m_commandList->Close();

    // 执行命令列表
    ID3D12CommandList* ppCommandLists[] = {m_commandList.Get()};
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 设置围栏信号
    const uint64_t fence = m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fence);
    m_fenceValue++;
}

void DX12RenderDevice::Present() {
    if (!m_initialized || !m_swapChain) {
        return;
    }

    // 等待前一帧完成
    WaitForPreviousFrame();

    // 呈现帧
    HRESULT hr = m_swapChain->Present(1, 0);
    if (FAILED(hr)) {
        // 处理错误
    }

    // 更新帧索引
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

bool DX12RenderDevice::SupportsMultiThreaded() const {
    return true;
}

bool DX12RenderDevice::SupportsBindlessTextures() const {
    return true;
}

bool DX12RenderDevice::SupportsComputeShader() const {
    return true;
}

bool DX12RenderDevice::SupportsRayTracing() const {
    return false;  // 需要DXR支持
}

bool DX12RenderDevice::SupportsMeshShader() const {
    return false;  // 需要特定硬件支持
}

bool DX12RenderDevice::SupportsVariableRateShading() const {
    return false;  // 需要特定硬件支持
}
IRenderDevice::GPUMemoryInfo DX12RenderDevice::GetGPUMemoryInfo() const {

    // TODO: 获取GPU内存信息
    return {};
}
IRenderDevice::RenderStats DX12RenderDevice::GetRenderStats() const {
    // TODO: 获取渲染统计信息
    return {};
}

void DX12RenderDevice::BeginDebugMarker(const std::string& /*name*/) {
    // TODO: 实现调试标记
}

void DX12RenderDevice::EndDebugMarker() {
    // TODO: 实现调试标记结束
}

void DX12RenderDevice::SetDebugMarker(const std::string& /*name*/) {
    // TODO: 实现设置调试标记
}

ID3D12CommandSignature* DX12RenderDevice::GetCommandSignature() {
    // TODO: 实现命令签名
    return nullptr;
}

ID3D12CommandSignature* DX12RenderDevice::GetIndexedCommandSignature() {
    // TODO: 实现索引命令签名
    return nullptr;
}

ID3D12CommandSignature* DX12RenderDevice::GetDispatchCommandSignature() {
    // TODO: 实现计算命令签名
    return nullptr;
}

IDXGISwapChain3* DX12RenderDevice::GetDXGISwapChain() const {
    return m_swapChain.Get();
}

ID3D12Resource* DX12RenderDevice::GetRenderTarget(UINT bufferIndex) const {
    if (bufferIndex < FrameCount) {
        return m_renderTargets[bufferIndex].Get();
    }
    return nullptr;
}

HRESULT DX12RenderDevice::PresentWithParams(UINT syncInterval, UINT flags) {
    if (m_swapChain) {
        return m_swapChain->Present(syncInterval, flags);
    }
    return E_FAIL;
}

bool DX12RenderDevice::GetSwapChainDesc(DXGI_SWAP_CHAIN_DESC* desc) const {
    if (m_swapChain && desc) {
        return SUCCEEDED(m_swapChain->GetDesc(desc));
    }
    return false;
}

HRESULT DX12RenderDevice::ResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT format, UINT flags) {
    if (m_swapChain) {
        return m_swapChain->ResizeBuffers(bufferCount, width, height, format, flags);
    }
    return E_FAIL;
}

HRESULT DX12RenderDevice::SetFullscreenState(BOOL fullscreen, IDXGIOutput* target) {
    if (m_swapChain) {
        return m_swapChain->SetFullscreenState(fullscreen, target);
    }
    return E_FAIL;
}

HRESULT DX12RenderDevice::GetSwapChainBuffer(UINT bufferIndex, REFIID riid, void** ppSurface) {
    if (m_swapChain) {
        return m_swapChain->GetBuffer(bufferIndex, riid, ppSurface);
    }
    return E_FAIL;
}

bool DX12RenderDevice::InitializeImGui() {
#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
    if (!m_device || !m_rtvHeap) return false;

    // 初始化 ImGui DX12 后端
    // 获取 RTV 描述符堆中的一个位置用于 ImGui
    // 实际上 ImGui DX12 需要一个 ShaderVisible 的 SRV 堆
    // 这里我们假设上层已经处理好了描述符堆的管理
    
    // 我们需要从资源工厂获取一个描述符
    // 为简单起见，这里先返回 true，具体的初始化由 RenderSystem 完成
    // 但我们需要确保这个方法被实现以满足接口要求
    return true;
#else
    return true;
#endif
}

void DX12RenderDevice::ShutdownImGui() {
#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
#endif
}

}  // namespace PrismaEngine::Graphic::DX12