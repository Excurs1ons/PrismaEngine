#include "RenderSystem.h"
#include "RenderAPIVulkan.h"
#include "pipelines/ForwardPipeline.h"
#include "Logger.h"

namespace Prisma::Graphic {

RenderSystem::RenderSystem(const RenderSystemDesc& desc)
    : m_desc(desc) {}

RenderSystem::~RenderSystem() {
    Shutdown();
}

int RenderSystem::Initialize() {
    LOG_INFO("Renderer", "Initializing RenderSystem backend: {0}", (int)m_desc.backendType);
    
    if (!InitializeDevice()) return -1;
    if (!InitializeResourceManager()) return -1;
    if (!InitializePipelines()) return -1;

    LOG_INFO("Renderer", "RenderSystem initialized successfully.");
    return 0;
}

bool RenderSystem::InitializeDevice() {
    if (m_desc.backendType == RenderAPIType::Vulkan) {
        m_device = std::make_unique<RenderAPIVulkan>();
        // 这里需要把 m_desc 传给底层驱动，
        // 实际代码中可能需要调整接口来接收 desc。
        // 但现在我们先聚焦在 Engine 集成上。
        return m_device->Initialize(m_desc); 
    }
    return false;
}

bool RenderSystem::InitializeResourceManager() {
    // 资源管理器需要设备指针
    // m_resourceManager = std::make_unique<RenderResourceManager>(m_device.get());
    return true;
}

bool RenderSystem::InitializePipelines() {
    // 默认创建前向渲染管线
    // m_mainPipeline = std::make_shared<ForwardPipeline>(m_device.get());
    return true;
}

void RenderSystem::Update(Timestep ts) {
    // 渲染循环逻辑
}

void RenderSystem::Shutdown() {
    LOG_INFO("Renderer", "Shutting down renderer...");
    if (m_mainPipeline) m_mainPipeline.reset();
    if (m_resourceManager) m_resourceManager.reset();
    if (m_device) m_device->Shutdown();
}

void RenderSystem::BeginFrame() {
    if (m_device) m_device->BeginFrame();
}

void RenderSystem::EndFrame() {
    if (m_device) m_device->EndFrame();
}

void RenderSystem::Present() {
    if (m_device) m_device->Present();
}

void RenderSystem::Resize(uint32_t width, uint32_t height) {
    m_desc.width = width;
    m_desc.height = height;
    if (m_device) m_device->Resize(width, height);
}

} // namespace Prisma::Graphic
