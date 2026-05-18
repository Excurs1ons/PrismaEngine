#include "RenderSystem.h"
#include "../app/Engine.h"
#include "../platform/Platform.h"
#include "../logger/Logger.h"
#include "../scene/Scene.h"
#include "../transform/Camera.h"
#include "RenderResourceManager.h"
#include "Renderer.h"
#include "Renderer2D.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "adapters/vulkan/VulkanCommandBuffer.h"
#include "pipelines/forward/ForwardPipeline.h"
#include "2d/Pipeline2D.h"

namespace Prisma::Graphic {
RenderSystem::RenderSystem(const RenderSystemDesc& desc) : m_desc(desc) {}

RenderSystem::~RenderSystem() {
    Shutdown();
}

int RenderSystem::Initialize() {
    LOG_INFO("Renderer", "正在初始化渲染系统后端: {0}", (int)m_desc.backendType);
    int dev_init_result = InitializeDevice();
    if (dev_init_result != 0) {
        LOG_ERROR("Renderer", "渲染设备初始化失败！ {0}", dev_init_result);
        return dev_init_result;
    }
    LOG_INFO("Renderer", "渲染设备初始化成功: {0} ({1})", m_device->GetName(), m_device->GetAPIName());

    int res_manager_init_result = InitializeRenderResourceManager();
    if (res_manager_init_result != 0) {
        LOG_ERROR("Renderer", "渲染资源管理器初始化失败！ {0}", res_manager_init_result);
        return res_manager_init_result;
    }
    LOG_DEBUG("Renderer", "渲染资源管理器初始化成功。");

    if (m_desc.renderMode != RenderMode::SRP) {
        int pipeline_init_result = InitializeRenderPipelines();
        if (pipeline_init_result != 0) {
            LOG_ERROR("Renderer", "渲染管线初始化失败！ {0}", pipeline_init_result);
            return pipeline_init_result;
        }
        LOG_DEBUG("Renderer", "管线初始化成功。");
    } else {
        LOG_INFO("Renderer", "SRP 模式：跳过内置渲染管线");
    }

    // 2D 渲染器：SRP 模式不初始化，由 C# 管线完全接管
    if (m_desc.renderMode != RenderMode::SRP) {
        Renderer2D::Initialize();
        LOG_DEBUG("Renderer", "2D 渲染器初始化成功。");
    } else {
        LOG_DEBUG("Renderer", "SRP 模式：跳过 2D 渲染器初始化");
    }

    LOG_INFO("Renderer", "渲染系统初始化成功。");
    return 0;
}

int RenderSystem::InitializeDevice() {
    if (m_desc.backendType == RenderAPIType::Vulkan) {
        LOG_DEBUG("Renderer", "正在创建 Vulkan 渲染设备...");
        m_device = std::make_unique<Vulkan::RenderDeviceVulkan>();

        DeviceDesc devDesc;
        devDesc.name             = m_desc.name;
        devDesc.width            = m_desc.width;
        devDesc.height           = m_desc.height;
        devDesc.presentMode      = m_desc.presentMode;
        devDesc.enableValidation = m_desc.enableValidation;

        return m_device->Initialize(devDesc);
    }
    LOG_ERROR("Renderer", "不支持的渲染 API 类型: {0}", (int)m_desc.backendType);
    return -1;
}

int RenderSystem::InitializeRenderResourceManager() {
    // 使用单例实例并进行初始化，确保全局访问一致性
    m_renderResourceManager = std::dynamic_pointer_cast<RenderResourceManager>(RenderResourceManager::Get());
    if (!m_renderResourceManager) {
        LOG_ERROR("Renderer", "获取全局渲染资源管理器失败！");
        return -1;
    }
    return m_renderResourceManager->Initialize(m_device.get());
}

int RenderSystem::InitializeRenderPipelines() {
    // 根据渲染模式选择管线
    if (m_desc.renderMode == RenderMode::Mode2D) {
        m_mainRenderPipeline = std::make_shared<Pipeline2D>();
    } else {
        m_mainRenderPipeline = std::make_shared<ForwardPipeline>();
    }
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
        LOG_DEBUG("Renderer", "渲染系统无需关闭（无设备）");
        return;
    }

    LOG_DEBUG("Renderer", "正在关闭渲染器...");

    // 关闭 2D 渲染器
    LOG_DEBUG("Renderer", "关闭 2D 渲染器...");
    Renderer2D::Shutdown();
    LOG_DEBUG("Renderer", "2D 渲染器已关闭");

    // 1. 先销毁依赖设备的管线和资源管理器
    if (m_mainRenderPipeline) {
        LOG_DEBUG("Renderer", "关闭主渲染管线...");
        m_mainRenderPipeline->Shutdown();
        m_mainRenderPipeline.reset();
        LOG_DEBUG("Renderer", "主渲染管线已关闭");
    }

    if (m_renderResourceManager) {
        LOG_DEBUG("Renderer", "关闭渲染资源管理器...");
        m_renderResourceManager->Shutdown();
        m_renderResourceManager.reset();
        LOG_DEBUG("Renderer", "渲染资源管理器已关闭");
    }

    // 2. 最后关闭设备并置空，确保此函数是幂等的
    if (m_device) {
        LOG_DEBUG("Renderer", "关闭渲染设备...");
        m_device->Shutdown();
        m_device.reset();
        LOG_DEBUG("Renderer", "渲染设备已关闭");
    }
}

void RenderSystem::BeginFrame() {
    if (m_device)
        m_device->BeginFrame();
}

void RenderSystem::EndFrame() {
    if (m_device && m_mainRenderPipeline) {
        auto& commands = Renderer::GetCommandQueue();
        size_t cmdCount = commands.size();

        // 每 5 秒真实时间记录一次 EndFrame 队列状态
        static double lastLogTime = 0.0;
        double now = Platform::GetTimeSeconds();
        if (now - lastLogTime >= 5.0) {
            // 额外统计：直接从命令缓冲区获取 GPU Draw/Dispatch 次数
            uint32_t gpuCmdCount = 0;
            auto* vkDevice = dynamic_cast<Vulkan::RenderDeviceVulkan*>(m_device.get());
            if (vkDevice) {
                auto* vkCmd = dynamic_cast<Vulkan::VulkanCommandBuffer*>(vkDevice->GetCurrentCommandBuffer());
                if (vkCmd) gpuCmdCount = vkCmd->GetAndResetCommandCount();
            }
            LOG_DEBUG("RenderSystem", "EndFrame: {} 条(渲染器) + {} 条(GPU命令)",
                     cmdCount, gpuCmdCount);
            lastLogTime = now;
        }

        if (!commands.empty()) {
            RenderContext ctx;
            ctx.device = m_device.get();

            auto vkDevice = dynamic_cast<Vulkan::RenderDeviceVulkan*>(m_device.get());
            if (vkDevice) {
                ctx.commandBuffer = reinterpret_cast<ICommandBuffer*>(vkDevice->GetCurrentCommandBuffer());
            }

            const auto& sceneData       = Renderer::GetSceneData();
            ctx.camera.viewMatrix       = sceneData.camera.viewMatrix;
            ctx.camera.projectionMatrix = sceneData.camera.projectionMatrix;
            ctx.camera.position         = sceneData.camera.position;
            ctx.camera.nearPlane        = sceneData.camera.nearPlane;
            ctx.camera.farPlane         = sceneData.camera.farPlane;

            ctx.frameIndex = m_device->GetCurrentFrameIndex();
            ctx.width      = m_desc.width;
            ctx.height     = m_desc.height;
            ctx.deltaTime  = 0.016f;

            m_mainRenderPipeline->Execute(ctx);
            Renderer::ClearQueue();
        }
    }

    if (m_device)
        m_device->EndFrame();
}

void RenderSystem::Present() {
    if (m_device)
        m_device->Present();
}

void RenderSystem::Resize(uint32_t width, uint32_t height) {
    m_desc.width  = width;
    m_desc.height = height;
    if (m_device)
        m_device->Resize(width, height);
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
            LOG_ERROR("RenderSystem", "初始化新的主管线失败: {0}", result);
            m_mainRenderPipeline.reset();
        }
    }
}

void RenderSystem::RenderScene(::Prisma::Scene* scene, ::Prisma::Graphic::ICamera* camera, ITexture* targetTexture) {
    if (!scene || !camera) {
        LOG_WARNING("RenderSystem", "尝试使用空的场景或相机进行渲染");
        return;
    }

    if (m_mainRenderPipeline) {
        // [新增] 开始场景收集
        CameraData cameraData;
        cameraData.viewMatrix       = camera->GetViewMatrix();
        cameraData.projectionMatrix = camera->GetProjectionMatrix();
        cameraData.position         = camera->GetPosition();
        cameraData.nearPlane        = camera->GetNearPlane();
        cameraData.farPlane         = camera->GetFarPlane();

        Renderer::BeginScene(cameraData);

        // [新增] 遍历场景中的对象并提交渲染指令
        // 注意：目前由 Scene 负责 Update 并调用内部组件的渲染提交。
        // 但为了确保 RenderScene 调用时队列里有东西，我们需要确保提交逻辑被触发。
        // 暂时假设上一帧或本帧的 Scene::Update 已经填好了 Renderer::s_Data。
        // 为了保险，我们在这里显式触发一次提交（如果组件支持）。

        Renderer::EndScene();

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

        ctx.targetTexture           = targetTexture;
        ctx.camera.viewMatrix       = camera->GetViewMatrix();
        ctx.camera.projectionMatrix = camera->GetProjectionMatrix();
        ctx.camera.position         = camera->GetPosition();
        ctx.camera.nearPlane        = camera->GetNearPlane();
        ctx.camera.farPlane         = camera->GetFarPlane();
        ctx.frameIndex              = m_device ? m_device->GetCurrentFrameIndex() : 0;
        ctx.width                   = m_desc.width;
        ctx.height                  = m_desc.height;
        ctx.lights.clear();

        m_mainRenderPipeline->Execute(ctx);
    } else {
        LOG_ERROR("RenderSystem", "没有处于活动状态的管线来进行场景渲染");
    }
}

}  // namespace Prisma::Graphic
