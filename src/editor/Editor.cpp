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
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    auto& engine      = Engine::Get();
    auto renderSystem = engine.GetRenderSystem();
    auto device       = renderSystem->GetDevice();
    auto vkInstance   = device->GetVkInstance();
    // 绑定后端
    auto& window          = engine.GetWindow();
    SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());
    if (!ImGui_ImplSDL3_InitForVulkan(sdlWindow)) {

        return -1;
    }

    ImGui_ImplVulkan_InitInfo init_info;
    init_info.Instance       = device->GetVkInstance();
    init_info.PhysicalDevice = device->GetPhysicalDevice();
    init_info.Device         = device->GetVkDevice();
    init_info.QueueFamily    = device->GetGraphicsQueueFamily();
    init_info.Queue          = device->GetGraphicsQueue();
    init_info.DescriptorPool = device->GetImGuiDescriptorPool();
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.DescriptorPool = VK_NULL_HANDLE;
    init_info.MinImageCount  = 2;
    init_info.ImageCount     = 2;
    init_info.UseDynamicRendering               = false;
    init_info.Allocator           = nullptr;
    init_info.CheckVkResultFn                   = nullptr;
    if (!ImGui_ImplVulkan_Init(&init_info)) {
        return -1;
    }
    return Engine::Get().GetRenderSystem()->InitializeImGui();
}
void Editor::OnUpdate(Timestep ts) {
    Application::OnUpdate(ts);
}

void Editor::OnImGuiRender() {
    // 1. ImGui 帧开始
    ImGui_ImplSDL3_NewFrame();
    // ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    // 2. 渲染所有 Layer 的 UI
    Application::OnImGuiRender();

    // 3. ImGui 帧结束
    ImGui::Render();
}

void Editor::OnRender() {
    // 这里放置场景提交逻辑 (由 Engine 循环调用)
    Application::OnRender();
}

void Editor::OnShutdown() {
    LOG_INFO("Editor", "Shutting down Editor...");

    auto renderSystem = Engine::Get().GetRenderSystem();
    if (renderSystem) {
        renderSystem->ShutdownImGui();
    }

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
