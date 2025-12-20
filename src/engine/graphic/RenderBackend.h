#pragma once
#include "Platform.h"
#include "RenderCommandContext.h"
#include "../math/MathTypes.h"
#include <cstdint>
#include <functional>

namespace Engine {
enum class RenderBackendType {
    None,
    SDL3,
    DirectX12,
    Vulkan,
};
struct RenderCommand {};

enum RendererFeature : uint32_t {
    None               = 0,
    MultiThreaded      = 1 << 0,  // 多线程渲染
    BindlessTextures   = 1 << 1,  // 支持 Bindless 资源
    MeshInstancing     = 1 << 2,  // 实例化渲染
    AsyncCompute       = 1 << 3,  // 异步 Compute
    RayTracing         = 1 << 4,  // 硬件光线追踪
    TileBasedRendering = 1 << 5,  // 新增瓦片渲染特性
};

class RenderBackend {
public:
    int m_currentFrame;
    RenderBackend() {}
    virtual ~RenderBackend() {}

    // 添加带参数的初始化方法
    virtual bool Initialize(Platform* platform, WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height) { return false; }
    virtual void Shutdown() = 0;

    // GUI 渲染回调，参数为 void* (VkCommandBuffer)
    using GuiRenderCallback = std::function<void(void*)>;
    virtual void SetGuiRenderCallback(GuiRenderCallback callback) {}

    virtual void BeginFrame(PrismaMath::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f}) = 0;
    virtual void EndFrame()   = 0;

    virtual void Resize(uint32_t width, uint32_t height) {}

    virtual void SubmitRenderCommand(const RenderCommand& cmd) = 0;

    virtual bool Supports(RendererFeature feature) const = 0;

    virtual void Present() = 0;
    
    // 创建渲染命令上下文
    virtual RenderCommandContext* CreateCommandContext() = 0;

    // 获取默认渲染目标和深度缓冲
    virtual void* GetDefaultRenderTarget() = 0;
    virtual void* GetDefaultDepthBuffer() = 0;

    // 获取当前渲染目标尺寸
    virtual void GetRenderTargetSize(uint32_t& width, uint32_t& height) = 0;

    bool isInitialized     = false;

protected:
    const RendererFeature m_support = RendererFeature::None;
};

}  // namespace Engine
