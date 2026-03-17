#include "GBuffer.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "Logger.h"

namespace Prisma::Graphic {

const uint32_t GBufferFormats::POSITION_FORMAT = 0;
const uint32_t GBufferFormats::NORMAL_FORMAT = 0;
const uint32_t GBufferFormats::ALBEDO_FORMAT = 0;
const uint32_t GBufferFormats::EMISSIVE_FORMAT = 0;
const uint32_t GBufferFormats::DEPTH_FORMAT = 0;

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

void GBuffer::SetAsRenderTarget(IDeviceContext* deviceContext) {}
void GBuffer::BindAsShaderResources(IDeviceContext* deviceContext, uint32_t startSlot) {}
void GBuffer::UnbindShaderResources(IDeviceContext* deviceContext, uint32_t startSlot, uint32_t count) {}
void GBuffer::Clear(IDeviceContext* deviceContext, const float color[4]) {}
void GBuffer::ClearDepth(IDeviceContext* deviceContext, float depth) {}

bool GBuffer::InitializeVulkanResources(uint32_t width, uint32_t height) {
    // Basic implementation for now
    m_created = true;
    return true;
}

void GBuffer::DestroyVulkanResources() {
    if (!m_created) return;
    
    // Cleanup logic
    m_created = false;
}

} // namespace Prisma::Graphic
