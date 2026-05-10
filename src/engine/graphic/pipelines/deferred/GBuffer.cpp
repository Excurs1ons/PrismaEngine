#include "GBuffer.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "Logger.h"

namespace Prisma::Graphic {

class GBuffer::RenderTargetProxy final : public ITextureRenderTarget {
public:
    RenderTargetProxy(uint32_t width, uint32_t height, TextureFormat format, VulkanResource* resource)
        : m_width(width), m_height(height), m_format(format), m_resource(resource) {}

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    TextureFormat GetFormat() const override { return m_format; }
    TextureType GetType() const override { return TextureType::Texture2D; }
    void* GetNativeHandle() const override {
        return m_resource != nullptr ? reinterpret_cast<void*>(m_resource->imageView) : nullptr;
    }
    bool IsSwapChain() const override { return false; }
    void Clear(const float color[4]) override {
        if (!color) {
            return;
        }
        m_clearColor = {color[0], color[1], color[2], color[3]};
    }
    uint32_t GetMipLevels() const override { return 1; }
    uint32_t GetArraySize() const override { return 1; }
    ITexture* GetTexture() override { return nullptr; }

    void Resize(uint32_t width, uint32_t height) {
        m_width = width;
        m_height = height;
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    TextureFormat m_format = TextureFormat::Unknown;
    VulkanResource* m_resource = nullptr;
    std::array<float, 4> m_clearColor{0.0f, 0.0f, 0.0f, 0.0f};
};

class GBuffer::DepthStencilProxy final : public IDepthStencil {
public:
    DepthStencilProxy(uint32_t width, uint32_t height, TextureFormat format, VulkanResource* resource)
        : m_width(width), m_height(height), m_format(format), m_resource(resource) {}

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    TextureFormat GetFormat() const override { return m_format; }
    void* GetNativeHandle() const override {
        return m_resource != nullptr ? reinterpret_cast<void*>(m_resource->imageView) : nullptr;
    }
    void ClearDepth(float depth) override { m_depth = depth; }
    void ClearStencil(uint8_t stencil) override { m_stencil = stencil; }
    void Clear(float depth, uint8_t stencil) override {
        m_depth = depth;
        m_stencil = stencil;
    }

    void Resize(uint32_t width, uint32_t height) {
        m_width = width;
        m_height = height;
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    TextureFormat m_format = TextureFormat::Unknown;
    VulkanResource* m_resource = nullptr;
    float m_depth = 1.0f;
    uint8_t m_stencil = 0;
};

const uint32_t GBufferFormats::POSITION_FORMAT = static_cast<uint32_t>(TextureFormat::RGBA16_Float);
const uint32_t GBufferFormats::NORMAL_FORMAT = static_cast<uint32_t>(TextureFormat::RGBA16_Float);
const uint32_t GBufferFormats::ALBEDO_FORMAT = static_cast<uint32_t>(TextureFormat::RGBA8_UNorm);
const uint32_t GBufferFormats::EMISSIVE_FORMAT = static_cast<uint32_t>(TextureFormat::RGBA16_Float);
const uint32_t GBufferFormats::DEPTH_FORMAT = static_cast<uint32_t>(TextureFormat::D32_Float);

GBuffer::GBuffer() = default;

GBuffer::~GBuffer() {
    DestroyVulkanResources();
}

bool GBuffer::Initialize(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    return InitializeVulkanResources(width, height);
}

bool GBuffer::Resize(uint32_t width, uint32_t height) {
    DestroyVulkanResources();
    return Initialize(width, height);
}

void GBuffer::SetAsRenderTarget(IDeviceContext* deviceContext) {
    if (!deviceContext || !m_created) {
        return;
    }

    IRenderTarget* colorTargets[4] = {
        m_colorTargetViews[0].get(),
        m_colorTargetViews[1].get(),
        m_colorTargetViews[2].get(),
        m_colorTargetViews[3].get()
    };
    deviceContext->SetRenderTargets(colorTargets, 4, m_depthStencilView.get());
}
void GBuffer::BindAsShaderResources(IDeviceContext* deviceContext, uint32_t startSlot) {
    if (!deviceContext || !m_created) {
        return;
    }

    for (uint32_t i = 0; i < 4; ++i) {
        deviceContext->SetTexture(m_colorTargetViews[i] ? m_colorTargetViews[i]->GetTexture() : nullptr, startSlot + i);
    }
}
void GBuffer::UnbindShaderResources(IDeviceContext* deviceContext, uint32_t startSlot, uint32_t count) {
    if (!deviceContext) {
        return;
    }

    for (uint32_t i = 0; i < count; ++i) {
        deviceContext->SetTexture(nullptr, startSlot + i);
    }
}
void GBuffer::Clear(IDeviceContext* deviceContext, const float color[4]) {
    if (!deviceContext || !m_created) {
        return;
    }

    for (const auto& colorTarget : m_colorTargetViews) {
        if (colorTarget) {
            deviceContext->ClearRenderTarget(colorTarget.get(), color);
        }
    }
}
void GBuffer::ClearDepth(IDeviceContext* deviceContext, float depth) {
    if (!deviceContext || !m_depthStencilView) {
        return;
    }

    deviceContext->ClearDepthStencil(m_depthStencilView.get(), depth, 0);
}

bool GBuffer::InitializeVulkanResources(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    m_colorTargetViews[0] = std::make_unique<RenderTargetProxy>(width, height, TextureFormat::RGBA16_Float, &m_renderTargets[0]);
    m_colorTargetViews[1] = std::make_unique<RenderTargetProxy>(width, height, TextureFormat::RGBA16_Float, &m_renderTargets[1]);
    m_colorTargetViews[2] = std::make_unique<RenderTargetProxy>(width, height, TextureFormat::RGBA8_UNorm, &m_renderTargets[2]);
    m_colorTargetViews[3] = std::make_unique<RenderTargetProxy>(width, height, TextureFormat::RGBA16_Float, &m_renderTargets[3]);
    m_depthStencilView = std::make_unique<DepthStencilProxy>(width, height, TextureFormat::D32_Float, &m_depthBuffer);
    m_created = true;
    return true;
}

void GBuffer::DestroyVulkanResources() {
    if (!m_created) return;

    for (auto& colorTarget : m_colorTargetViews) {
        colorTarget.reset();
    }
    m_depthStencilView.reset();
    m_created = false;
}

ITextureRenderTarget* GBuffer::GetTarget(GBufferTarget target) {
    const uint32_t index = static_cast<uint32_t>(target);
    if (index >= m_colorTargetViews.size()) {
        return nullptr;
    }
    return m_colorTargetViews[index].get();
}

IDepthStencil* GBuffer::GetDepthStencil() {
    return m_depthStencilView.get();
}

void GBuffer::GetColorTargets(ITextureRenderTarget** targets, uint32_t count) {
    if (!targets) {
        return;
    }

    const uint32_t maxTargets = std::min<uint32_t>(count, static_cast<uint32_t>(m_colorTargetViews.size()));
    for (uint32_t i = 0; i < maxTargets; ++i) {
        targets[i] = m_colorTargetViews[i].get();
    }
}

TextureFormat GBuffer::GetTargetFormat(GBufferTarget target) const {
    switch (target) {
        case GBufferTarget::Position:
            return TextureFormat::RGBA16_Float;
        case GBufferTarget::Normal:
            return TextureFormat::RGBA16_Float;
        case GBufferTarget::Albedo:
            return TextureFormat::RGBA8_UNorm;
        case GBufferTarget::Emissive:
            return TextureFormat::RGBA16_Float;
        case GBufferTarget::Depth:
            return TextureFormat::D32_Float;
        case GBufferTarget::Count:
            break;
    }

    return TextureFormat::Unknown;
}

} // namespace Prisma::Graphic
