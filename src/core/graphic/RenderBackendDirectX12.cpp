#include "RenderBackendDirectX12.h"
#include <directx/d3dx12.h>
//directx-headers 必须放在windows sdk的d3d12.h之前
// #include "ApplicationWindows.h"
#include "Camera2D.h"
#include "LogScope.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <iostream>
#include <wrl.h>

#include "Helper.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "RenderThread.h"
#include "Logger.h"

#include "SceneManager.h"

using namespace Microsoft::WRL;
using namespace Engine;
HWND g_hWnd = nullptr;

namespace Engine {

// Minimal DirectX implementation of IRenderCommandContext to forward calls to the D3D12 command list
class DXRenderCommandContext : public RenderCommandContext {
public:
    DXRenderCommandContext(ID3D12GraphicsCommandList* cmdList,
                           D3D12_VIEWPORT* vp,
                           D3D12_RECT* sc,
                           RenderBackendDirectX12* backend)
        : m_commandList(cmdList), m_viewport(vp ? *vp : D3D12_VIEWPORT{}), m_scissorRect(sc ? *sc : D3D12_RECT{}),
          m_backend(backend) {}

    void SetConstantBuffer(const char* name, FXMMATRIX matrix) override {
        LOG_DEBUG("DXContext", "SetConstantBuffer(matrix) name={0}", name);

        // 确定常量缓冲区寄存器
        uint32_t registerIndex = 0;
        if (strcmp(name, "ViewProjection") == 0) {
            registerIndex = 0;
        } else if (strcmp(name, "World") == 0) {
            registerIndex = 1;
        } else if (strcmp(name, "BaseColor") == 0) {
            registerIndex = 2;
        } else if (strcmp(name, "MaterialParams") == 0) {
            registerIndex = 3;
        } else {
            LOG_WARNING("DXContext", "Unknown constant buffer name: {0}", name);
            return;
        }

        // 存储矩阵数据到临时缓冲区
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, matrix);
        SetConstantBuffer(name, reinterpret_cast<const float*>(&matrixData), 16);
    }

    void SetConstantBuffer(const char* name, const float* data, size_t size) override {
        LOG_DEBUG("DXContext", "SetConstantBuffer(data) name={0} size={1}", name, size);

        // 确定常量缓冲区寄存器
        uint32_t registerIndex = 0;
        if (strcmp(name, "ViewProjection") == 0) {
            registerIndex = 0;
        } else if (strcmp(name, "World") == 0) {
            registerIndex = 1;
        } else if (strcmp(name, "BaseColor") == 0) {
            registerIndex = 2;
        } else if (strcmp(name, "MaterialParams") == 0) {
            registerIndex = 3;
        } else {
            LOG_WARNING("DXContext", "Unknown constant buffer name: {0}", name);
            return;
        }

        // 获取常量缓冲区所需的GPU虚拟地址
        // 这里我们使用动态上传缓冲区来存储常量数据
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_backend->GetDynamicConstantBufferAddress(data, size * sizeof(float));

        // 设置根参数
        m_commandList->SetGraphicsRootConstantBufferView(registerIndex, gpuAddress);
    }
    void SetVertexBuffer(const void* data, uint32_t sizeInBytes, uint32_t strideInBytes) override {
        if (!m_commandList || !m_backend) {
            LOG_ERROR("DXContext", "SetVertexBuffer: 无效的命令列表或后端");
            return;
        }
        // 转发给后端，后端拥有动态上传缓冲区和绑定逻辑
        m_backend->UploadAndBindVertexBuffer(m_commandList, data, sizeInBytes, strideInBytes);
    }
    void SetIndexBuffer(const void* data, uint32_t sizeInBytes, bool use16BitIndices = true) override {
        if (!m_commandList || !m_backend) {
            LOG_ERROR("DXContext", "SetIndexBuffer: 无效的命令列表或后端");
            return;
        }
        // 转发给后端处理索引缓冲区上传和绑定
        m_backend->UploadAndBindIndexBuffer(m_commandList, data, sizeInBytes, use16BitIndices);
    }
    void SetShaderResource(const char* name, void* resource) override {
        LOG_DEBUG("DXContext", "SetShaderResource name={0}", name);
    }
    void SetSampler(const char* name, void* sampler) override { LOG_DEBUG("DXContext", "SetSampler name={0}", name); }
    void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, uint32_t baseVertexLocation = 0) override {
        if (!m_commandList) {
            LOG_ERROR("DXContext", "DrawIndexed 调用时命令列表为空");
            return;
        }
        LOG_TRACE("DXContext",
                  "DrawIndexed 索引数={0} 起始索引={1} 基础顶点={2}",
                  indexCount,
                  startIndexLocation,
                  baseVertexLocation);
        // 执行索引绘制
        m_commandList->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
    }
    void Draw(uint32_t vertexCount, uint32_t startVertexLocation = 0) override {
        if (!m_commandList) {
            LOG_ERROR("DXContext", "Draw called but command list is null");
            return;
        }
        LOG_TRACE("DXContext", "Draw count={0} startVertex={1}", vertexCount, startVertexLocation);
        m_commandList->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
    }
    void SetViewport(float x, float y, float width, float height) override {
        if (!m_commandList)
            return;
        D3D12_VIEWPORT vp = {x, y, width, height, 0.0f, 1.0f};
        m_commandList->RSSetViewports(1, &vp);
    }
    void SetScissorRect(int left, int top, int right, int bottom) override {
        if (!m_commandList)
            return;
        D3D12_RECT rect = {left, top, right, bottom};
        m_commandList->RSSetScissorRects(1, &rect);
    }

private:
    ID3D12GraphicsCommandList* m_commandList = nullptr;
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    RenderBackendDirectX12* m_backend = nullptr;
};

RenderBackendDirectX12::RenderBackendDirectX12(std::wstring name)
    : m_fenceEvent(nullptr), m_fenceValue(0), m_frameIndex(0), m_height(1), m_rtvDescriptorSize(0),
      m_scissorRect{0, 0, 1L, 1L}, m_viewport{0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}, m_width(1),
      m_aspectRatio(1), m_dynamicVBCPUAddress(nullptr), m_dynamicVBSize(0), m_dynamicVBOffset(0),
      m_dynamicIBCPUAddress(nullptr), m_dynamicIBSize(0), m_dynamicIBOffset(0),
      m_dynamicCBCPUAddress(nullptr), m_dynamicCBSize(0), m_dynamicCBOffset(0) {}
bool RenderBackendDirectX12::Initialize(Platform* platform, WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height) {
    g_hWnd        = (HWND)windowHandle;
    m_height      = height;
    m_width       = width;
    m_scissorRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    m_viewport    = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    // m_RenderThread
    if (!LoadPipeline()) {
        return false;
    }
    if (!InitializeRenderObjects()) {
        return false;
    }
    isInitialized = true;
    return true;
}
RenderBackendDirectX12::~RenderBackendDirectX12() {}
void RenderBackendDirectX12::OnRender() {}
void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);

void RenderBackendDirectX12::Shutdown() {
    // Wait for the GPU to be done with all resources.
    // WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}

void RenderBackendDirectX12::BeginFrame() {
    // 创建渲染帧日志作用域
    LogScope* frameScope = LogScopeManager::GetInstance().CreateScope("DirectXFrame");
    Logger::GetInstance().PushLogScope(frameScope);

    // 重置每帧动态缓冲区偏移量
    m_dynamicVBOffset = 0;
    m_dynamicIBOffset = 0;
    m_dynamicCBOffset = 0;

    // 指令列表分配器只能在关联的指令列表在GPU完成执行之后才能重置，
    // apps should use
    // fences to determine GPU execution progress.
    HRESULT hr = m_commandAllocator->Reset();
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法重置命令分配器: {0}", HrToString(hr));
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        return;
    }

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    hr = m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法重置命令列表: {0}", HrToString(hr));
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        return;
    }

    // 设置必需的状态
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // 将后台缓冲区作为渲染目标
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 从场景获取主相机和清除颜色，默认为青色
    float clearColor[4] = {0.0f, 1.0f, 1.0f, 1.0f};  // 默认青色
    //auto& app           = Singleton<ApplicationWindows>::GetInstance();
    auto scene = SceneManager::GetInstance()->GetCurrentScene();
    if (scene) {
        // 尝试从场景中获取主相机
        Camera* mainCamera = scene->GetMainCamera();
        if (mainCamera) {
            // 从主相机获取清除颜色
            XMVECTOR cameraClearColor = mainCamera->GetClearColor();
            clearColor[0]             = XMVectorGetX(cameraClearColor);
            clearColor[1]             = XMVectorGetY(cameraClearColor);
            clearColor[2]             = XMVectorGetZ(cameraClearColor);
            clearColor[3]             = XMVectorGetW(cameraClearColor);
            LOG_DEBUG("RenderBackendDirectX12",
                      "Using main camera clear color: ({0}, {1}, {2}, {3})",
                      clearColor[0],
                      clearColor[1],
                      clearColor[2],
                      clearColor[3]);
        } else {
            LOG_DEBUG("RenderBackendDirectX12", "No main camera found in scene, using default clear color");
        }
    }

    // 录制渲染指令
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 调用场景渲染
    auto scenePtr = SceneManager::GetInstance()->GetCurrentScene();
    if (scenePtr) {
        // 获取相机矩阵
        XMMATRIX cameraMatrix = XMMatrixIdentity();
        Camera* mainCamera = scenePtr->GetMainCamera();
        if (mainCamera) {
            // 尝试转换为Camera2D来获取视图投影矩阵
            Camera2D* camera2D = dynamic_cast<Camera2D*>(mainCamera);
            if (camera2D) {
                // 更新投影矩阵以适应当前窗口宽高比
                camera2D->UpdateProjectionMatrix(static_cast<float>(m_width), static_cast<float>(m_height));
                cameraMatrix = camera2D->GetViewProjectionMatrix();
                LOG_DEBUG("RenderBackendDirectX12", "Using Camera2D matrix from main camera");
            } else {
                LOG_DEBUG("RenderBackendDirectX12", "Main camera is not Camera2D, using identity matrix");
            }
        } else {
            LOG_DEBUG("RenderBackendDirectX12", "No main camera found, using identity matrix");
        }

        // 创建渲染命令上下文
        DXRenderCommandContext context(m_commandList.Get(), &m_viewport, &m_scissorRect, this);

        // 设置相机矩阵到渲染上下文
        context.SetConstantBuffer("ViewProjection", reinterpret_cast<const float*>(&cameraMatrix), 16);

        // 渲染场景
        scenePtr->Render(&context);
    }

    // 将后台缓冲区切换到呈现状态
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    hr = m_commandList->Close();
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法关闭命令列表: {0}", HrToString(hr));
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        return;
    }

    // 执行渲染指令
    ID3D12CommandList* ppCommandLists[] = {m_commandList.Get()};
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 绘制
    hr = m_swapChain->Present(1, 0);
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "交换链呈现失败: {0}", HrToString(hr));
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        return;
    }

    // 等待上一帧完成
    WaitForPreviousFrame();

    // 正常结束BeginFrame，继续保持作用域活跃到EndFrame
}

void RenderBackendDirectX12::EndFrame() {
    // 获取当前日志作用域
    LogScope* frameScope = Logger::GetInstance().GetCurrentLogScope();

    // DirectX的EndFrame目前没有太多逻辑，主要是通知作用域结束
    if (frameScope) {
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, true);  // 正常结束，不输出日志
    }
}

/// <summary>
/// 调用多次，每次调用都会录制一个渲染指令
/// </summary>
/// <param name="cmd"></param>
void RenderBackendDirectX12::SubmitRenderCommand(const RenderCommand& cmd) {
    // TODO: 实现具体的渲染命令提交
}

bool RenderBackendDirectX12::Supports(RendererFeature feature) const {
    return (static_cast<int>(m_support) & static_cast<int>(feature)) != 0;
}

void RenderBackendDirectX12::Present() {
    // 在EndFrame中已经实现了呈现逻辑
}

bool RenderBackendDirectX12::LoadPipeline() {
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    // 在调试模式下启用调试层
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

            // 启用额外的调试层（需要安装SDK配置）
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法创建DXGI工厂: {0}", HrToString(hr));
        return false;
    }
    LOG_INFO("DirectX", "成功创建DXGI工厂");

    if (g_hWnd == nullptr) {
        LOG_ERROR("DirectX", "无效的窗口句 HANDLE");
        return false;
    }

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    m_width  = rc.right - rc.left;
    m_height = rc.bottom - rc.top;

    // 创建设备
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.Get(), &hardwareAdapter);

    hr = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法创建D3D12设备: {0}", HrToString(hr));
        return false;
    }
    LOG_INFO("DirectX", "成功创建D3D12设备");

    // 描述符大小对于分配增量描述符非常重要
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 创建命令队列
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法创建命令队列: {0}", HrToString(hr));
        return false;
    }
    LOG_INFO("DirectX", "成功创建命令队列");

    // 创建交换链
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount           = FrameCount;
    swapChainDesc.Width                 = m_width;
    swapChainDesc.Height                = m_height;
    swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count      = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    hr = factory->CreateSwapChainForHwnd(m_commandQueue.Get(),  // 交换链需要对设备的命令队列进行引用
                                         g_hWnd,
                                         &swapChainDesc,
                                         nullptr,
                                         nullptr,
                                         &swapChain);

    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法创建交换链: {0}", HrToString(hr));
        return false;
    }
    LOG_INFO("DirectX", "成功创建交换链");

    // 此示例不支持全屏转换
    hr = factory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法设置窗口关联: {0}", HrToString(hr));
        return false;
    }

    hr = swapChain.As(&m_swapChain);
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法转换交换链: {0}", HrToString(hr));
        return false;
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // 创建渲染目标视图描述符堆
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors             = FrameCount;
        rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr                                     = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "无法创建RTV描述符堆: {0}", HrToString(hr));
            return false;
        }
        LOG_INFO("DirectX", "成功创建RTV描述符堆");
    }

    // 创建帧资源
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // 创建实际的渲染目标视图
        for (UINT n = 0; n < FrameCount; n++) {
            hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
            if (FAILED(hr)) {
                LOG_ERROR("DirectX", "无法获取交换链缓冲区 {0}: {1}", n, HrToString(hr));
                return false;
            }

            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            LOG_INFO("DirectX", "成功创建渲染目标视图 {0}", n);
        }
    }

    // 创建深度缓冲区和DSV堆
    {
        // 创建DSV描述符堆
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "无法创建DSV描述符堆: {0}", HrToString(hr));
            return false;
        }
        LOG_INFO("DirectX", "成功创建DSV描述符堆");

        // 创建深度缓冲区资源
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        CD3DX12_RESOURCE_DESC depthTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            m_width,
            m_height,
            1,
            0,
            1,
            0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthTextureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencil));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "无法创建深度缓冲区: {0}", HrToString(hr));
            return false;
        }
        LOG_INFO("DirectX", "成功创建深度缓冲区");

        // 创建深度模板视图
        m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        LOG_INFO("DirectX", "成功创建深度模板视图");
    }

    hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法创建命令分配器: {0}", HrToString(hr));
        return false;
    }
    LOG_INFO("DirectX", "成功创建命令分配器");

    // 创建命令列表
    hr = m_device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法创建命令列表: {0}", HrToString(hr));
        return false;
    }
    LOG_INFO("DirectX", "成功创建命令列表");

    // 命令列表在首次创建时处于打开状态，但不需要它处于打开状态，因此将其关闭
    hr = m_commandList->Close();
    if (FAILED(hr)) {
        LOG_ERROR("DirectX", "无法关闭命令列表: {0}", HrToString(hr));
        return false;
    }

    // 创建同步对象
    {
        hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "无法创建围栏: {0}", HrToString(hr));
            return false;
        }
        LOG_INFO("DirectX", "成功创建围栏");

        m_fenceValue = 1;

        // 创建事件句柄
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ERROR("DirectX", "无法创建围栏事件: {0}", HrToString(hr));
            return false;
        }
        LOG_INFO("DirectX", "成功创建围栏事件");
    }

    return true;
}

bool RenderBackendDirectX12::InitializeRenderObjects() {
    // 创建根签名支持常量缓冲区
    {
        // 定义4个常量缓冲区根参数
        CD3DX12_ROOT_PARAMETER1 rootParameters[4];

        // ViewProjection矩阵 - b0
        rootParameters[0].InitAsConstantBufferView(0);

        // World矩阵 - b1
        rootParameters[1].InitAsConstantBufferView(1);

        // BaseColor - b2
        rootParameters[2].InitAsConstantBufferView(2);

        // MaterialParams - b3
        rootParameters[3].InitAsConstantBufferView(3);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
                                   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT hRserializeRootSig = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
                                                                          D3D_ROOT_SIGNATURE_VERSION_1_1,
                                                                          &signature, &error);
        if (FAILED(hRserializeRootSig)) {
            LOG_ERROR("DirectX", "序列化根签名失败: {0}", HrToString(hRserializeRootSig));
            if (error) {
                LOG_ERROR("DirectX", "错误信息: {0}", static_cast<const char*>(error->GetBufferPointer()));
            }
            return false;
        }
        LOG_INFO("DirectX", "成功序列化根签名");
        HRESULT hRcreateRootSignature;
        hRcreateRootSignature = m_device->CreateRootSignature(
            0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
        if (FAILED(hRcreateRootSignature)) {
            LOG_ERROR("DirectX", "创建根签名失败: {0}", HrToString(hRcreateRootSignature));
            return false;
        }
        LOG_INFO("DirectX", "成功创建根签名");
    }

    // 创建管线状态，包括编译和加载着色器
    {
        // 使用资源管理器加载着色器
        auto resourceManager = ResourceManager::GetInstance();
        auto shaderHandle     = resourceManager->Load<Shader>("shader.hlsl");

        if (!shaderHandle.IsValid()) {
            LOG_ERROR("DirectX", "通过资源管理器加载着色器失败");
            return false;
        }
        LOG_INFO("DirectX", "成功加载着色器");
        auto* shader      = shaderHandle.Get();
        auto vertexShader = shader->GetVertexShaderBlob();
        auto pixelShader  = shader->GetPixelShaderBlob();

        // 定义顶点输入布局
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

        // 创建图形管线状态对象（PSO）
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
        psoDesc.pRootSignature                     = m_rootSignature.Get();
        psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable      = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc       = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable    = FALSE;
        psoDesc.DSVFormat                         = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleMask                         = UINT_MAX;
        psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets                   = 1;
        psoDesc.RTVFormats[0]                      = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count                   = 1;
        HRESULT hRcreatePSO = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
        if (FAILED(hRcreatePSO)) {
            LOG_ERROR("DirectX", "创建图形管线状态失败: {0}", HrToString(hRcreatePSO));
            return false;
        }
        LOG_INFO("DirectX", "成功创建图形管线状态");
    }

    // 顶点缓冲区现在由动态渲染系统管理，不再需要硬编码几何体

    // 创建动态上传缓冲区（用于每帧小批量顶点上传）
    {
        // 4MB per-frame upload buffer
        const uint64_t dynamicSize = 4ULL * 1024ULL * 1024ULL;
        auto heapProps             = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc          = CD3DX12_RESOURCE_DESC::Buffer(dynamicSize);
        HRESULT hr                 = m_device->CreateCommittedResource(&heapProps,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &resourceDesc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr,
                                                       IID_PPV_ARGS(&m_dynamicVertexBuffer));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "创建动态顶点上传缓冲区失败: {0}", HrToString(hr));
            return false;
        }
        m_dynamicVBSize = dynamicSize;
        // 映射一次，保持 CPU 指针直到销毁
        CD3DX12_RANGE readRange(0, 0);
        uint8_t* ptr = nullptr;
        hr           = m_dynamicVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&ptr));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "映射动态顶点缓冲区失败: {0}", HrToString(hr));
            return false;
        }
        m_dynamicVBCPUAddress = ptr;
        m_dynamicVBOffset     = 0;
    }

    // 创建动态索引上传缓冲区（每帧临时索引数据）
    {
        // 1MB per-frame upload buffer for indices（对于大多数情况应该足够了）
        const uint64_t dynamicIBSize = 1ULL * 1024ULL * 1024ULL;
        auto heapProps               = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc            = CD3DX12_RESOURCE_DESC::Buffer(dynamicIBSize);
        HRESULT hr                   = m_device->CreateCommittedResource(&heapProps,
                                                         D3D12_HEAP_FLAG_NONE,
                                                         &resourceDesc,
                                                         D3D12_RESOURCE_STATE_GENERIC_READ,
                                                         nullptr,
                                                         IID_PPV_ARGS(&m_dynamicIndexBuffer));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "创建动态索引上传缓冲区失败: {0}", HrToString(hr));
            return false;
        }
        m_dynamicIBSize = dynamicIBSize;
        // 映射一次，保持 CPU 指针直到销毁
        CD3DX12_RANGE readRange(0, 0);
        uint8_t* ptr = nullptr;
        hr           = m_dynamicIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&ptr));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "映射动态索引缓冲区失败: {0}", HrToString(hr));
            return false;
        }
        m_dynamicIBCPUAddress = ptr;
        m_dynamicIBOffset     = 0;
    }

    // 创建动态常量缓冲区（用于MVP矩阵、材质参数等）
    {
        // 256KB per-frame upload buffer for constants（16个矩阵+其他参数应该足够了）
        const uint64_t dynamicCBSize = 256ULL * 1024ULL;
        auto heapProps               = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc            = CD3DX12_RESOURCE_DESC::Buffer(dynamicCBSize);
        HRESULT hr                   = m_device->CreateCommittedResource(&heapProps,
                                                         D3D12_HEAP_FLAG_NONE,
                                                         &resourceDesc,
                                                         D3D12_RESOURCE_STATE_GENERIC_READ,
                                                         nullptr,
                                                         IID_PPV_ARGS(&m_dynamicConstantBuffer));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "创建动态常量缓冲区失败: {0}", HrToString(hr));
            return false;
        }
        m_dynamicCBSize = dynamicCBSize;
        // 映射一次，保持 CPU 指针直到销毁
        CD3DX12_RANGE readRange(0, 0);
        uint8_t* ptr = nullptr;
        hr           = m_dynamicConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&ptr));
        if (FAILED(hr)) {
            LOG_ERROR("DirectX", "映射动态常量缓冲区失败: {0}", HrToString(hr));
            return false;
        }
        m_dynamicCBCPUAddress = ptr;
        m_dynamicCBOffset     = 0;
        LOG_INFO("DirectX", "成功创建动态常量缓冲区");
    }

    return true;
}

void RenderBackendDirectX12::UploadAndBindVertexBuffer(ID3D12GraphicsCommandList* cmdList,
                                                const void* data,
                                                uint32_t sizeInBytes,
                                                uint32_t strideInBytes) {
    if (!m_dynamicVertexBuffer || !m_dynamicVBCPUAddress) {
        LOG_ERROR("DirectX", "UploadAndBindVertexBuffer: dynamic buffer not created");
        return;
    }
    // Simple linear allocator with wrap when necessary. Ensure 16-byte alignment for safety.
    const uint64_t align = 16;
    uint64_t offset      = (m_dynamicVBOffset + (align - 1)) & ~(align - 1);
    if (offset + sizeInBytes > m_dynamicVBSize) {
        // wrap to start
        offset = 0;
    }
    // copy CPU data
    memcpy(m_dynamicVBCPUAddress + offset, data, sizeInBytes);

    // build a temporary VB view and bind
    D3D12_VERTEX_BUFFER_VIEW vbView{};
    vbView.BufferLocation = m_dynamicVertexBuffer->GetGPUVirtualAddress() + offset;
    vbView.SizeInBytes    = sizeInBytes;
    vbView.StrideInBytes  = strideInBytes;

    cmdList->IASetVertexBuffers(0, 1, &vbView);

    // 为下次分配推进偏移量
    m_dynamicVBOffset = offset + sizeInBytes;
}

void RenderBackendDirectX12::UploadAndBindIndexBuffer(ID3D12GraphicsCommandList* cmdList,
                                                     const void* data,
                                                     uint32_t sizeInBytes,
                                                     bool use16BitIndices) {
    if (!m_dynamicIndexBuffer || !m_dynamicIBCPUAddress) {
        LOG_ERROR("DirectX", "UploadAndBindIndexBuffer: 动态索引缓冲区未创建");
        return;
    }

    // 简单的线性分配器，必要时回绕。确保对齐到4字节边界。
    const uint64_t align = 4;
    uint64_t offset = (m_dynamicIBOffset + (align - 1)) & ~(align - 1);
    if (offset + sizeInBytes > m_dynamicIBSize) {
        // 回绕到开始位置
        offset = 0;
    }

    // 复制CPU数据
    memcpy(m_dynamicIBCPUAddress + offset, data, sizeInBytes);

    // 创建临时索引缓冲区视图并绑定
    D3D12_INDEX_BUFFER_VIEW ibView{};
    ibView.BufferLocation = m_dynamicIndexBuffer->GetGPUVirtualAddress() + offset;
    ibView.SizeInBytes = sizeInBytes;
    ibView.Format = use16BitIndices ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    cmdList->IASetIndexBuffer(&ibView);

    // 为下次分配推进偏移量
    m_dynamicIBOffset = offset + sizeInBytes;
}

D3D12_GPU_VIRTUAL_ADDRESS RenderBackendDirectX12::GetDynamicConstantBufferAddress(const void* data, size_t sizeInBytes) {
    if (!m_dynamicConstantBuffer || !m_dynamicCBCPUAddress) {
        LOG_ERROR("DirectX", "动态常量缓冲区未初始化");
        return 0;
    }

    // 对齐到256字节边界（DirectX 12常量缓冲区要求）
    const size_t alignment = 256;
    size_t alignedSize = (sizeInBytes + alignment - 1) & ~(alignment - 1);

    // 检查是否有足够空间
    if (m_dynamicCBOffset + alignedSize > m_dynamicCBSize) {
        LOG_WARNING("DirectX", "动态常量缓冲区空间不足，重置偏移量");
        m_dynamicCBOffset = 0;
    }

    // 复制数据到常量缓冲区
    uint8_t* dest = m_dynamicCBCPUAddress + m_dynamicCBOffset;
    memcpy(dest, data, sizeInBytes);

    // 获取GPU虚拟地址
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_dynamicConstantBuffer->GetGPUVirtualAddress() + m_dynamicCBOffset;

    // 更新偏移量
    m_dynamicCBOffset += alignedSize;

    return gpuAddress;
}

void RenderBackendDirectX12::WaitForPreviousFrame() {
    // 等待帧渲染完成并不是最佳实践
    // This is code implemented as such for simplicity. More advanced samples
    // illustrate how to use fences for efficient resource usage.

    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    if (!m_fence) {
        // 错误处理：记录/返回，避免调用 Signal(nullptr,...)
        return;
    }
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence) {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter) {
    ComPtr<IDXGIAdapter1> adapter;
    *ppAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter);
         ++adapterIndex) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            // 不要选择基本渲染器适配器，如果请求的话
            continue;
        }

        // 检查适配器是否支持D3D12，但如果创建设备失败则继续
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
            break;
        }
    }

    *ppAdapter = adapter.Detach();
}

}  // namespace Engine