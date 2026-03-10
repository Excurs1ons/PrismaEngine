#include "RenderSystem.h"
#include "../Camera.h"
#include "../Logger.h"
#include "../SceneManager.h"
#include "../core/ECS.h"
#include "pipelines/forward/ForwardPipeline.h"

// ImGui
#ifdef PRISMA_BUILD_EDITOR
#include <imgui.h>
#endif

#ifdef PRISMA_ENABLE_RENDER_DX12
#include "adapters/dx12/DX12Adapters.h"
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#endif

#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include "adapters/vulkan/VulkanAdapters.h"
#ifndef IMGUI_IMPL_VULKAN
#define IMGUI_IMPL_VULKAN
#endif
#include <imgui_impl_vulkan.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

namespace PrismaEngine::Graphic {

std::shared_ptr<RenderSystem> RenderSystem::GetInstance() {
    static std::shared_ptr<RenderSystem> instance = std::make_shared<RenderSystem>();
    return instance;
}

int RenderSystem::Initialize() {
    RenderSystemDesc defaultDesc;
    return Initialize(defaultDesc);
}

int RenderSystem::Initialize(const RenderSystemDesc& desc) {
    LOG_INFO("Render",
             "正在初始化渲染系统 (Backend: {0})...",
             desc.backendType == RenderAPIType::Vulkan ? "Vulkan" : "DirectX12");
    m_desc = desc;
    Logger::GetInstance().Flush();

    // 1. 初始化设备
    if (!InitializeDevice(desc)) {
        LOG_ERROR("Render", "渲染设备初始化失败！请检查驱动程序或 SDK 是否正确安装。");
        Logger::GetInstance().Flush();
        return false;
    }

    // 2. 初始化资源管理器
    if (!InitializeResourceManager()) {
        LOG_ERROR("Render", "资源管理器初始化失败。");
        Logger::GetInstance().Flush();
        return false;
    }

    // 3. 初始化管线
    if (!InitializePipelines()) {
        LOG_ERROR("Render", "渲染管线初始化失败。");
        Logger::GetInstance().Flush();
        return false;
    }

    // 4. 启动渲染线程
    m_renderThread.Start();

    LOG_INFO("Render", "渲染系统初始化成功。");
    Logger::GetInstance().Flush();
    return true;
}

void RenderSystem::Shutdown() {
    LOG_INFO("Render", "渲染系统正在关闭...");

    if (m_renderThread.IsRunning()) {
        m_renderThread.Stop();
        m_renderThread.Join();
    }

    ShutdownImGui();

    m_mainPipeline.reset();
    m_forwardPipeline.reset();
    m_resourceManager.reset();

    if (m_device) {
        m_device->Shutdown();
        m_device.reset();
    }

    LOG_INFO("RenderSystem", "渲染系统已关闭");
}

bool RenderSystem::InitializeImGui() {

    if (!m_device) {
        LOG_ERROR("Render", "Device not initialized, cannot init ImGui.");
        return false;
    }

    // ImGui 上下文已经在 Editor 中创建，这里不再重复创建

#if defined(PRISMA_ENABLE_RENDER_DX12)
    if (m_desc.backendType == RenderAPIType::DirectX12) {
        LOG_INFO("Render", "正在初始化ImGui(DX12)");

        ImGui_ImplDX12_InitInfo init_info = {};
        // TODO: 填充 init_info
        if (!ImGui_ImplDX12_Init(&init_info))
            return false;
        if (!m_device->InitializeImGui())
            return false;
        m_imguiInitialized = true;
    }
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    if (m_desc.backendType == RenderAPIType::Vulkan) {
        LOG_INFO("Render", "正在初始化 ImGui (Vulkan)...");
        auto instancePtr = static_cast<Vulkan::RenderDeviceVulkan*>(m_device.get());

        // 先初始化渲染器端的 ImGui（创建描述符池等）
        if (!m_device->InitializeImGui()) {
            LOG_ERROR("Render", "Device ImGui 初始化失败");
            return false;
        }

        // 然后初始化 Vulkan 后端
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion                = VK_API_VERSION_1_3;
        init_info.Instance                  = instancePtr->GetInstance();
        init_info.PhysicalDevice            = instancePtr->GetPhysicalDevice();
        init_info.Device                    = instancePtr->GetDevice();
        init_info.QueueFamily               = instancePtr->GetGraphicsQueueFamily();
        init_info.Queue                     = instancePtr->GetGraphicsQueue();
        init_info.MinImageCount             = 3;
        init_info.ImageCount                = 3;
        init_info.DescriptorPool            = instancePtr->GetImGuiDescriptorPool();
        init_info.DescriptorPoolSize        = 0;  // 使用外部描述符池

        // 设置渲染通道信息（新API使用 PipelineInfoMain）
        init_info.PipelineInfoMain.Subpass    = 0;
        init_info.PipelineInfoMain.RenderPass = instancePtr->GetImGuiRenderPass();

        if (init_info.PipelineInfoMain.RenderPass == VK_NULL_HANDLE) {
            LOG_ERROR("Render", "ImGui Vulkan 初始化失败：RenderPass 为空");
            return false;
        }

        if (!ImGui_ImplVulkan_Init(&init_info)) {
            LOG_ERROR("Render", "ImGui_ImplVulkan_Init 失败");
            return false;
        }

        m_imguiInitialized = true;
        LOG_DEBUG("Render", "ImGui (Vulkan) 初始化完成");
    }
#endif

    if (m_imguiInitialized) {
        LOG_INFO("Render", "ImGui初始化完成");
        return true;
    }
    LOG_FATAL("Render", "ImGui初始化失败");
    return false;
}

void RenderSystem::ShutdownImGui() {

    if (!m_imguiInitialized)
        return;

#if defined(PRISMA_ENABLE_RENDER_DX12)
    if (m_desc.backendType == RenderAPIType::DirectX12) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
    }
#endif
#if defined(PRISMA_ENABLE_RENDER_VULKAN) && !defined(_WIN32) && !defined(__ANDROID__)
    if (m_desc.backendType == RenderAPIType::Vulkan) {
        ImGui_ImplVulkan_Shutdown();
    }
#endif
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }
    m_imguiInitialized = false;
}

RenderSystem::~RenderSystem() {}

void RenderSystem::Update(float deltaTime) {
    UpdateStats(deltaTime);

    if (!m_renderThread.IsRunning()) {
        RenderFrame();
    }
}

void RenderSystem::BeginFrame() {
    if (m_device)
        m_device->BeginFrame();
}

void RenderSystem::EndFrame() {
    if (m_device) {
        if (m_guiCallback) {
            m_guiCallback(m_device.get());
        }
        m_device->EndFrame();
    }
}

void RenderSystem::Present() {
    if (m_device)
        m_device->Present();
    m_stats.frameCount++;
}

void RenderSystem::Resize(uint32_t width, uint32_t height) {
    m_desc.width  = width;
    m_desc.height = height;
    // TODO: Resize swap chain
}

void RenderSystem::SetMainPipeline(std::shared_ptr<IPipeline> pipeline) {
    m_mainPipeline = pipeline;
    if (pipeline && m_device) {
        pipeline->Initialize(m_device.get());
    }
}

void RenderSystem::SetGuiRenderCallback(GuiRenderCallback callback) {
    m_guiCallback = callback;
}

RenderSystem::RenderStats RenderSystem::GetRenderStats() const {
    RenderStats stats = m_stats;
    if (m_device) {
        auto devStats   = m_device->GetRenderStats();
        stats.drawCalls = devStats.drawCalls;
        stats.triangles = devStats.triangles;

        auto memInfo         = m_device->GetGPUMemoryInfo();
        stats.gpuMemoryUsage = memInfo.usedMemory;
    }
    return stats;
}

void RenderSystem::ResetStats() {
    m_stats = {};
}

bool RenderSystem::InitializeDevice(const RenderSystemDesc& desc) {
    LOG_INFO("Render", "正在探测可用后端...");
#ifdef PRISMA_ENABLE_RENDER_DX12
    LOG_INFO("Render", " - DirectX12: 已启用 (Enabled)");
#else
    LOG_INFO("Render", " - DirectX12: 未编译 (Disabled)");
#endif

#ifdef PRISMA_ENABLE_RENDER_VULKAN
    LOG_INFO("Render", " - Vulkan:    已启用 (Enabled)");
#else
    LOG_INFO("Render", " - Vulkan:    未编译 (Disabled)");
#endif
    Logger::GetInstance().Flush();

    switch (desc.backendType) {
#ifdef PRISMA_ENABLE_RENDER_DX12
        case RenderAPIType::DirectX12: {
            DeviceDesc devDesc;
            devDesc.windowHandle     = desc.windowHandle;
            devDesc.width            = desc.width;
            devDesc.height           = desc.height;
            devDesc.enableDebug      = desc.enableDebug;
            devDesc.enableValidation = desc.enableValidation;
            m_device                 = DX12::CreateDX12RenderDeviceInterface(devDesc);
            break;
        }
#endif
#ifdef PRISMA_ENABLE_RENDER_VULKAN
        case RenderAPIType::Vulkan: {
            DeviceDesc devDesc;
            devDesc.windowHandle     = desc.windowHandle;
            devDesc.width            = desc.width;
            devDesc.height           = desc.height;
            devDesc.enableDebug      = desc.enableDebug;
            devDesc.enableValidation = desc.enableValidation;
            m_device                 = Vulkan::CreateRenderDeviceVulkanInterface(devDesc);
            break;
        }
#endif
        default:
            return false;
    }
    return m_device != nullptr;
}

bool RenderSystem::InitializeResourceManager() {
    return m_device != nullptr;
}

bool RenderSystem::InitializePipelines() {
    auto forward      = std::make_shared<ForwardPipeline>();
    m_forwardPipeline = forward;
    m_mainPipeline    = forward;
    LOG_INFO("Render", "Render pipelines initialized.");
    return true;
}

void RenderSystem::RenderFrame() {
    BeginFrame();
    // TODO: Scene rendering
    EndFrame();
    Present();
}

void RenderSystem::UpdateStats(float deltaTime) {
    m_stats.frameTime             = deltaTime;
    static float fpsAccumulator   = 0.0f;
    static uint32_t fpsFrameCount = 0;
    static float fpsUpdateTime    = 0.0f;

    if (deltaTime > 0.0f) {
        fpsAccumulator += 1.0f / deltaTime;
        fpsFrameCount++;
        fpsUpdateTime += deltaTime;

        if (fpsUpdateTime >= 1.0f) {
            m_stats.fps    = fpsAccumulator / fpsFrameCount;
            fpsAccumulator = 0.0f;
            fpsFrameCount  = 0;
            fpsUpdateTime  = 0.0f;
        }
    }
}

RenderContext RenderSystem::GetRenderContext() const {
    RenderContext context;
    context.device     = m_device.get();
    context.frameIndex = m_stats.frameCount;
    context.deltaTime  = m_stats.frameTime;
    return context;
}

}  // namespace PrismaEngine::Graphic