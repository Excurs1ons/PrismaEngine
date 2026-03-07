#include "Editor.h"
#include "../graphic/RenderSystemNew.h"
#include "Logger.h"
#include "Platform.h"
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <directx/d3dx12.h>
#include <windows.h>
#endif

#include "Mesh.h"
#include "ResourceManagerNew.h"
#include "SceneManager.h"
#include "nlohmann/json.hpp"
#include <vector>

#include <imgui.h>

// 确保旧版 ImGui DX12 API 可用
#ifdef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#undef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#endif

// ImGui platform bindings - 根据渲染后端条件编译
#if defined(PRISMA_ENABLE_RENDER_DX12)
#include "../engine/graphic/adapters/dx12/DX12RenderDevice.h"
#include <d3d12.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#elif defined(PRISMA_ENABLE_RENDER_VULKAN) || defined(PRISMA_ENABLE_RENDER_OPENGL)
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#endif

#include "Mesh.h"
#include "ResourceManagerNew.h"
#include "SceneManager.h"
#include "nlohmann/json.hpp"
#include <vector>

#include <imgui.h>

// 确保旧版 ImGui DX12 API 可用
#ifdef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#undef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#endif

// ImGui platform bindings - 根据渲染后端条件编译
#if defined(PRISMA_ENABLE_RENDER_DX12)
#include "../engine/graphic/adapters/dx12/DX12RenderDevice.h"
#include <d3d12.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#elif defined(PRISMA_ENABLE_RENDER_VULKAN) || defined(PRISMA_ENABLE_RENDER_OPENGL)
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#endif

#include "Mesh.h"
#include "ResourceManagerNew.h"
#include "SceneManager.h"
#include "nlohmann/json.hpp"
#include <vector>

using namespace PrismaEngine;

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

Editor::~Editor() {
    LOG_INFO("Editor", "正在关闭编辑器");
}

bool Editor::Initialize() {
    // 1. 初始化平台 (SDL)
    if (!Platform::Initialize()) {
        LOG_FATAL("System", "平台初始化失败");
        return false;
    }

    // 2. 创建窗口
    WindowProps props("SDL3 Editor", 1600, 900);

    m_window = Platform::CreateWindow(props);

    if (!m_window) {
        LOG_FATAL("System", "无法创建窗口");
        return false;
    }

    LOG_INFO("Editor", "窗口句柄: 0x{0:X}", reinterpret_cast<uintptr_t>(m_window));

    // 3. 初始化渲染系统
    LOG_INFO("Editor", "准备初始化渲染系统...");
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();

    if (!renderSystem) {
        LOG_FATAL("Editor", "获取渲染系统实例失败");
        return false;
    }

    // Platform::CreateWindow() 直接返回 HWND（在 Windows 平台）
    HWND hwnd = static_cast<HWND>(m_window);

    // 准备渲染系统描述
    PrismaEngine::Graphic::RenderSystemDesc renderDesc;
    renderDesc.backendType  = PrismaEngine::Graphic::RenderAPIType::DirectX12;
    renderDesc.windowHandle = hwnd;
    renderDesc.width        = 1600;
    renderDesc.height       = 900;
    renderDesc.name         = "PrismaEditor";

    LOG_INFO("Editor", "Windows HWND: 0x{0:X}", reinterpret_cast<uintptr_t>(hwnd));
    LOG_INFO("Editor", "开始初始化渲染系统...");
    if (!renderSystem->Initialize(renderDesc)) {
        LOG_FATAL("System", "渲染系统初始化失败");
        return false;
    }

    LOG_INFO("Editor", "渲染系统初始化成功");

    // 4. 初始化 ImGui
    if (!InitializeImGui()) {
        LOG_ERROR("Editor", "ImGui 初始化失败");
        return false;
    }

    return true;
}

bool Editor::InitializeImGui() {
    LOG_INFO("Editor", "正在初始化 ImGui");

    // 1. 初始化 ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    LOG_INFO("Editor", "ImGui 上下文创建成功");

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // 加载默认字体
    io.Fonts->AddFontDefault();
    LOG_INFO("Editor", "默认字体加载成功");

    // 设置样式
    ImGui::StyleColorsDark();

    // 2. 通过 RenderSystem 初始化 ImGui（使用通用接口）
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();
    if (!renderSystem) {
        LOG_ERROR("Editor", "渲染系统未初始化，无法初始化 ImGui");
        return false;
    }

    if (!renderSystem->InitializeImGui()) {
        LOG_ERROR("Editor", "ImGui 通过 RenderSystem 初始化失败");
        return false;
    }

    LOG_INFO("Editor", "ImGui 通过 RenderSystem 初始化完成");

    // 2. 初始化平台/渲染器后端
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();

#if defined(PRISMA_ENABLE_RENDER_DX12)
    // DirectX 12 backend
    LOG_INFO("Editor", "初始化 ImGui Win32 平台后端");
    ImGui_ImplWin32_Init(GetModuleHandleA(NULL));
    LOG_INFO("Editor", "ImGui Win32 平台后端初始化成功");

    // 初始化 ImGui DX12 渲染后端
    auto device = renderSystem->GetDevice();
    if (!device) {
        LOG_ERROR("Editor", "渲染设备未初始化，无法初始化 ImGui DX12 后端");
        return false;
    }

    auto dx12Device = dynamic_cast<PrismaEngine::Graphic::DX12::DX12RenderDevice*>(device);
    if (!dx12Device) {
        LOG_ERROR("Editor", "无法获取 DX12 设备");
        return false;
    }

    LOG_INFO("Editor", "初始化 ImGui DX12 渲染后端");
    ID3D12Device* d3d12Device        = dx12Device->GetD3D12Device();
    ID3D12CommandQueue* commandQueue = dx12Device->GetCommandQueue();

    LOG_INFO("Editor", "获取 DX12 设备句柄: 0x{0:X}", reinterpret_cast<uintptr_t>(d3d12Device));
    LOG_INFO("Editor", "获取命令队列: 0x{0:X}", reinterpret_cast<uintptr_t>(commandQueue));

    // 创建 ImGui 专用的 SRV 描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.NumDescriptors             = 1;  // 只需要一个描述符用于字体纹理
    srvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = d3d12Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_imguiSrvHeap));
    if (FAILED(hr)) {
        LOG_ERROR("Editor", "创建 ImGui SRV 描述符堆失败: 0x{0:X}", hr);
        return false;
    }
    LOG_INFO("Editor", "ImGui SRV 描述符堆创建成功: 0x{0:X}", reinterpret_cast<uintptr_t>(m_imguiSrvHeap.Get()));

    // 使用旧版 ImGui_ImplDX12_Init API（更稳定）
    if (!ImGui_ImplDX12_Init(d3d12Device,
                             2,
                             DXGI_FORMAT_R8G8B8A8_UNORM,
                             m_imguiSrvHeap.Get(),
                             m_imguiSrvHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
                             m_imguiSrvHeap.Get()->GetGPUDescriptorHandleForHeapStart())) {
        LOG_ERROR("Editor", "ImGui DX12 后端初始化失败");
        return false;
    }

    // 显式创建设备对象（确保字体图集被构建）
    if (!ImGui_ImplDX12_CreateDeviceObjects()) {
        LOG_ERROR("Editor", "ImGui DX12 设备对象创建失败");
        return false;
    }

    LOG_INFO("Editor", "ImGui DX12 后端初始化成功");
#elif defined(PRISMA_ENABLE_RENDER_VULKAN) || defined(PRISMA_ENABLE_RENDER_OPENGL)
    // SDL3 backend (Vulkan/OpenGL)
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
#endif

    // 注册渲染回调
    // 对于非Vulkan后端，使用通用的渲染回调
    PrismaEngine::Graphic::RenderSystem::GuiRenderCallback callback = [](void* cmdBuffer) {
        // 这里可以调用特定平台的ImGui渲染实现
        // 目前暂时留空
    };
    renderSystem->SetGuiRenderCallback(callback);

    return true;
}

void Editor::DrawMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Project Settings")) {
                m_showProjectSettings = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

int Editor::Run() {
    auto renderSystem   = PrismaEngine::Graphic::RenderSystem::GetInstance();
    bool running        = true;
    uint32_t frameCount = 0;

    LOG_INFO("Editor", "编辑器主循环启动");

    while (running) {
        Platform::PumpEvents();
        if (Platform::ShouldClose(m_window)) {
            running = false;
        }

        // 开始新的一帧
        renderSystem->BeginFrame();

        // ImGui 新帧
#if defined(PRISMA_ENABLE_RENDER_DX12)
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
#elif defined(PRISMA_ENABLE_RENDER_VULKAN) || defined(PRISMA_ENABLE_RENDER_OPENGL)
        ImGui_ImplSDL3_NewFrame();
#endif
        ImGui::NewFrame();

        if (frameCount == 0) {
            LOG_INFO("Editor", "第一帧: ImGui NewFrame 调用成功");
        }

        // 构建 UI
        DrawMainMenu();

        if (m_showProjectSettings) {
            m_projectSettingsWindow.Draw(&m_showProjectSettings);
        }

        if (frameCount == 0) {
            LOG_INFO("Editor", "第一帧: UI 构建完成");
        }

        ShowDemo();
        ImGui::ShowDemoWindow();

        // 渲染 ImGui
        ImGui::Render();

        if (frameCount == 0) {
            LOG_INFO("Editor", "第一帧: ImGui Render 调用成功");
        }

#if defined(PRISMA_ENABLE_RENDER_DX12)
        // 获取 DX12 命令列表并渲染 ImGui
        auto dx12Device = dynamic_cast<PrismaEngine::Graphic::DX12::DX12RenderDevice*>(renderSystem->GetDevice());
        if (dx12Device) {
            ID3D12GraphicsCommandList* commandList = dx12Device->GetCommandList();
            if (commandList) {
                ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
                if (frameCount == 0) {
                    LOG_INFO("Editor", "第一帧: ImGui DX12 渲染完成");
                }
            } else {
                if (frameCount == 0) {
                    LOG_INFO("Editor", "第一帧: commandList 为 nullptr，无法渲染 ImGui");
                }
            }
        }

        // 结束帧并呈现
        renderSystem->EndFrame();
        renderSystem->Present();

        frameCount++;
    }
    return 0;
}

void Editor::Shutdown() {
    LOG_INFO("Editor", "正在关闭编辑器");

    // 清理 ImGui
#if defined(PRISMA_ENABLE_RENDER_DX12)
    ImGui_ImplWin32_Shutdown();
#elif defined(PRISMA_ENABLE_RENDER_VULKAN) || defined(PRISMA_ENABLE_RENDER_OPENGL)
    ImGui_ImplSDL3_Shutdown();
#endif
    ImGui::DestroyContext();

    // 清理渲染系统
    auto renderSystem = PrismaEngine::Graphic::RenderSystem::GetInstance();
    if (renderSystem) {
        renderSystem->Shutdown();
    }

    // 清理平台
    Platform::Shutdown();
}