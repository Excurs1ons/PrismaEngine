#pragma once

#include "../Export.h"
#include "../ManagerBase.h"
#include "WorkerThread.h"
#include "interfaces/IPipeline.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceManager.h"
#include "interfaces/RenderTypes.h"
#include <functional>
#include <memory>
#include <string>

namespace PrismaEngine::Graphic {

// 前置声明
class ForwardPipeline;

/// @brief 渲染系统描述
struct RenderSystemDesc {
    RenderAPIType backendType  = RenderAPIType::Vulkan;
    void* windowHandle         = nullptr;
    void* surface              = nullptr;
    uint32_t width             = 1600;
    uint32_t height            = 900;
    bool enableDebug           = true;
    bool enableValidation      = true;
    uint32_t maxFramesInFlight = 3;
    std::string name           = "PrismaEditor";
};

/// @brief 渲染系统实现
/// 使用抽象接口，支持多后端
class ENGINE_API RenderSystem : public ::PrismaEngine::ManagerBase<RenderSystem> {
public:
    /// @brief 获取单例实例
    static std::shared_ptr<RenderSystem> GetInstance();

    static constexpr const char* GetStaticName() { return "RenderSystem"; }

    // === 初始化和关闭 ===

    /// @brief 初始化渲染系统
    /// @param desc 渲染系统描述
    /// @return 是否初始化成功
    int Initialize(const RenderSystemDesc& desc);

    /// @brief 初始化渲染系统（实现基类纯虚函数）
    /// @return 是否初始化成功
    int Initialize() override;

    /// @brief 初始化 ImGui（与渲染后端无关）
    /// @return 是否初始化成功
#ifdef PRISMA_BUILD_EDITOR
    bool InitializeImGui();

    /// @brief 清理 ImGui 资源
    void ShutdownImGui();
#else
    bool InitializeImGui() { return false; }
    void ShutdownImGui() {}
#endif

    /// @brief 关闭渲染系统
    void Shutdown() override;

    /// @brief 析构函数
    ~RenderSystem();

    /// @brief 更新（每帧调用）
    /// @param deltaTime 时间增量
    void Update(float deltaTime) override;

    // === 帧控制 ===

    /// @brief 开始帧渲染
    void BeginFrame();

    /// @brief 结束帧渲染
    void EndFrame();

    /// @brief 呈现（交换缓冲区）
    void Present();

    /// @brief 调整渲染目标大小
    /// @param width 新宽度
    /// @param height 新高度
    void Resize(uint32_t width, uint32_t height);

    // === 设备和资源访问 ===

    /// @brief 获取渲染设备
    /// @return 渲染设备指针
    IRenderDevice* GetDevice() const { return m_device.get(); }

    /// @brief 获取资源管理器
    /// @return 资源管理器指针
    IResourceManager* GetResourceManager() const { return m_resourceManager.get(); }

    // === 渲染流程管理 ===

    /// @brief 设置主渲染流程
    /// @param pipeline 渲染流程
    void SetMainPipeline(std::shared_ptr<IPipeline> pipeline);

    /// @brief 获取主渲染流程
    /// @return 渲染流程指针
    IPipeline* GetMainPipeline() const { return m_mainPipeline.get(); }

    // === 回调函数 ===
    /// @brief GUI渲染回调
    using GuiRenderCallback = std::function<void(IRenderDevice*)>;
    void SetGuiRenderCallback(GuiRenderCallback callback);

    // === 统计和调试 ===

    /// @brief 渲染统计信息
    struct RenderStats {
        uint32_t frameCount     = 0;
        float frameTime         = 0.0f;
        float fps               = 0.0f;
        uint32_t drawCalls      = 0;
        uint32_t triangles      = 0;
        uint64_t gpuMemoryUsage = 0;
        uint64_t cpuMemoryUsage = 0;
    };

    /// @brief 获取渲染统计
    /// @return 统计信息
    RenderStats GetRenderStats() const;

    /// @brief 重置统计信息
    void ResetStats();

    /// @brief 渲染帧（在渲染线程中执行）
    void RenderFrame();

private:
    // 内部组件
    std::unique_ptr<IRenderDevice> m_device;
    std::unique_ptr<IResourceManager> m_resourceManager;
    std::shared_ptr<IPipeline> m_mainPipeline;
    std::shared_ptr<ForwardPipeline> m_forwardPipeline;

    // 渲染线程
    WorkerThread m_renderThread;

    // 渲染描述
    RenderSystemDesc m_desc;

    // 统计信息
    mutable RenderStats m_stats = {};

    // GUI回调
    GuiRenderCallback m_guiCallback;
#ifdef PRISMA_BUILD_EDITOR
    bool m_imguiInitialized = false;
#endif

    // === 内部方法 ===

    bool InitializeDevice(const RenderSystemDesc& desc);
    bool InitializeResourceManager();
    bool InitializePipelines();
    void UpdateStats(float deltaTime);
    RenderContext GetRenderContext() const;
};

}  // namespace PrismaEngine::Graphic