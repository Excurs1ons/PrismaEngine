#include "Editor.h"
#include "../graphic/RenderSystem.h"
#include "Logger.h"
#include "Platform.h"
#include <imgui.h>
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
#include <imgui_impl_vulkan.h>
#endif
#if defined(PRISMA_ENABLE_RENDER_DX12)
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#endif
#include <imgui_impl_sdl3.h>
#include <SDL3/SDL.h>

namespace PrismaEngine {

Editor::Editor() {
}

Editor::~Editor() {
    Shutdown();
}

bool Editor::Initialize() {
    LOG_INFO("Editor", "Initializing Editor...");

    // 1. Create Window via SDL3 (through Platform abstraction)
    WindowProps props;
    props.Width = 1600;
    props.Height = 900;
    props.Title = "Prisma Engine Editor";
    m_window = Platform::CreateWindow(props);

    if (!m_window) {
        LOG_FATAL("Editor", "Failed to create editor window.");
        return false;
    }

    // 2. Initialize RenderSystem with Vulkan
    auto renderSystem = Graphic::RenderSystem::GetInstance();
    Graphic::RenderSystemDesc renderDesc;
    renderDesc.backendType = Graphic::RenderAPIType::Vulkan;
    renderDesc.windowHandle = m_window;
    renderDesc.width = props.Width;
    renderDesc.height = props.Height;
    renderDesc.name = "EditorRender";

    if (!renderSystem->Initialize(renderDesc)) {
        LOG_FATAL("Editor", "Failed to initialize RenderSystem with Vulkan.");
        return false;
    }

    // 3. Initialize ImGui
    if (!InitializeImGui()) {
        LOG_ERROR("Editor", "Failed to initialize ImGui.");
        return false;
    }

    LOG_INFO("Editor", "Editor initialized successfully.");
    return true;
}

bool Editor::InitializeImGui() {
    auto renderSystem = Graphic::RenderSystem::GetInstance();
    return renderSystem->InitializeImGui();
}

int Editor::Run() {
    LOG_INFO("Editor", "Entering Editor main loop.");
    
    while (!Platform::ShouldClose(m_window)) {
        Platform::PumpEvents();
        
        // Start ImGui Frame
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
        ImGui_ImplVulkan_NewFrame();
#endif
#if defined(PRISMA_ENABLE_RENDER_DX12)
        ImGui_ImplDX12_NewFrame();
#endif
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        DrawMainMenu();

        // Render
        ImGui::Render();
        
        auto renderSystem = Graphic::RenderSystem::GetInstance();
        renderSystem->BeginFrame();
        // TODO: Main rendering logic
        renderSystem->EndFrame();
        renderSystem->Present();
    }

    return 0;
}

void Editor::DrawMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Exit logic can be added here
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Editor::Shutdown() {
    LOG_INFO("Editor", "Shutting down Editor...");
    
    auto renderSystem = Graphic::RenderSystem::GetInstance();
    if (renderSystem) {
        renderSystem->Shutdown();
    }

    if (m_window) {
        Platform::DestroyWindow(m_window);
        m_window = nullptr;
    }
}

// DLL Exported functions
extern "C" {
    EDITOR_API bool Initialize() {
        return PrismaEngine::Editor::GetInstance().Initialize();
    }

    EDITOR_API int Run() {
        return PrismaEngine::Editor::GetInstance().Run();
    }

    EDITOR_API void Shutdown() {
        PrismaEngine::Editor::GetInstance().Shutdown();
    }

    EDITOR_API void Update() {
        // Handled in Run loop
    }
}

} // namespace PrismaEngine