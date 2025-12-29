#pragma once

#include "interfaces/IRenderDevice.h"
#include "interfaces/IDeviceContext.h"
#include "interfaces/RenderTypes.h"
#include "math/MathTypes.h"
#include <cstdint>
#include <functional>

namespace PrismaEngine::Graphic {

// 前置声明
class Platform;
using WindowHandle = void*;

// 使用 RenderTypes.h 中定义的 RenderAPIType
// enum class RenderAPIType { DirectX12, Vulkan, OpenGL };

/// @brief 渲染器特性
enum class RendererFeature : uint32_t {
    None               = 0,
    MultiThreaded      = 1 << 0,  // 多线程渲染
    BindlessTextures   = 1 << 1,  // 支持 Bindless 资源
    MeshInstancing     = 1 << 2,  // 实例化渲染
    AsyncCompute       = 1 << 3,  // 异步 Compute
    RayTracing         = 1 << 4,  // 硬件光线追踪
    TileBasedRendering = 1 << 5,  // 瓦片渲染
    ComputeShader      = 1 << 6,  // 计算着色器
    GeometryShader     = 1 << 7,  // 几何着色器
    Tessellation       = 1 << 8   // 曲面细分
};

/// @brief 渲染命令（旧格式，待废弃）
struct RenderCommand {};

/// @brief 渲染 API
/// 渲染子系统的统一抽象层，提供设备级别的图形API抽象
/// 注意：这是一个临时适配器，后续应由具体后端（DX12/Vulkan）直接实现 IRenderDevice
class RenderAPI : public IRenderDevice {
public:
    RenderAPI();
    ~RenderAPI() override;

    // === IRenderDevice 接口实现 ===

    bool Initialize(const DeviceDesc& desc) override;
    void Shutdown() override;
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetAPIName() const override;

    // 命令缓冲区管理
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override;
    void SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence = nullptr) override;
    void SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                              const std::vector<IFence*>& fences = {}) override;

    // 同步操作
    void WaitForIdle() override;
    std::unique_ptr<IFence> CreateFence() override;
    void WaitForFence(IFence* fence) override;

    // 资源管理
    [[nodiscard]] IResourceFactory* GetResourceFactory() const override;

    // 交换链管理
    std::unique_ptr<ISwapChain> CreateSwapChain(void* windowHandle,
                                                 uint32_t width,
                                                 uint32_t height,
                                                 bool vsync = true) override;
    [[nodiscard]] ISwapChain* GetSwapChain() const override;

    // 帧管理
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    // 功能查询
    [[nodiscard]] bool SupportsMultiThreaded() const override;
    [[nodiscard]] bool SupportsBindlessTextures() const override;
    [[nodiscard]] bool SupportsComputeShader() const override;
    [[nodiscard]] bool SupportsRayTracing() const override;
    [[nodiscard]] bool SupportsMeshShader() const override;
    [[nodiscard]] bool SupportsVariableRateShading() const override;

    [[nodiscard]] GPUMemoryInfo GetGPUMemoryInfo() const override;
    [[nodiscard]] RenderStats GetRenderStats() const override;

    // 调试
    void BeginDebugMarker(const std::string& name) override;
    void EndDebugMarker() override;
    void SetDebugMarker(const std::string& name) override;

    // === 旧 API（待废弃） ===

    /// @brief 初始化后端（旧方法）
    /// @deprecated 使用 Initialize(const DeviceDesc&) 替代
    virtual bool InitializeLegacy(Platform* platform, WindowHandle windowHandle,
                                   void* surface, uint32_t width, uint32_t height);

    /// @brief 开始帧（旧方法）
    /// @deprecated 使用 BeginFrame() 替代
    virtual void BeginFrameLegacy(PrismaMath::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f});

    /// @brief 调整大小
    virtual void Resize(uint32_t width, uint32_t height);

    /// @brief 提交渲染命令
    virtual void SubmitRenderCommand(const RenderCommand& cmd);

    /// @brief 检查特性支持
    virtual bool Supports(RendererFeature feature) const;

    /// @brief 创建命令上下文（旧方法）
    /// @deprecated 使用 CreateCommandBuffer() 替代
    virtual IDeviceContext* CreateCommandContext();

    /// @brief 获取默认渲染目标（旧方法）
    /// @deprecated 应通过 IRenderTarget 接口获取
    virtual void* GetDefaultRenderTarget();

    /// @brief 获取默认深度缓冲（旧方法）
    /// @deprecated 应通过 IDepthStencil 接口获取
    virtual void* GetDefaultDepthBuffer();

    /// @brief 获取渲染目标尺寸
    virtual void GetRenderTargetSize(uint32_t& width, uint32_t& height);

    /// @brief GUI 渲染回调
    using GuiRenderCallback = std::function<void(void*)>;
    virtual void SetGuiRenderCallback(GuiRenderCallback callback);

    /// @brief 获取后端类型
    [[nodiscard]] RenderAPIType GetBackendType() const { return m_backendType; }

    /// @brief 是否已初始化
    [[nodiscard]] bool IsInitialized() const { return isInitialized; }

    // === 公共成员（兼容旧代码） ===

    [[nodiscard]] int32_t GetCurrentFrame() const { return m_currentFrame; }
    int m_currentFrame = 0;
    bool isInitialized = false;

protected:
    RenderAPIType m_backendType = RenderAPIType::None;
    RendererFeature m_supportedFeatures = static_cast<RendererFeature>(0);

    // 资源工厂
    IResourceFactory* m_resourceFactory = nullptr;
};

/// @brief 特性位运算辅助
inline RendererFeature operator|(RendererFeature a, RendererFeature b) {
    return static_cast<RendererFeature>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline RendererFeature operator&(RendererFeature a, RendererFeature b) {
    return static_cast<RendererFeature>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasFeature(RendererFeature features, RendererFeature feature) {
    return (static_cast<uint32_t>(features) & static_cast<uint32_t>(feature)) != 0;
}

} // namespace PrismaEngine::Graphic
