#include "Editor.h"
#include "EditorLayer.h"
#include "../engine/Engine.h"
#include "../engine/Platform.h"
#include "../engine/graphic/RenderSystem.h"
#include "graphic/ImGuiVulkanResourceManager.h"

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

namespace Prisma {

Editor* Editor::s_Instance = nullptr;

Editor::Editor() {
    s_Instance = this;
}

Editor::~Editor() {
    Shutdown();
    s_Instance = nullptr;
}

int Editor::Initialize() {
    // 1. 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_FATAL("Editor", "Failed to initialize SDL: {0}", SDL_GetError());
        return -1;
    }

    // 2. 创建窗口 (不强制要求 Vulkan 标志，除非我们需要底层互操作)
    m_Window = SDL_CreateWindow("Prisma Editor (Independent UI)", 1600, 900, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!m_Window) {
        LOG_FATAL("Editor", "Failed to create SDL window: {0}", SDL_GetError());
        return -1;
    }

    // 3. 创建 SDL_Renderer (用于 UI 渲染)
    m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
    if (!m_Renderer) {
        LOG_FATAL("Editor", "Failed to create SDL_Renderer: {0}", SDL_GetError());
        return -1;
    }

    // 4. 初始化 ImGui (使用 SDL_Renderer3 后端)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(m_Window, m_Renderer);
    ImGui_ImplSDLRenderer3_Init(m_Renderer);

    // 5. 初始化引擎 (Headless 模式)
    // 架构调整：引擎现在作为编辑器的子组件运行，不负责创建主窗口。
    EngineSpecification engineSpec;
    engineSpec.Name = "Prisma Engine Core";
    engineSpec.Headless = true; // 关键：不创建引擎级的 SDL 窗口
    
    m_Engine = std::make_unique<Engine>(engineSpec);
    if (m_Engine->Initialize() != 0) {
        LOG_FATAL("Editor", "Failed to initialize Engine core!");
        return -1;
    }

    // 6. 初始化编辑器层
    auto editorLayer = std::make_unique<EditorLayer>();
    m_Layers.push_back(std::move(editorLayer));

    m_Running = true;
    return 0;
}

void Editor::Run() {
    double lastFrameTime = Platform::GetTimeSeconds();

    while (m_Running) {
        double time = Platform::GetTimeSeconds();
        float deltaTime = static_cast<float>(time - lastFrameTime);
        lastFrameTime = time;

        // 1. 事件处理
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            OnEvent(event);
        }

        if (!m_Running) break;

        // 2. 引擎逻辑步进
        m_Engine->Update(Timestep(std::min(deltaTime, 0.1f)));

        // 3. 渲染流程开始
        m_Engine->BeginFrame(); // 虽然没有交换链，但会重置指令缓冲
        
        // 执行引擎渲染逻辑 (场景绘制到离屏纹理)
        m_Engine->Render(); 

        // 4. 编辑器 UI 渲染 (由 SDL_Renderer 负责)
        OnRender();

        m_Engine->EndFrame();
        // 注意：不调用 m_Engine->Present()，因为引擎没有交换链。
    }
}

void Editor::OnEvent(SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT) {
        m_Running = false;
    }
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window)) {
        m_Running = false;
    }
}

void Editor::OnRender() {
    // A. SDL_Renderer 清屏
    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_Renderer);

    // B. ImGui 帧开始
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // C. 提交所有层的 UI
    OnImGuiRender();

    // D. ImGui 渲染
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer);

    // E. 呈现窗口
    SDL_RenderPresent(m_Renderer);
}

void Editor::OnImGuiRender() {
    // 此处逻辑与原 Application::OnImGuiRender 类似，遍历 Layers
    for (auto& layer : m_Layers) {
        layer->OnImGuiRender();
    }
    
    if (m_showProjectSettings) {
        m_projectSettingsWindow.Draw(&m_showProjectSettings);
    }
}

void Editor::Shutdown() {
    if (!m_Running && !m_Window) return;

    LOG_INFO("Editor", "Shutting down Editor...");

    m_Layers.clear();

    if (m_Engine) {
        m_Engine->Shutdown();
        m_Engine.reset();
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (m_Renderer) {
        SDL_DestroyRenderer(m_Renderer);
        m_Renderer = nullptr;
    }

    if (m_Window) {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }

    SDL_Quit();
    m_Running = false;
}

} // namespace Prisma
