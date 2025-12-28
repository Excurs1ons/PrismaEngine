#include "RenderBackend.h"
#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/IResourceFactory.h"
#include "interfaces/ISwapChain.h"
#include <cstring>

namespace PrismaEngine::Graphic {

RenderBackend::RenderBackend() = default;

RenderBackend::~RenderBackend() {
    Shutdown();
}

// === IRenderDevice 接口实现 ===

bool RenderBackend::Initialize(const DeviceDesc& desc) {
    (void)desc;
    return false;
}

void RenderBackend::Shutdown() {
    m_resourceFactory = nullptr;
}

std::string RenderBackend::GetName() const {
    return "RenderBackend";
}

std::string RenderBackend::GetAPIName() const {
    switch (m_backendType) {
        case RenderBackendType::DirectX12: return "DirectX12";
        case RenderBackendType::Vulkan: return "Vulkan";
        case RenderBackendType::SDL3: return "SDL3";
        default: return "Unknown";
    }
}

std::unique_ptr<ICommandBuffer> RenderBackend::CreateCommandBuffer(CommandBufferType type) {
    (void)type;
    return nullptr;
}

void RenderBackend::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {
    (void)cmdBuffer; (void)fence;
}

void RenderBackend::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                                        const std::vector<IFence*>& fences) {
    (void)cmdBuffers; (void)fences;
}

void RenderBackend::WaitForIdle() {
}

std::unique_ptr<IFence> RenderBackend::CreateFence() {
    return nullptr;
}

void RenderBackend::WaitForFence(IFence* fence) {
    (void)fence;
}

IResourceFactory* RenderBackend::GetResourceFactory() const {
    return m_resourceFactory;
}

std::unique_ptr<ISwapChain> RenderBackend::CreateSwapChain(void* windowHandle,
                                                            uint32_t width,
                                                            uint32_t height,
                                                            bool vsync) {
    (void)windowHandle; (void)width; (void)height; (void)vsync;
    return nullptr;
}

ISwapChain* RenderBackend::GetSwapChain() const {
    return nullptr;
}

void RenderBackend::BeginFrame() {
}

void RenderBackend::EndFrame() {
}

void RenderBackend::Present() {
}

bool RenderBackend::SupportsMultiThreaded() const {
    return HasFeature(m_supportedFeatures, RendererFeature::MultiThreaded);
}

bool RenderBackend::SupportsBindlessTextures() const {
    return HasFeature(m_supportedFeatures, RendererFeature::BindlessTextures);
}

bool RenderBackend::SupportsComputeShader() const {
    return HasFeature(m_supportedFeatures, RendererFeature::ComputeShader);
}

bool RenderBackend::SupportsRayTracing() const {
    return HasFeature(m_supportedFeatures, RendererFeature::RayTracing);
}

bool RenderBackend::SupportsMeshShader() const {
    return false;  // Not in RendererFeature enum
}

bool RenderBackend::SupportsVariableRateShading() const {
    return false;  // Not in RendererFeature enum
}

IRenderDevice::GPUMemoryInfo RenderBackend::GetGPUMemoryInfo() const {
    return {};
}

IRenderDevice::RenderStats RenderBackend::GetRenderStats() const {
    return {};
}

void RenderBackend::BeginDebugMarker(const std::string& name) {
    (void)name;
}

void RenderBackend::EndDebugMarker() {
}

void RenderBackend::SetDebugMarker(const std::string& name) {
    (void)name;
}

// === 旧 API（待废弃） ===

bool RenderBackend::InitializeLegacy(Platform* platform, WindowHandle windowHandle,
                                     void* surface, uint32_t width, uint32_t height) {
    (void)platform; (void)windowHandle; (void)surface; (void)width; (void)height;
    return false;
}

void RenderBackend::BeginFrameLegacy(PrismaMath::vec4 clearColor) {
    (void)clearColor;
    BeginFrame();
}

void RenderBackend::Resize(uint32_t width, uint32_t height) {
    (void)width; (void)height;
}

void RenderBackend::SubmitRenderCommand(const RenderCommand& cmd) {
    (void)cmd;
}

bool RenderBackend::Supports(RendererFeature feature) const {
    return HasFeature(m_supportedFeatures, feature);
}

IDeviceContext* RenderBackend::CreateCommandContext() {
    return nullptr;
}

void* RenderBackend::GetDefaultRenderTarget() {
    return m_nativeRenderTarget;
}

void* RenderBackend::GetDefaultDepthBuffer() {
    return m_nativeDepthStencil;
}

void RenderBackend::GetRenderTargetSize(uint32_t& width, uint32_t& height) {
    width = 0;
    height = 0;
}

void RenderBackend::SetGuiRenderCallback(GuiRenderCallback callback) {
    (void)callback;
}

} // namespace PrismaEngine::Graphic
