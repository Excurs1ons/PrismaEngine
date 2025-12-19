#pragma once

#include "DX12RenderDevice.h"
#include "interfaces/ISwapChain.h"
#include <memory>

namespace PrismaEngine::Graphic::DX12 {

/// @brief DirectX12交换链适配器
/// 实现ISwapChain接口，包装现有的交换链实现
class DX12SwapChain : public ISwapChain {
public:
    /// @brief 构造函数
    DX12SwapChain(DX12RenderDevice* device);
    /// @brief 析构函数
    ~DX12SwapChain() override;

    // ISwapChain接口实现
    uint32_t GetBufferCount() const override;
    uint32_t GetCurrentBufferIndex() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    TextureFormat GetFormat() const override;
    SwapChainMode GetMode() const override;
    bool IsHDR() const override;

    ITexture* GetRenderTarget(uint32_t bufferIndex) override;
    ITexture* GetCurrentRenderTarget() override;

    bool Present() override;
    bool SetMode(SwapChainMode mode) override;
    bool Resize(uint32_t width, uint32_t height) override;
    bool SetHDR(bool enable) override;

    const char* GetColorSpace() const override;
    bool SetColorSpace(const char* colorSpace) override;

    float GetFrameRate() const override;
    float GetFrameTime() const override;
    PresentStats GetPresentStats() const override;
    void ResetStats() override;

    bool IsFullscreen() const override;
    bool SetFullscreen(bool fullscreen) override;

    bool Screenshot(const std::string& filename, uint32_t bufferIndex) override;
    void EnableDebugLayer(bool enable) override;

    // === DirectX12特定方法 ===

    /// @brief 克隆交换链
    /// @return 新的交换链实例
    std::unique_ptr<DX12SwapChain> Clone() const;

    /// @brief 获取DXGI交换链
    /// @return DXGI交换链指针
    IDXGISwapChain3* GetDXGISwapChain() const;

private:
    DX12RenderDevice* m_device;
    SwapChainMode m_mode = SwapChainMode::VSync;
    bool m_hdr = false;
    std::string m_colorSpace = "sRGB";

    // 统计信息
    mutable PresentStats m_stats = {};
    mutable LARGE_INTEGER m_lastFrameTime = {};
    mutable LARGE_INTEGER m_frequency = {};

    // 渲染目标适配器
    std::vector<std::unique_ptr<ITexture>> m_renderTargets;

    // 辅助方法
    void UpdateStats() const;
    void CreateRenderTargetAdapters();
    void InitializeStats();
};

} // namespace PrismaEngine::Graphic::DX12