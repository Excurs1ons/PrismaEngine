#include "Editor.h"
#include "../engine/graphic/RenderSystem.h"
#include "CommandLineEditor.h"
#include "CommandLineParser.h"
#include "Environment.h"
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
#include <iostream>

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
    LOG_INFO("Editor", "正在初始化图形界面编辑器...");

    // 1. 创建窗口
    WindowProps props;
    props.Width = 1600;
    props.Height = 900;
    props.Title = "Prisma Engine Editor";
    m_window = Platform::CreateWindow(props);

    if (!m_window) {
        LOG_FATAL("Editor", "无法创建编辑器窗口。");
        return 1;
    }

    // 2. 初始化渲染系统
    auto renderSystem = Graphic::RenderSystem::GetInstance();
    Graphic::RenderSystemDesc renderDesc;
    renderDesc.backendType = Graphic::RenderAPIType::Vulkan;
    renderDesc.windowHandle = m_window;
    renderDesc.width = props.Width;
    renderDesc.height = props.Height;
    renderDesc.name = "EditorRender";

    if (!renderSystem->Initialize(renderDesc)) {
        LOG_FATAL("Editor", "渲染系统初始化失败。");
        return 2;
    }

    // 3. 初始化 ImGui
    if (!InitializeImGui()) {
        LOG_ERROR("Editor", "ImGui 初始化失败。");
        return 3;
    }

    LOG_INFO("Editor", "图形界面编辑器初始化完成。");
    return 0;
}

bool Editor::InitializeImGui() {
    auto renderSystem = Graphic::RenderSystem::GetInstance();
    return renderSystem->InitializeImGui();
}

int Editor::Run() {
    LOG_INFO("Editor", "进入编辑器主循环。");
    
    while (!Platform::ShouldClose(m_window)) {
        Platform::PumpEvents();
        
#if defined(PRISMA_ENABLE_RENDER_VULKAN)
        ImGui_ImplVulkan_NewFrame();
#endif
#if defined(PRISMA_ENABLE_RENDER_DX12)
        ImGui_ImplDX12_NewFrame();
#endif
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        DrawMainMenu();

        ImGui::Render();
        
        auto renderSystem = Graphic::RenderSystem::GetInstance();
        renderSystem->BeginFrame();
        // 渲染逻辑...
        renderSystem->EndFrame();
        renderSystem->Present();
    }

    return 0;
}

void Editor::DrawMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("文件")) {
            if (ImGui::MenuItem("退出", "Alt+F4")) {
                Platform::SetShouldClose(m_window, true);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Editor::Shutdown() {
    LOG_INFO("Editor", "正在关闭编辑器...");
    
    auto renderSystem = Graphic::RenderSystem::GetInstance();
    if (renderSystem) {
        renderSystem->Shutdown();
    }

    if (m_window) {
        Platform::DestroyWindow(m_window);
        m_window = nullptr;
    }
}

// ========== DLL 导出 C 接口 ==========

extern "C" {
    /// @brief 编辑器统一入口点，承载所有运行模式逻辑
    EDITOR_API int PrismaEditor_Main(int argc, char** argv, Logger* externalLogger) {
        // 1. 同步日志单例（如果外部提供了实例）
        if (externalLogger) {
            Logger::SetInstance(externalLogger);
        } else {
            Logger::GetInstance().Initialize();
        }

        // 2. 解析命令行参数
        CommandLineParser parser(argc, argv);
        if (!parser.Parse()) {
            LOG_FATAL("Editor", "命令行参数解析失败");
            return 1;
        }

        const auto& args = parser.GetArguments();

        // 3. 处理信息显示
        if (parser.ShouldShowInfo()) {
            if (args.options.find("help") != args.options.end()) {
                parser.ShowHelp();
            } else if (args.options.find("version") != args.options.end()) {
                parser.ShowVersion();
            }
            return 0;
        }

        // 4. 环境探测与模式选择
        EnvironmentType env = Environment::DetectEnvironment();
        LOG_INFO("Editor", "检测到运行环境: {}", Environment::GetEnvironmentDescription());

        IApplicationBase* app = nullptr;
        EditorRunMode runMode = args.mode;

        if (runMode == EditorRunMode::CLI || runMode == EditorRunMode::Batch || env != EnvironmentType::Desktop) {
            LOG_INFO("Editor", "启动命令行编辑器模式");
            auto cliEditor = CommandLineEditor::GetInstance();
            cliEditor->SetArguments(args);
            app = cliEditor.get();
        } else {
            LOG_INFO("Editor", "启动图形界面编辑器模式");
            app = Editor::GetInstance().get();
        }

        // 5. 初始化并运行
        int exitCode = 0;
        LOG_INFO("Editor", "正在初始化子系统...");
        exitCode = app->Initialize();
        
        if (exitCode == 0) {
            LOG_INFO("Editor", "编辑器运行中...");
            try {
                exitCode = app->Run();
            } catch (const std::exception& e) {
                LOG_ERROR("Editor", "运行时异常: {}", e.what());
                exitCode = 1;
            }
            app->Shutdown();
        } else {
            LOG_FATAL("Editor", "编辑器初始化失败，错误码: {}", exitCode);
        }

        LOG_INFO("Editor", "编辑器已退出，状态码: {}", exitCode);
        Logger::GetInstance().Flush();
        return exitCode;
    }
}

} // namespace PrismaEngine