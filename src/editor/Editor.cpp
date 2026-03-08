#include "Editor.h"
#include "../engine/graphic/RenderSystem.h"
#include "CommandLineEditor.h"
#include "CommandLineParser.h"
#include "Environment.h"
#include "../engine/Engine.h"
#include "../engine/Platform.h"

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

namespace PrismaEngine {

std::shared_ptr<Editor> Editor::GetInstance() {
    static std::shared_ptr<Editor> instance = std::make_shared<Editor>();
    return instance;
}

Editor::Editor() : m_window(nullptr) {
}

Editor::~Editor() {
}

int Editor::Initialize() {
    LOG_INFO("Editor", "正在初始化编辑器 (Direct SDL3 Mode)...");

    // 1. 初始化 SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOG_ERROR("Editor", "SDL_Init 失败: {}", SDL_GetError());
        return 1;
    }

    // 2. 创建窗口 (直接调用 SDL3)
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_window = SDL_CreateWindow("PrismaEngine Editor", 1280, 720, window_flags);
    
    if (!m_window) {
        LOG_ERROR("Editor", "SDL_CreateWindow 失败: {}", SDL_GetError());
        return 2;
    }

    // 3. 初始化引擎核心
    if (EngineCore::GetInstance()->Initialize() != 0) {
        LOG_ERROR("Editor", "引擎核心初始化失败");
        return 3;
    }

    // 4. 初始化渲染后端 (将 SDL_Window* 作为 WindowHandle 传入)
    Graphic::RenderSystemDesc renderDesc;
    renderDesc.windowHandle = m_window; // 传入 SDL_Window*
    renderDesc.width = 1280;
    renderDesc.height = 720;
    renderDesc.enableValidation = true;

    if (!Graphic::RenderSystem::GetInstance()->Initialize(renderDesc)) {
        LOG_ERROR("Editor", "渲染系统初始化失败");
        return 4;
    }

    // 5. 初始化 ImGui
    if (!InitializeImGui()) {
        LOG_ERROR("Editor", "ImGui 初始化失败");
        return 5;
    }

    SetRunning(true);
    LOG_INFO("Editor", "编辑器初始化完成");
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

    // 绑定 SDL3 后端
    if (!ImGui_ImplSDL3_InitForVulkan(m_window)) {
        return false;
    }

    // 绑定渲染器后端 (通过 RenderSystem 辅助)
    return Graphic::RenderSystem::GetInstance()->InitializeImGui();
}

int Editor::Run() {
    while (IsRunning()) {
        ProcessEvents();

        if (m_minimized) {
            SDL_Delay(10);
            continue;
        }

        // 开始 ImGui 帧
        Graphic::RenderSystem::GetInstance()->BeginFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 绘制编辑器 UI
        DrawMainMenu();
        ImGui::ShowDemoWindow();

        // 结束 ImGui 帧并渲染
        ImGui::Render();
        Graphic::RenderSystem::GetInstance()->EndFrame();
        Graphic::RenderSystem::GetInstance()->Present();
    }
    return 0;
}

void Editor::ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT) {
            SetRunning(false);
        }
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_window)) {
            SetRunning(false);
        }
        if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {
            m_minimized = true;
        }
        if (event.type == SDL_EVENT_WINDOW_RESTORED) {
            m_minimized = false;
        }
    }
}

void Editor::DrawMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("文件")) {
            if (ImGui::MenuItem("退出", "Alt+F4")) SetRunning(false);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Editor::Shutdown() {
    LOG_INFO("Editor", "正在关闭编辑器...");
    
    Graphic::RenderSystem::GetInstance()->ShutdownImGui();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
    EngineCore::GetInstance()->Shutdown();
}

} // namespace PrismaEngine

// C 接口导出 (位于命名空间外)
extern "C" {
    EDITOR_API int PrismaEditor_Main(int argc, char** argv, Logger* externalLogger) {
        if (externalLogger) {
            Logger::SetInstance(externalLogger);
            LOG_INFO("EditorDLL", "成功挂载宿主日志系统");
        }

        PrismaEngine::CommandLineParser parser(argc, argv);
        parser.Parse();
        auto args = parser.GetArguments();
        
        PrismaEngine::IApplicationBase* app = nullptr;

        if (args.mode == PrismaEngine::EditorRunMode::CLI) {
            auto cliEditor = PrismaEngine::CommandLineEditor::GetInstance();
            cliEditor->SetArguments(args);
            app = cliEditor.get();
        } else {
            app = PrismaEngine::Editor::GetInstance().get();
        }

        int exitCode = app->Initialize();
        Logger::GetInstance().Flush();

        if (exitCode == 0) {
            exitCode = app->Run();
            app->Shutdown();
        }

        Logger::GetInstance().Flush();
        return exitCode;
    }
}
