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
            LOG_INFO("Render", "DirectX12渲染后端创建完成");
            break;
        case RenderBackendType::Vulkan:
            this->renderBackend = std::make_unique<RenderBackendVulkan>();
            LOG_INFO("Render", "Vulkan渲染后端创建完成");
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

    if (!this->renderBackend->Initialize(platform, windowHandle, surface, width, height)) {
        LOG_ERROR("Render", "渲染后端初始化失败");
        return false;
    }
    
    // 初始化可编程渲染管线
    this->renderPipe = std::make_unique<ScriptableRenderPipeline>();
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

    // 获取并设置默认渲染目标和深度缓冲
    auto defaultRenderTarget = this->renderBackend->GetDefaultRenderTarget();
    auto defaultDepthBuffer = this->renderBackend->GetDefaultDepthBuffer();
    this->renderBackend->GetRenderTargetSize(width, height);

    if (defaultRenderTarget && defaultDepthBuffer) {
        // 设置所有RenderPass的渲染目标和深度缓冲
        this->forwardPipeline->SetRenderTargets(defaultRenderTarget, defaultDepthBuffer, width, height);
        LOG_INFO("Render", "设置默认渲染目标: {0}x{1}", width, height);
    }

    this->renderBackend->isInitialized = true;
    LOG_INFO("Render", "渲染系统初始化完成");
    return true;
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
    bool result = Initialize(platform, RenderBackendType::DirectX12, windowHandle, nullptr, 1600, 900);
    if (result && renderBackend) {
        renderBackend->isInitialized = true;
    }
    return result;
#else
    bool result = Initialize(platform, RenderBackendType::DirectX12, nullptr, nullptr, 1600, 900);
    if (result && renderBackend) {
        renderBackend->isInitialized = true;
    }
    return result;
#endif
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
    // 检查渲染后端是否初始化
    if (!renderBackend) {
        LOG_ERROR("Render", "渲染后端为空");
        return;
    }

    // 检查设备状态
    if (!renderBackend->isInitialized) {
        LOG_ERROR("Render", "渲染设备未初始化，无法继续渲染");
        throw std::runtime_error("渲染设备未初始化");
    }

    // 更新前向渲染管线（从Scene获取相机等数据）
    if (forwardPipeline) {
        forwardPipeline->Update(deltaTime);
    }

    // 在主线程中执行渲染
    RenderFrame();
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

void RenderSystem::Resize(uint32_t width, uint32_t height)
{
    if (!renderBackend) {
        LOG_ERROR("Render", "渲染后端未初始化，无法调整大小");
        return;
    }

    LOG_INFO("Render", "调整渲染大小到: {0}x{1}", width, height);

    // 调整渲染后端大小
    renderBackend->Resize(width, height);

    if (renderPipe) {
        renderPipe->SetViewportSize(width, height);
    }

    // 获取新的渲染目标和尺寸
    auto renderTarget = renderBackend->GetDefaultRenderTarget();
    auto depthBuffer = renderBackend->GetDefaultDepthBuffer();
    uint32_t newWidth, newHeight;
    renderBackend->GetRenderTargetSize(newWidth, newHeight);

    // 更新所有RenderPass的视口和渲染目标
    if (forwardPipeline && renderTarget && depthBuffer) {
        forwardPipeline->SetRenderTargets(renderTarget, depthBuffer, newWidth, newHeight);
    }
}

void RenderSystem::RenderFrame() {
    if (!renderBackend || !renderBackend->isInitialized) {
        LOG_WARNING("Render", "渲染后端未初始化，跳过渲染帧");
        return;
    }

    LOG_DEBUG("Render", "开始渲染帧");

    // 执行渲染帧
    renderBackend->BeginFrame();

    // 执行可编程渲染管线
    if (renderPipe) {
        LOG_DEBUG("Render", "执行可编程渲染管线");
        renderPipe->Execute();
    }

    LOG_DEBUG("Render", "结束渲染帧");
    renderBackend->EndFrame();
    LOG_DEBUG("Render", "Present帧");
    renderBackend->Present();
    LOG_DEBUG("Render", "渲染帧完成");
}

}  // namespace Engine