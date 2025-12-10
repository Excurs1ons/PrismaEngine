#include "RenderSystem.h"
#include "Logger.h"
#include "PlatformWindows.h"
#include "RenderBackendDirectX12.h"
#include "RenderBackendVulkan.h"
#include "pipelines/forward/ForwardPipeline.h"
#if defined(_WIN32)
#include <Windows.h>
extern HWND g_hWnd;
#endif

namespace Engine {

bool RenderSystem::Initialize(
    Platform* platform, RenderBackendType renderBackendType, WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height) {
    LOG_INFO("Render", "正在初始化渲染系统: 后端类型={0}", static_cast<int>(renderBackendType));

    switch (renderBackendType) {
        case RenderBackendType::SDL3:
            LOG_ERROR("Render", "SDL3渲染后端尚未实现");
            return false;
        case RenderBackendType::DirectX12:
            this->renderBackend = std::make_unique<RenderBackendDirectX12>(L"RendererDirectX");
            break;
        case RenderBackendType::Vulkan:
            this->renderBackend = std::make_unique<RenderBackendVulkan>();
            break;
        case RenderBackendType::None:
        default:
            LOG_ERROR("Render", "未指定渲染后端");
            return false;
    }

    if (!this->renderBackend) {
        LOG_ERROR("Render", "渲染后端创建失败: {0}", static_cast<int>(renderBackendType));
        return false;
    }
    LOG_INFO("Render", "渲染后端创建完成 : {0}", static_cast<int>(renderBackendType));

    if (!this->renderBackend->Initialize(platform, windowHandle, surface, width, height)) {
        LOG_ERROR("Render", "渲染后端初始化失败");
        return false;
    }
    
    // 初始化可编程渲染管线
    this->renderPipe = std::make_unique<ScriptableRenderPipe>();
    if (!this->renderPipe->Initialize(this->renderBackend.get())) {
        LOG_ERROR("Render", "可编程渲染管线初始化失败");
        return false;
    }
    
    // 初始化前向渲染管线
    this->forwardPipeline = std::make_unique<Graphic::Pipelines::Forward::ForwardPipeline>();
    if (!this->forwardPipeline->Initialize(this->renderPipe.get())) {
        LOG_ERROR("Render", "前向渲染管线初始化失败");
        return false;
    }
    
    this->renderBackend->isInitialized = true;
    LOG_INFO("Render", "渲染系统初始化完成");
    return true;
}

bool RenderSystem::Initialize() {
    Platform* platform = nullptr;
#if defined(_WIN32)
    platform = PlatformWindows::GetInstance().get();
#endif
    return Initialize(platform, RenderBackendType::DirectX12, nullptr, nullptr, 1600, 900);
}

void RenderSystem::Shutdown() {
    LOG_INFO("Render", "渲染系统开始关闭");
    
    // 先关闭前向渲染管线
    if (forwardPipeline) {
        forwardPipeline->Shutdown();
        forwardPipeline.reset();
    }
    
    // 先关闭渲染管线
    if (renderPipe) {
        renderPipe->Shutdown();
        renderPipe.reset();
    }
    
    if (renderBackend) {
        renderBackend->Shutdown();
        renderBackend.reset();
    }
    
    LOG_INFO("Render", "渲染系统关闭完成");
}

void RenderSystem::Update(float deltaTime) {
    if (!renderBackend || !renderBackend->isInitialized) {
        return;
    }
    
    // 执行渲染帧
    renderBackend->BeginFrame();
    
    // 执行可编程渲染管线
    if (renderPipe) {
        renderPipe->Execute();
    }
    
    renderBackend->EndFrame();
    renderBackend->Present();
}

void RenderSystem::SetGuiRenderCallback(GuiRenderCallback callback) {
    if (renderBackend) {
        renderBackend->SetGuiRenderCallback(callback);
    }
}

void RenderSystem::BeginFrame() {
    if (renderBackend) {
        renderBackend->BeginFrame();
    }
}

void RenderSystem::EndFrame() {
    if (renderBackend) {
        renderBackend->EndFrame();
    }
}

void RenderSystem::Present() {
    if (renderBackend) {
        renderBackend->Present();
    }
}

void RenderSystem::Resize(uint32_t width, uint32_t height) {
    if (renderBackend) {
        renderBackend->Resize(width, height);
    }
    
    if (renderPipe) {
        renderPipe->SetViewportSize(width, height);
    }
}

}  // namespace Engine