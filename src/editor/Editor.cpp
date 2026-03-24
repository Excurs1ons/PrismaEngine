#include "Editor.h"
#include "EditorLayer.h"
#include "../engine/Engine.h"
#include "../engine/Platform.h"
#include "../engine/graphic/RenderSystem.h"
#include "../engine/SceneManager.h"
#include "../engine/Scene.h"
#include "CommandLineEditor.h"
#include "CommandLineParser.h"
#include "Environment.h"

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>


#define IMGUI_IMPL_VULKAN_USE_LOADER

namespace Prisma {

Editor::Editor() : Application(ApplicationSpecification{"Prisma Editor", 1280, 720}) {}

Editor::~Editor() {}

int Editor::OnInitialize() {
    LOG_INFO("Editor", "Initializing Editor Plugin (Pure Mode)...");

    // 1. 初始化 ImGui
    int result = OnImGuiInitialize();
    if (result != 0) {
        LOG_ERROR("Editor", "ImGui initialization failed");
        return result;
    }

    // 2. 推送编辑器层
    PushLayer(new EditorLayer());

    LOG_INFO("Editor", "Editor Plugin initialized successfully.");
    return 0;
}

int Editor::OnImGuiInitialize() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;  // 禁用多视口
    ImGui::StyleColorsDark();

    auto& engine      = Engine::Get();
    auto renderSystem = engine.GetRenderSystem();
    auto device       = renderSystem->GetDevice();
    // 绑定后端
    auto& window          = engine.GetWindow();
    SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());
    if (!ImGui_ImplSDL3_InitForVulkan(sdlWindow)) {

        return -1;
    }

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance       = device->GetVkInstance();
    init_info.PhysicalDevice = device->GetPhysicalDevice();
    init_info.Device         = device->GetVkDevice();
    init_info.QueueFamily    = device->GetGraphicsQueueFamily();
    init_info.Queue          = device->GetGraphicsQueue();
    init_info.DescriptorPool = device->GetImGuiDescriptorPool();
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.MinImageCount  = 3;
    init_info.ImageCount     = 3;
    init_info.UseDynamicRendering               = false;
    init_info.Allocator           = nullptr;
    init_info.CheckVkResultFn                   = nullptr;
    // --- 关键：新版本设置 RenderPass 的地方 ---
    init_info.PipelineInfoMain.RenderPass  = device->GetImGuiRenderPass();
    init_info.PipelineInfoMain.Subpass     = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    // 如果你开启了多窗口 (Viewports)，副窗口通常也用同样的设置
    init_info.PipelineInfoForViewports = init_info.PipelineInfoMain;
   
    if (!ImGui_ImplVulkan_Init(&init_info)) {
        return -1;
    }

    return 0;
}
void Editor::OnUpdate(Timestep ts) {
    Application::OnUpdate(ts);
}

void* Editor::GetImGuiContext() {
    return (void*)ImGui::GetCurrentContext();
}

void Editor::OnImGuiRender() {
    // 1. ImGui 帧开始
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 2. 渲染所有 Layer 的 UI
    Application::OnImGuiRender();

    // 渲染 Editor 自己的内置窗口
    if (m_showProjectSettings) {
        m_projectSettingsWindow.Draw(&m_showProjectSettings);
    }

    // 3. ImGui 帧结束
    ImGui::Render();
    // 注意：不再在这里调用 ImGui_ImplVulkan_RenderDrawData
    // 而是由 RenderDeviceVulkan::EndFrame 在正确的 RenderPass 中调用
}

void Editor::OnRender() {
    // 这里放置场景提交逻辑 (由 Engine 循环调用)
    Application::OnRender();
}

void Editor::OnShutdown() {
    LOG_INFO("Editor", "Shutting down Editor...");

    if (auto renderSystem = Engine::Get().GetRenderSystem()) {
        if (renderSystem->GetDevice()) {
            renderSystem->GetDevice()->WaitForIdle();
        }
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

}  // namespace Prisma

// ============================================================================
// Factory
// ============================================================================
extern "C" EDITOR_API Prisma::Application* CreateApplication() {
    return new Prisma::Editor();
}
