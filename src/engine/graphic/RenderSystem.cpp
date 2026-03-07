#include "RenderSystem.h"
#include "../Camera.h"
#include "../Logger.h"
#include "../SceneManager.h"
#include "../core/ECS.h"
#include "pipelines/forward/ForwardPipeline.h"

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
extern HWND g_hWnd;
#endif

#include <SDL3/SDL.h>

namespace PrismaEngine::Graphic {

bool RenderSystem::Initialize(const RenderSystemDesc& desc) {
    LOG_INFO("Render", "Initializing RenderSystem...");
    m_desc = desc;

    if (!InitializeDevice(desc)) {
        LOG_ERROR("Render", "Failed to initialize render device.");
        return false;
    }

    if (!InitializeResourceManager()) {
        LOG_ERROR("Render", "Failed to initialize resource manager.");
        return false;
    }

    if (!InitializePipelines()) {
        LOG_ERROR("Render", "Failed to initialize render pipelines.");
        return false;
    }

    m_renderThread.Start();
    LOG_INFO("Render", "RenderSystem initialized successfully.");
    return true;
}

bool RenderSystem::Initialize() {
    RenderSystemDesc desc;
    return Initialize(desc);
}

bool RenderSystem::InitializeImGui() {
    if (!m_device) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

#if defined(PRISMA_ENABLE_RENDER_DX12)
    if (m_desc.backendType == RenderAPIType::DirectX12) {
        if (!ImGui_ImplWin32_Init(g_hWnd)) return false;
        if (!m_device->InitializeImGui()) return false;
    }
#elif defined(PRISMA_ENABLE_RENDER_VULKAN)
    if (m_desc.windowHandle) {
        if (!ImGui_ImplSDL3_InitForVulkan((SDL_Window*)m_desc.windowHandle)) return false;
    }
    if (!m_device->InitializeImGui()) return false;
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

void RenderSystem::Shutdown() {
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

RenderSystem::~RenderSystem() {
    Shutdown();
}

void RenderSystem::Update(float deltaTime) {
    UpdateStats(deltaTime);
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
}

void RenderSystem::SetMainPipeline(std::shared_ptr<ForwardPipeline> pipeline) {
    m_mainPipeline = pipeline;
}

void RenderSystem::SetGuiRenderCallback(GuiRenderCallback callback) {
    m_guiCallback = callback;
}

RenderSystem::RenderStats RenderSystem::GetRenderStats() const {
    return m_stats;
}

void PrismaEngine::Graphic::RenderSystem::ResetStats() {
    m_stats = {};
}

bool RenderSystem::InitializeDevice(const RenderSystemDesc& desc) {
    switch (desc.backendType) {
#ifdef PRISMA_ENABLE_RENDER_DX12
        case RenderAPIType::DirectX12: {
            DeviceDesc deviceDesc;
            deviceDesc.windowHandle = desc.windowHandle;
            m_device = DX12::CreateDX12RenderDeviceInterface(deviceDesc);
            break;
        }
#endif
#ifdef PRISMA_ENABLE_RENDER_VULKAN
        case RenderAPIType::Vulkan: {
            DeviceDesc deviceDesc;
            deviceDesc.windowHandle = desc.windowHandle;
            m_device = Vulkan::CreateRenderDeviceVulkanInterface(deviceDesc);
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
    m_forwardPipeline = std::make_shared<ForwardPipeline>();
    m_mainPipeline = m_forwardPipeline;
    return true;
}

void RenderSystem::RenderFrame() {
    BeginFrame();
    EndFrame();
    Present();
}

void RenderSystem::UpdateStats(float deltaTime) {
    m_stats.frameTime = deltaTime;
    m_stats.fps = deltaTime > 0 ? 1.0f / deltaTime : 0;
}

} // namespace PrismaEngine::Graphic