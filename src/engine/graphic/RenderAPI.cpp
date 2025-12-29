#include "RenderAPI.h"
#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/IResourceFactory.h"
#include "interfaces/ISwapChain.h"
#include <cstring>

namespace PrismaEngine::Graphic {

RenderAPI::RenderAPI() = default;

RenderAPI::~RenderAPI() {
    Shutdown();
}

// === IRenderDevice 接口实现 ===

bool RenderAPI::Initialize(const DeviceDesc& desc) {
    (void)desc;
    return false;
}

void RenderAPI::Shutdown() {
    m_resourceFactory = nullptr;
}

std::string RenderAPI::GetName() const {
    return "RenderAPI";
}

std::string RenderAPI::GetAPIName() const {
    switch (m_backendType) {
        case RenderAPIType::DirectX12: return "DirectX12";
        case RenderAPIType::Vulkan: return "Vulkan";
        case RenderAPIType::OpenGL: return "OpenGL";
        default: return "Unknown";
    }
}

std::unique_ptr<ICommandBuffer> RenderAPI::CreateCommandBuffer(CommandBufferType type) {
    (void)type;
    return nullptr;
}

void RenderAPI::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {
    (void)cmdBuffer; (void)fence;
}

void RenderAPI::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                                        const std::vector<IFence*>& fences) {
    (void)cmdBuffers; (void)fences;
}

void RenderAPI::WaitForIdle() {
}

std::unique_ptr<IFence> RenderAPI::CreateFence() {
    return nullptr;
}

void RenderAPI::WaitForFence(IFence* fence) {
    (void)fence;
}

IResourceFactory* RenderAPI::GetResourceFactory() const {
    return m_resourceFactory;
}

std::unique_ptr<ISwapChain> RenderAPI::CreateSwapChain(void* windowHandle,
                                                            uint32_t width,
                                                            uint32_t height,
                                                            bool vsync) {
    (void)windowHandle; (void)width; (void)height; (void)vsync;
    return nullptr;
}

ISwapChain* RenderAPI::GetSwapChain() const {
    return nullptr;
}

void RenderAPI::BeginFrame() {
}

void RenderAPI::EndFrame() {
}

void RenderAPI::Present() {
}

bool RenderAPI::SupportsMultiThreaded() const {
    return HasFeature(m_supportedFeatures, RendererFeature::MultiThreaded);
}

bool RenderAPI::SupportsBindlessTextures() const {
    return HasFeature(m_supportedFeatures, RendererFeature::BindlessTextures);
}

bool RenderAPI::SupportsComputeShader() const {
    return HasFeature(m_supportedFeatures, RendererFeature::ComputeShader);
}

bool RenderAPI::SupportsRayTracing() const {
    return HasFeature(m_supportedFeatures, RendererFeature::RayTracing);
}

bool RenderAPI::SupportsMeshShader() const {
    return false;  // Not in RendererFeature enum
}

bool RenderAPI::SupportsVariableRateShading() const {
    return false;  // Not in RendererFeature enum
}

IRenderDevice::GPUMemoryInfo RenderAPI::GetGPUMemoryInfo() const {
    return {};
}

IRenderDevice::RenderStats RenderAPI::GetRenderStats() const {
    return {};
}

void RenderAPI::BeginDebugMarker(const std::string& name) {
    (void)name;
}

void RenderAPI::EndDebugMarker() {
}

void RenderAPI::SetDebugMarker(const std::string& name) {
    (void)name;
}

// === 旧 API（待废弃） ===

bool RenderAPI::InitializeLegacy(Platform* platform, WindowHandle windowHandle,
                                     void* surface, uint32_t width, uint32_t height) {
    (void)platform; (void)windowHandle; (void)surface; (void)width; (void)height;
    return false;
}

void RenderAPI::BeginFrameLegacy(PrismaMath::vec4 clearColor) {
    (void)clearColor;
    BeginFrame();
}

void RenderAPI::Resize(uint32_t width, uint32_t height) {
    (void)width; (void)height;
}

void RenderAPI::SubmitRenderCommand(const RenderCommand& cmd) {
    (void)cmd;
}

bool RenderAPI::Supports(RendererFeature feature) const {
    return HasFeature(m_supportedFeatures, feature);
}

IDeviceContext* RenderAPI::CreateCommandContext() {
    return nullptr;
}

void* RenderAPI::GetDefaultRenderTarget() {
    return nullptr;  // TODO: 返回有效的渲染目标句柄
}

void* RenderAPI::GetDefaultDepthBuffer() {
    return nullptr;  // TODO: 返回有效的深度缓冲句柄
}

void RenderAPI::GetRenderTargetSize(uint32_t& width, uint32_t& height) {
    width = 0;
    height = 0;
}

void RenderAPI::SetGuiRenderCallback(GuiRenderCallback callback) {
    (void)callback;
}

} // namespace PrismaEngine::Graphic
