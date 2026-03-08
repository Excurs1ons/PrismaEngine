#include "RenderSystem.h"
#include "../Camera.h"
#include "../Logger.h"
#include "../SceneManager.h"
#include "../core/ECS.h"
#include "pipelines/forward/ForwardPipeline.h"

// ImGui
#include <imgui.h>

#ifdef PRISMA_ENABLE_RENDER_DX12
#include "adapters/dx12/DX12Adapters.h"
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#endif

#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include "adapters/vulkan/VulkanAdapters.h"
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include <SDL3/SDL.h>

namespace PrismaEngine::Graphic {

bool RenderSystem::Initialize() {
    RenderSystemDesc defaultDesc;
    return Initialize(defaultDesc);
}

bool RenderSystem::Initialize(const RenderSystemDesc& desc) {
    LOG_INFO("Render", "Initializing RenderSystem...");
    m_desc = desc;

    // 1. 初始化设备
    if (!InitializeDevice(desc)) {
        LOG_ERROR("Render", "Failed to initialize render device.");
        return false;
    }

    // 2. 初始化资源管理器
    if (!InitializeResourceManager()) {
        LOG_ERROR("Render", "Failed to initialize resource manager.");
        return false;
    }

    // 3. 初始化管线
    if (!InitializePipelines()) {
        LOG_ERROR("Render", "Failed to initialize render pipelines.");
        return false;
    }

    // 4. 启动渲染线程
    m_renderThread.Start();

    LOG_INFO("Render", "RenderSystem initialized successfully.");
    return true;
}

void RenderSystem::Shutdown() {
    LOG_INFO("Render", "Shutting down RenderSystem...");

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

    LOG_INFO("Render", "RenderSystem shut down.");
}

bool RenderSystem::InitializeImGui() {
    if (!m_device) {
        LOG_ERROR("Render", "Device not initialized, cannot init ImGui.");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

#if defined(PRISMA_ENABLE_RENDER_DX12)
    if (m_desc.backendType == RenderAPIType::DirectX12) {
        LOG_INFO("Render", "Initializing ImGui Win32 + DX12");
        if (!ImGui_ImplWin32_Init(static_cast<HWND>(m_desc.windowHandle))) return false;
        if (!m_device->InitializeImGui()) return false;
    }
#endif

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    if (m_desc.backendType == RenderAPIType::Vulkan) {
        LOG_INFO("Render", "Initializing ImGui SDL3 + Vulkan");
        if (m_desc.windowHandle) {
            if (!ImGui_ImplSDL3_InitForVulkan((SDL_Window*)m_desc.windowHandle)) return false;
        }
        if (!m_device->InitializeImGui()) return false;
    }
#endif

    LOG_INFO("Render", "ImGui initialized successfully.");
    return true;
}

void RenderSystem::ShutdownImGui() {
#if defined(PRISMA_ENABLE_RENDER_DX12)
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
#elif defined(PRISMA_ENABLE_RENDER_VULKAN)
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
#endif
    ImGui::DestroyContext();
}

RenderSystem::~RenderSystem() {
    Shutdown();
}

void RenderSystem::Update(float deltaTime) {
    UpdateStats(deltaTime);

    if (!m_renderThread.IsRunning()) {
        RenderFrame();
    }
}

void RenderSystem::BeginFrame() {
    if (m_device) m_device->BeginFrame();
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
    if (m_device) m_device->Present();
    m_stats.frameCount++;
}

void RenderSystem::Resize(uint32_t width, uint32_t height) {
    m_desc.width = width;
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
        auto devStats = m_device->GetRenderStats();
        stats.drawCalls = devStats.drawCalls;
        stats.triangles = devStats.triangles;
        
        auto memInfo = m_device->GetGPUMemoryInfo();
        stats.gpuMemoryUsage = memInfo.usedMemory;
    }
    return stats;
}

void RenderSystem::ResetStats() {
    m_stats = {};
}

bool RenderSystem::InitializeDevice(const RenderSystemDesc& desc) {
    switch (desc.backendType) {
#ifdef PRISMA_ENABLE_RENDER_DX12
        case RenderAPIType::DirectX12: {
            DeviceDesc devDesc;
            devDesc.windowHandle = desc.windowHandle;
            devDesc.width = desc.width;
            devDesc.height = desc.height;
            devDesc.enableDebug = desc.enableDebug;
            devDesc.enableValidation = desc.enableValidation;
            m_device = DX12::CreateDX12RenderDeviceInterface(devDesc);
            break;
        }
#endif
#ifdef PRISMA_ENABLE_RENDER_VULKAN
        case RenderAPIType::Vulkan: {
            DeviceDesc devDesc;
            devDesc.windowHandle = desc.windowHandle;
            devDesc.width = desc.width;
            devDesc.height = desc.height;
            devDesc.enableDebug = desc.enableDebug;
            devDesc.enableValidation = desc.enableValidation;
            m_device = Vulkan::CreateRenderDeviceVulkanInterface(devDesc);
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
    auto forward = std::make_shared<ForwardPipeline>();
    m_forwardPipeline = forward;
    m_mainPipeline = forward;
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
    m_stats.frameTime = deltaTime;
    static float fpsAccumulator = 0.0f;
    static uint32_t fpsFrameCount = 0;
    static float fpsUpdateTime = 0.0f;

    if (deltaTime > 0.0f) {
        fpsAccumulator += 1.0f / deltaTime;
        fpsFrameCount++;
        fpsUpdateTime += deltaTime;

        if (fpsUpdateTime >= 1.0f) {
            m_stats.fps = fpsAccumulator / fpsFrameCount;
            fpsAccumulator = 0.0f;
            fpsFrameCount = 0;
            fpsUpdateTime = 0.0f;
        }
    }
}

RenderContext RenderSystem::GetRenderContext() const {
    RenderContext context;
    context.device = m_device.get();
    context.frameIndex = m_stats.frameCount;
    context.deltaTime = m_stats.frameTime;
    return context;
}

} // namespace PrismaEngine::Graphic