#include "Logger.h"
#include "RenderSystem.h"
#include "RenderBackendDirectX12.h"
#include "RenderBackendVulkan.h"
#include "PlatformWindows.h"
#include <memory>

namespace Engine {
bool RenderSystem::Initialize(
    Platform* platform, RenderBackendType renderBackendType, WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height) {
    LOG_INFO("Render", "渲染系统初始化开始");
    
    // 如果没有提供窗口句柄，创建一个默认窗口
    if (!windowHandle) {
        LOG_DEBUG("Render", "窗口句柄为空，创建默认窗口");
        WindowProps props;
        props.Title  = "YAGE Render Window";
        props.Width  = 1600;
        props.Height = 900;
        props.Resizable = true;
        
        if (!platform) {
            LOG_ERROR("Render", "未提供平台接口，无法创建默认窗口");
            return false;
        }

        windowHandle = platform->CreateWindow(props);

        if (!windowHandle) {
            LOG_ERROR("Render", "创建默认窗口失败");
            return false;
        }
    }
    
    switch (renderBackendType) {
        case RenderBackendType::SDL3:
            LOG_ERROR("Render", "尚未实现SDL3渲染后端");
            return false;
        case RenderBackendType::DirectX12:
            this->renderBackend = std::make_unique<RenderBackendDirectX12>(L"RendererDirectX");
            break;
        case RenderBackendType::Vulkan:
            this->renderBackend = std::make_unique<RendererVulkan>();
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
}

}  // namespace Engine