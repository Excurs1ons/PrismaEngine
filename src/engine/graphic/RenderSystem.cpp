#include "RenderSystem.h"
#include "Logger.h"
#include "PlatformWindows.h"
#include "RenderBackendDirectX12.h"
#include "RenderBackendVulkan.h"
#include "pipelines/forward/ForwardPipeline.h"
#include "RenderSystemNew.h"
#include "../SceneManager.h"
#include "../Camera3D.h"
#if defined(_WIN32)
#include <Windows.h>
extern HWND g_hWnd;
#endif

// 适配器实现包含在内，不暴露给外部

namespace Engine {

// RenderSystem适配器类的实现
RenderSystem::Adapter::Adapter(RenderSystem* renderSystem)
    : m_renderSystem(renderSystem) {}

// 静态适配器方法定义
bool RenderSystem::Initialize(
    Platform* platform, RenderBackendType renderBackendType,
    WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height) {

    // 创建适配器
    m_adapter = std::make_unique<Adapter>(this);

    // 使用适配器初始化
    bool result = m_adapter->Initialize(platform, renderBackendType, windowHandle, surface, width, height);

    if (result) {
        LOG_INFO("Render", "渲染系统（适配器）初始化成功");
    } else {
        LOG_ERROR("Render", "渲染系统（适配器）初始化失败");
    }

    return result;
}

bool RenderSystem::Initialize() {
    Platform* platform = nullptr;
#if defined(_WIN32)
    platform = PlatformWindows::GetInstance().get();
    // 创建一个默认窗口用于 DirectX 渲染
    WindowProps props("Game Window", 1600, 900);
    auto windowHandle = platform->CreateWindow(props);
    if (!windowHandle) {
        LOG_ERROR("Render", "无法创建默认窗口");
        return false;
    }
    bool result = Initialize(platform, Engine::RenderBackendType::DirectX12, windowHandle, nullptr, 1600, 900);
    if (result && renderBackend) {
        renderBackend->isInitialized = true;
    }
    return result;
#else
    bool result = Initialize(platform, Engine::RenderBackendType::DirectX12, nullptr, nullptr, 1600, 900);
    if (result && renderBackend) {
        renderBackend->isInitialized = true;
    }
    return result;
#endif
}

void RenderSystem::Shutdown() {
    if (m_adapter) {
        m_adapter->Shutdown();
        m_adapter.reset();
    }
}

void RenderSystem::Update(float deltaTime) {
    if (m_adapter) {
        m_adapter->Update(deltaTime);
    }
}

void RenderSystem::BeginFrame() {
    if (m_adapter) {
        m_adapter->BeginFrame();
    }
}

void RenderSystem::EndFrame() {
    if (m_adapter) {
        m_adapter->EndFrame();
    }
}

void RenderSystem::Present() {
    if (m_adapter) {
        m_adapter->Present();
    }
}

void RenderSystem::Resize(uint32_t width, uint32_t height) {
    if (m_adapter) {
        m_adapter->Resize(width, height);
    }
}

void RenderSystem::SetGuiRenderCallback(GuiRenderCallback callback) {
    if (m_adapter) {
        m_adapter->SetGuiRenderCallback(callback);
    }
}

// RenderSystem::Adapter方法实现
bool RenderSystem::Adapter::Initialize(Platform* platform, RenderBackendType renderBackendType,
                                     WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height) {
    // 创建新的渲染系统描述
    PrismaEngine::Graphic::RenderSystemDesc desc;
    // 转换Engine::RenderBackendType到PrismaEngine::Graphic::RenderBackendType
    switch (renderBackendType) {
        case Engine::RenderBackendType::DirectX12:
            desc.backendType = PrismaEngine::Graphic::RenderBackendType::DirectX12;
            break;
        case Engine::RenderBackendType::Vulkan:
            desc.backendType = PrismaEngine::Graphic::RenderBackendType::Vulkan;
            break;
        case Engine::RenderBackendType::SDL3:
        default:
            desc.backendType = PrismaEngine::Graphic::RenderBackendType::OpenGL; // Fallback
            break;
    }
    desc.windowHandle = windowHandle;
    desc.surface = surface;
    desc.width = width;
    desc.height = height;
    desc.enableDebug = false; // 可以从配置文件读取
    desc.name = "PrismaRenderSystem";

    // 初始化新的渲染系统
    if (!m_newRenderSystem.Initialize(desc)) {
        LOG_ERROR("Render", "新渲染系统初始化失败");
        return false;
    }

    // 将旧的后端指针设置到旧RenderSystem中（用于兼容性）
    m_renderSystem->renderBackend = m_newRenderSystem.GetRenderBackend();
    m_renderSystem->renderPipe = m_newRenderSystem.GetRenderPipe();

    // 注意：forwardPipeline需要在m_newRenderSystem中获取并移动
    // 这里我们需要修改RenderSystemNew来公开forwardPipeline的访问
    m_renderSystem->forwardPipeline = std::make_unique<Graphic::Pipelines::Forward::ForwardPipeline>();

    return true;
}

void RenderSystem::Adapter::BeginFrame() {
    m_newRenderSystem.BeginFrame();
}

void RenderSystem::Adapter::EndFrame() {
    m_newRenderSystem.EndFrame();
}

void RenderSystem::Adapter::Present() {
    m_newRenderSystem.Present();
}

void RenderSystem::Adapter::Resize(uint32_t width, uint32_t height) {
    m_newRenderSystem.Resize(width, height);
}

void RenderSystem::Adapter::SetGuiRenderCallback(GuiRenderCallback callback) {
    m_newRenderSystem.SetGuiRenderCallback(callback);
}

void RenderSystem::Adapter::Shutdown() {
    m_newRenderSystem.Shutdown();
}

void RenderSystem::Adapter::Update(float deltaTime) {
    m_newRenderSystem.Update(deltaTime);
}

}  // namespace Engine