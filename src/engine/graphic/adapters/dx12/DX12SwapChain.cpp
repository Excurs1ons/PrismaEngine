#include "DX12SwapChain.h"

#include "DX12Texture.h"
#include "Logger.h"
#include <dxgi1_6.h>
#include <sstream>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

DX12SwapChain::DX12SwapChain(DX12RenderDevice* device)
    : m_device(device)
{
    // 初始化统计信息
    ResetStats();
    InitializeStats();

    // 创建渲染目标适配器
    CreateRenderTargetAdapters();
}

DX12SwapChain::~DX12SwapChain() {
    // 清理渲染目标适配器
    m_renderTargets.clear();
}

// ISwapChain接口实现
uint32_t DX12SwapChain::GetBufferCount() const {
    if (m_device) {
        return m_device->GetFrameCount(); // DirectX12使用双缓冲
    }
    return 0;
}

uint32_t DX12SwapChain::GetCurrentBufferIndex() const {
    if (m_device) {
        return m_device->GetCurrentFrameIndex();
    }
    return 0;
}

uint32_t DX12SwapChain::GetWidth() const {
    if (m_device) {
        // TODO: 从DX12RenderDevice获取尺寸
        return 1920; // 临时返回默认值
    }
    return 0;
}

uint32_t DX12SwapChain::GetHeight() const {
    if (m_device) {
        // TODO: 从DX12RenderDevice获取尺寸
        return 1080; // 临时返回默认值
    }
    return 0;
}

TextureFormat DX12SwapChain::GetFormat() const {
    // 默认格式
    return TextureFormat::RGBA8_UNorm;
}

SwapChainMode DX12SwapChain::GetMode() const {
    return m_mode;
}

bool DX12SwapChain::IsHDR() const {
    return m_hdr;
}

ITexture* DX12SwapChain::GetRenderTarget(uint32_t bufferIndex) {
    if (bufferIndex < m_renderTargets.size()) {
        return m_renderTargets[bufferIndex].get();
    }
    return nullptr;
}

ITexture* DX12SwapChain::GetCurrentRenderTarget() {
    uint32_t index = GetCurrentBufferIndex();
    return GetRenderTarget(index);
}

bool DX12SwapChain::Present() {
    if (!m_device) {
        return false;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // 根据模式决定呈现方式
    UINT syncInterval = 1;
    UINT flags = 0;

    switch (m_mode) {
        case SwapChainMode::Immediate:
            syncInterval = 0;
            flags = DXGI_PRESENT_ALLOW_TEARING;
            break;
        case SwapChainMode::VSync:
            syncInterval = 1;
            break;
        case SwapChainMode::AdaptiveVSync:
            // 自适应垂直同步，这里简化为1
            syncInterval = 1;
            break;
        case SwapChainMode::TripleBuffer:
            syncInterval = 1;
            // 三重缓冲需要在创建时指定
            break;
    }

    // 呈现
    HRESULT hr = m_device->Present();
    bool success = SUCCEEDED(hr);

    // 更新统计信息
    auto endTime = std::chrono::high_resolution_clock::now();
    float frameTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    m_stats.totalFrames++;
    m_stats.executionTime = frameTime;

    // 更新帧率
    UpdateStats();

    return success;
}

bool DX12SwapChain::SetMode(SwapChainMode mode) {
    m_mode = mode;
    return true; // 成功设置
}

bool DX12SwapChain::Resize(uint32_t width, uint32_t height) {
    if (!m_device || !m_device->GetDXGISwapChain()) {
        return false;
    }

    // 等待GPU完成所有帧
    for (UINT i = 0; i < 2; ++i) {
        m_device->WaitForPreviousFrame();
    }

    // 调整大小
    DXGI_SWAP_CHAIN_DESC desc;
    if (!m_device->GetSwapChainDesc(&desc)) {
        return false;
    }

    HRESULT hr = m_device->ResizeBuffers(
        desc.BufferCount,
        width,
        height,
        desc.BufferDesc.Format,
        desc.Flags
    );

    bool success = SUCCEEDED(hr);
    if (success) {
        LOG_INFO("SwapChain", "交换链大小调整为: {0}x{1}", width, height);
    }

    return success;
}

bool DX12SwapChain::SetHDR(bool enable) {
    m_hdr = enable;
    // HDR支持需要在创建交换链时指定
    // 这里暂时只设置标志
    return true;
}

const char* DX12SwapChain::GetColorSpace() const {
    return m_colorSpace.c_str();
}

bool DX12SwapChain::SetColorSpace(const char* colorSpace) {
    m_colorSpace = colorSpace;
    // 设置DXGI颜色空间
    return true;
}

float DX12SwapChain::GetFrameRate() const {
    return m_stats.frameRate;
}

float DX12SwapChain::GetFrameTime() const {
    return m_stats.executionTime;
}

DX12SwapChain::PresentStats DX12SwapChain::GetPresentStats() const {
    return m_stats;
}

void DX12SwapChain::ResetStats() {
    m_stats = {};
    m_stats.totalFrames = 0;
}

bool DX12SwapChain::IsFullscreen() const {
    if (!m_device || !m_device->GetDXGISwapChain()) {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc;
    if (!m_device->GetSwapChainDesc(&desc)) {
        return false;
    }
    return desc.Windowed != 1;
}

bool DX12SwapChain::SetFullscreen(bool fullscreen) {
    if (!m_device || !m_device->GetDXGISwapChain()) {
        return false;
    }

    // 设置全屏
    HRESULT hr = m_device->SetFullscreenState(fullscreen, nullptr);
    return SUCCEEDED(hr);
}

bool DX12SwapChain::Screenshot(const std::string& filename, uint32_t bufferIndex) {
    if (!m_device || !m_device->GetDXGISwapChain()) {
        return false;
    }

    // 获取后台缓冲区
    ComPtr<ID3D12Resource> backBuffer;
    HRESULT hr = m_device->GetSwapChainBuffer(bufferIndex, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) {
        LOG_ERROR("SwapChain", "无法获取后台缓冲区");
        return false;
    }

    // 映射缓冲区
    D3D12_RANGE readRange = {0, 0};
    void* data = nullptr;
    hr = backBuffer->Map(0, &readRange, &data);
    if (FAILED(hr)) {
        LOG_ERROR("SwapChain", "无法映射后台缓冲区");
        return false;
    }

    // 获取缓冲区信息
    D3D12_RESOURCE_DESC desc = backBuffer->GetDesc();
    uint32_t width = static_cast<uint32_t>(desc.Width);
    uint32_t height = static_cast<uint32_t>(desc.Height);

    // 保存为BMP（简化实现）
    // 这里需要使用图像库来保存
    LOG_INFO("SwapChain", "截图功能待实现: {0}x{1}", width, height);

    // 取消映射
    backBuffer->Unmap(0, nullptr);

    return false; // 暂时返回false
}

void DX12SwapChain::EnableDebugLayer(bool enable) {
    // 启用调试层
    LOG_INFO("SwapChain", "调试层: {0}", enable ? "启用" : "禁用");
}

// DirectX12特定方法
std::unique_ptr<DX12SwapChain> DX12SwapChain::Clone() const {
    if (!m_device) {
        return nullptr;
    }

    // auto clone = std::make_unique<DX12SwapChain>(GetDevice());
    // clone->m_mode = m_mode;
    // clone->m_hdr = m_hdr;
    // clone->m_colorSpace = m_colorSpace;
    //
    // return clone;
    return nullptr;
}

IDXGISwapChain3* DX12SwapChain::GetDXGISwapChain() const {
    return m_device ? m_device->GetDXGISwapChain() : nullptr;
}

// 私有方法
void DX12SwapChain::UpdateStats() const {
    // 更新帧率（每秒更新一次）
    static float fpsAccumulator = 0.0f;
    static uint32_t fpsFrameCount = 0;
    static float fpsUpdateTime = 0.0f;

    fpsAccumulator += 1.0f / m_stats.executionTime;
    fpsFrameCount++;
    fpsUpdateTime += m_stats.executionTime;

    if (fpsUpdateTime >= 1.0f) {
        m_stats.frameRate = fpsAccumulator / fpsFrameCount;
        fpsAccumulator = 0.0f;
        fpsFrameCount = 0;
        fpsUpdateTime = 0.0f;
    }
}

void DX12SwapChain::CreateRenderTargetAdapters() {
    if (m_device == nullptr) {
        return;
    }

    // 为每个缓冲区创建渲染目标适配器
    m_renderTargets.resize(2);

    for (UINT i = 0; i < 2; ++i) {
        if (m_device->GetRenderTarget(i)) {
            // 创建纹理适配器包装现有的渲染目标
            ComPtr<ID3D12Resource> resource;
            HRESULT hr = m_device->GetSwapChainBuffer(i, IID_PPV_ARGS(&resource));

            if (SUCCEEDED(hr)) {
                TextureDesc desc;
                desc.type = TextureType::Texture2D;
                desc.width = m_device->GetViewport().Width;
                desc.height = m_device->GetViewport().Height;
                desc.format = TextureFormat::RGBA8_UNorm;
                desc.allowRenderTarget = true;
                desc.name = "SwapChainRenderTarget_" + std::to_string(i);

                // 创建纹理适配器
                m_renderTargets[i] = std::make_unique<DX12Texture>(
                    (m_device != nullptr) ? nullptr : nullptr, // DX12设备适配器
                    resource,
                    desc
                );
            }
        }
    }
}

void DX12SwapChain::InitializeStats() {
    QueryPerformanceFrequency(&m_frequency);
    m_lastFrameTime.QuadPart = 0;
}

} // namespace PrismaEngine::Graphic::DX12