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

namespace Prisma {

std::shared_ptr<Editor> Editor::Get() {
    static std::shared_ptr<Editor> instance = std::shared_ptr<Editor>(new Editor());
    s_Instance = instance.get();
    return instance;
}

Editor::Editor() 
    : Application({ "Prisma Editor", { 1280, 720 } }) {
}

Editor::~Editor() {
}

int Editor::OnInitialize() {
    LOG_INFO("Editor", "Initializing Editor Plugin (Pure Mode)...");

    // 注意：此时 RenderSystem 已经由 Engine 初始化完毕

    // 1. 初始化 ImGui
    if (!InitializeImGui()) {
        LOG_ERROR("Editor", "ImGui initialization failed");
        return 1;
    }

    // 2. 推送编辑器层
    PushLayer(new EditorLayer());

    LOG_INFO("Editor", "Editor Plugin initialized successfully.");
    return 0;
}

bool Editor::InitializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    // 绑定后端 (此时窗口已经由基类创建并就绪)
    SDL_Window* sdlWindow = static_cast<SDL_Window*>(GetWindow().GetNativeWindow());
    if (!ImGui_ImplSDL3_InitForVulkan(sdlWindow)) {
        return false;
    }

    return Graphic::RenderSystem::Get()->InitializeImGui();
}

void Editor::OnUpdate(Timestep ts) {
    Application::OnUpdate(ts);
}

void Editor::OnRender() {
    auto renderSystem = Graphic::RenderSystem::Get();
    
    renderSystem->BeginFrame();
    
    for (Layer* layer : m_LayerStack) {
        layer->OnRender();
    }
    
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    for (Layer* layer : m_LayerStack) {
        layer->OnImGuiRender();
    }

    ImGui::Render();
    
    renderSystem->EndFrame();
    renderSystem->Present();
}

void Editor::OnShutdown() {
    LOG_INFO("Editor", "Shutting down Editor...");

    Graphic::RenderSystem::Get()->ShutdownImGui();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // 注意：RenderSystem 的 Shutdown 由 Engine 统一管理
}

} // namespace Prisma

// ============================================================================
// Factory
// ============================================================================
extern "C" {
    EDITOR_API Prisma::Application* CreateApplication() {
        return Prisma::Editor::Get().get();
    }
}
