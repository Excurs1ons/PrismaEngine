#pragma once

#include "interfaces/ISwapChain.h"
#include <vulkan/vulkan.h>

namespace PrismaEngine::Graphic::Vulkan {

class RenderDeviceVulkan;

class VulkanSwapChain : public ISwapChain {
public:
    VulkanSwapChain(RenderDeviceVulkan* device) : m_device(device) {}
    ~VulkanSwapChain() override = default;

    uint32_t GetBufferCount() const override { return 3; }
    uint32_t GetCurrentBufferIndex() const override { return 0; }
    uint32_t GetWidth() const override { return 1600; }
    uint32_t GetHeight() const override { return 900; }
    TextureFormat GetFormat() const override { return TextureFormat::RGBA8_UNorm; }
    SwapChainMode GetMode() const override { return SwapChainMode::VSync; }
    bool IsHDR() const override { return false; }

    ITexture* GetRenderTarget(uint32_t /*bufferIndex*/ = 0) override { return nullptr; }
    ITexture* GetCurrentRenderTarget() override { return nullptr; }

    bool Present() override { return true; }
    bool SetMode(SwapChainMode /*mode*/) override { return true; }
    bool Resize(uint32_t /*width*/, uint32_t /*height*/) override { return true; }
    bool SetHDR(bool /*enable*/) override { return true; }

    const char* GetColorSpace() const override { return "sRGB"; }
    bool SetColorSpace(const char* /*colorSpace*/) override { return true; }

    float GetFrameRate() const override { return 60.0f; }
    float GetFrameTime() const override { return 16.6f; }
    PresentStats GetPresentStats() const override { return {}; }
    void ResetStats() override {}

    bool IsFullscreen() const override { return false; }
    bool SetFullscreen(bool /*fullscreen*/) override { return true; }

    bool Screenshot(const std::string& /*filename*/, uint32_t /*bufferIndex*/ = 0) override { return true; }
    void EnableDebugLayer(bool /*enable*/) override {}

private:
    RenderDeviceVulkan* m_device;
};

} // namespace PrismaEngine::Graphic::Vulkan