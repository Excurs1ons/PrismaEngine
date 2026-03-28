#include "RenderSystem.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "pipelines/forward/ForwardPipeline.h"
#include "Logger.h"
#include "../Scene.h"
#include "../Camera.h"
#include "../Engine.h"
#include "RenderResourceManager.h"

namespace Prisma::Graphic {
RenderSystem::RenderSystem(const RenderSystemDesc& desc)
    : m_desc(desc) {}

RenderSystem::~RenderSystem() {
    Shutdown();
}

int RenderSystem::Initialize() {
    LOG_INFO("Renderer", "Initializing RenderSystem backend: {0}", (int)m_desc.backendType);
    int dev_init_result = InitializeDevice();
    if (dev_init_result != 0) {
        LOG_ERROR("Renderer", "RenderDevice initializing failed! {0}", dev_init_result);
        return dev_init_result;
    }
    LOG_INFO("Renderer", "RenderDevice initialized successfully: {0} ({1})", m_device->GetName(), m_device->GetAPIName());

    int res_manager_init_result = InitializeRenderResourceManager();
    if (res_manager_init_result != 0) {
        LOG_ERROR("Renderer", "Render Resource Manager initializing failed! {0}", res_manager_init_result);
        return res_manager_init_result;
    }
    LOG_INFO("Renderer", "Render Resource Manager initialized successfully.");

    int pipeline_init_result = InitializeRenderPipelines();
    if (pipeline_init_result != 0) {
        LOG_ERROR("Renderer", "Render Pipelines initializing failed! {0}", pipeline_init_result);
        return pipeline_init_result;
    }
    LOG_INFO("Renderer", "Pipelines initialized successfully.");

    LOG_INFO("Renderer", "RenderSystem initialized successfully.");
    return 0;
}

int RenderSystem::InitializeDevice() {
    if (m_desc.backendType == RenderAPIType::Vulkan) {
        LOG_INFO("Renderer", "Creating Vulkan RenderDevice...");
        m_device = std::make_unique<Vulkan::RenderDeviceVulkan>();

        DeviceDesc devDesc;
        devDesc.name = m_desc.name;
        devDesc.width = m_desc.width;
        devDesc.height = m_desc.height;
        devDesc.vsync = m_desc.enableVSync;
        devDesc.enableValidation = true; // For debug
        
        return m_device->Initialize(devDesc); 
    }
    LOG_ERROR("Renderer", "Unsupported RenderAPIType: {0}", (int)m_desc.backendType);
    return -1;
}

int RenderSystem::InitializeRenderResourceManager() {
    // 资源管理器需要设备指针
    m_renderResourceManager = std::make_unique<RenderResourceManager>();
    return m_renderResourceManager->Initialize(m_device.get());
}

int RenderSystem::InitializeRenderPipelines() {
    // 默认创建前向渲染管线
    m_mainRenderPipeline = std::make_shared<ForwardPipeline>();
    return m_mainRenderPipeline->Initialize(m_device.get());
}

void RenderSystem::Update(Timestep ts) {
    if (!m_device) {
        return;
    }

    if (m_renderResourceManager) {
        m_renderResourceManager->Update(ts);
        m_renderResourceManager->GarbageCollect();
    }
}

void RenderSystem::Shutdown() {
    if (!m_device) {
        return;
    }

    LOG_INFO("Renderer", "Shutting down renderer...");
    
    // 1. 先销毁依赖设备的管线和资源管理器
    if (m_mainRenderPipeline) {
        m_mainRenderPipeline->Shutdown();
        m_mainRenderPipeline.reset();
    }

    if (m_renderResourceManager) {
        m_renderResourceManager->Shutdown();
        m_renderResourceManager.reset();
    }

    // 2. 最后关闭设备并置空，确保此函数是幂等的
    if (m_device) {
        m_device->Shutdown();
        m_device.reset();
    }
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

void RenderSystem::SetMainPipeline(std::shared_ptr<IPipeline> pipeline) {
    if (m_mainRenderPipeline == pipeline) {
        return;
    }

    if (m_mainRenderPipeline) {
        m_mainRenderPipeline->Shutdown();
    }

    m_mainRenderPipeline = std::move(pipeline);

    if (m_mainRenderPipeline && m_device) {
        const int result = m_mainRenderPipeline->Initialize(m_device.get());
        if (result != 0) {
            LOG_ERROR("RenderSystem", "Failed to initialize new main pipeline: {0}", result);
            m_mainRenderPipeline.reset();
        }
    }
}

void RenderSystem::RenderScene(::Prisma::Scene* scene, ::Prisma::Graphic::ICamera* camera, ITexture* targetTexture) {
    if (!scene || !camera) {
        LOG_WARNING("RenderSystem", "Attempting to render with null scene or camera");
        return;
    }

    if (m_mainRenderPipeline) {
        // 构建 RenderContext
        RenderContext ctx;
        ctx.device = m_device.get();
        
        // [修复] 获取当前的指令缓冲
        auto vkDevice = dynamic_cast<Vulkan::RenderDeviceVulkan*>(m_device.get());
        if (vkDevice) {
            ctx.commandBuffer = reinterpret_cast<ICommandBuffer*>(vkDevice->GetCurrentCommandBuffer());
        } else {
            ctx.commandBuffer = nullptr;
        }

        ctx.targetTexture = targetTexture;
        ctx.camera.viewMatrix = camera->GetViewMatrix();
        ctx.camera.projectionMatrix = camera->GetProjectionMatrix();
        ctx.camera.position = camera->GetPosition();
        ctx.camera.nearPlane = camera->GetNearPlane();
        ctx.camera.farPlane = camera->GetFarPlane();
        ctx.frameIndex = m_device ? m_device->GetCurrentFrameIndex() : 0;
        ctx.width = m_desc.width;
        ctx.height = m_desc.height;
        ctx.lights.clear();

        m_mainRenderPipeline->Execute(ctx);
    } else {
        LOG_ERROR("RenderSystem", "No active pipeline to render the scene");
    }
}

} // namespace Prisma::Graphic
