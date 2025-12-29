#include "Editor.h"
#include "Logger.h"
#include "Platform.h"
#include "../graphic/RenderSystemNew.h"
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

#include "SceneManager.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "nlohmann/json.hpp"
#include <vector>

#include <imgui.h>
#include <imgui_impl_sdl3.h>

using namespace Engine;

/// @brief 日志输出测试演示
void ShowDemo() {
    LOG_INFO("Demo", "这是一条信息消息");
    LOG_WARNING("Demo", "这是一条警告消息");
    LOG_ERROR("Demo", "这是一条错误消息");
    LOG_FATAL("Demo", "这是一条致命错误消息");
    LOG_DEBUG("Demo", "这是一条调试消息");
    LOG_TRACE("Demo", "这是一条跟踪消息");
}

Editor::Editor() {
    LOG_INFO("Editor", "正在初始化编辑器");
}

Editor::~Editor()
{
    LOG_INFO("Editor", "正在关闭编辑器");
}

bool Editor::Initialize()
{
    LOG_INFO("Editor", "正在初始化编辑器");

    // 1. 初始化平台 (SDL)
    if (!Platform::Initialize()) {
        LOG_FATAL("System", "平台初始化失败");
        return false;
    }

    // 2. 创建窗口
    WindowProps props("SDL3 Editor", 1600, 900);
    // props.FullScreenMode = FullScreenMode::Window;
    // props.ShowState = WindowShowState::Maximize;

    m_window = Platform::CreateWindow(props);

    if (!m_window) {
        LOG_FATAL("System", "无法创建窗口");
        return false;
    }

    // 3. 初始化渲染系统
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();


    if (!renderSystem->Initialize()) {
        LOG_FATAL("System", "渲染系统初始化失败");
        return false;
    }

    // 4. 初始化 ImGui
    if (!InitializeImGui()) {
        LOG_ERROR("Editor", "ImGui 初始化失败");
        return false;
    }

    return true;
}

bool Editor::InitializeImGui()
{
    LOG_INFO("Editor", "正在初始化 ImGui");

    // 1. 初始化 ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // 设置样式
    ImGui::StyleColorsDark();

    // 2. 初始化平台/渲染器后端
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();
    renderSystem->Initialize();
    // 初始化 SDL3
    if (!ImGui_ImplSDL3_InitForOther((SDL_Window*)m_window)) {
        LOG_ERROR("Editor", "ImGui SDL3 初始化失败");
        return false;
    }

    // 注册事件回调
    Platform::SetEventCallback([](const void* eventPtr) -> bool {
        const SDL_Event* event = static_cast<const SDL_Event*>(eventPtr);
        ImGui_ImplSDL3_ProcessEvent(event);

        if (event->type == SDL_EVENT_WINDOW_RESIZED) {
            PrismaEngine::Graphic::RenderSystem::GetInstance()->Resize(event->window.data1, event->window.data2);
        }

        return false;
    });

    // 注册渲染回调
    // 对于非Vulkan后端，使用通用的渲染回调
    PrismaEngine::Graphic::RenderSystem::GuiRenderCallback callback = [](void* cmdBuffer) {
        // 这里可以调用特定平台的ImGui渲染实现
        // 目前暂时留空
    };
    renderSystem->SetGuiRenderCallback(callback);

    return true;
}

int Editor::Run()
{
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();
    bool running = true;

    while (running) {
        Platform::PumpEvents();
        if (Platform::ShouldClose(m_window)) {
            running = false;
        }

        // 开始新的一帧
        renderSystem->BeginFrame();

        // ImGui 新帧
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 构建 UI
        ShowDemo();
        ImGui::ShowDemoWindow();

        // 渲染 ImGui
        ImGui::Render();

        // 结束帧并呈现
        renderSystem->EndFrame();
        renderSystem->Present();
    }
    return 0;
}

void Editor::Shutdown()
{
    LOG_INFO("Editor", "正在关闭编辑器");

    // 清理 ImGui
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // 清理渲染系统
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();
    if (renderSystem) {
        renderSystem->Shutdown();
    }

    // 清理平台
    Platform::Shutdown();
}