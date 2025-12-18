#pragma once

#include "../ManagerBase.h"
#include "RenderBackend.h"
#include "ScriptableRenderPipeline.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceManager.h"
#include "interfaces/IPipeline.h"
#include "WorkerThread.h"
#include <memory>
#include <functional>

namespace PrismaEngine::Graphic {

// 前置声明
class RenderBackendDirectX12;
class DX12RenderDevice;

// 前向声明 ForwardPipeline 类
namespace Engine { namespace Graphic { namespace Pipelines { namespace Forward {
    class ForwardPipeline;
}}}}

/// @brief 渲染系统描述
struct RenderSystemDesc {
    RenderBackendType backendType = RenderBackendType::DirectX12;
    void* windowHandle = nullptr;
    void* surface = nullptr;
    uint32_t width = 1600;
    uint32_t height = 900;
    bool enableDebug = false;
    bool enableValidation = false;
    uint32_t maxFramesInFlight = 2;
    std::string name = "PrismaRenderSystem";
};

/// @brief 新的渲染系统
/// 使用抽象接口，支持多后端
class RenderSystem : public ::Engine::ManagerBase<RenderSystem> {
public:
    friend class ::Engine::ManagerBase<RenderSystem>;
    static constexpr std::string GetName() { return "RenderSystem"; }

    // === 初始化和关闭 ===

    /// @brief 初始化渲染系统
    /// @param desc 渲染系统描述
    /// @return 是否初始化成功
    bool Initialize(const RenderSystemDesc& desc);

    /// @brief 初始化渲染系统（实现基类纯虚函数）
    /// @return 是否初始化成功
    bool Initialize() override;

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
    using GuiRenderCallback = std::function<void(void*)>;
    void SetGuiRenderCallback(GuiRenderCallback callback);

    // === 统计和调试 ===

    /// @brief 获取渲染统计信息
    struct RenderStats {
        uint32_t frameCount = 0;
        float frameTime = 0.0f;
        float fps = 0.0f;
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint64_t gpuMemoryUsage = 0;
        uint64_t cpuMemoryUsage = 0;
    };

    /// @brief 获取渲染统计
    /// @return 统计信息
    RenderStats GetRenderStats() const;

    /// @brief 重置统计信息
    void ResetStats();

    // === 兼容性接口 ===

    /// @brief 获取旧的渲染后端（用于兼容）
    /// @return 渲染后端指针
    RenderBackend* GetRenderBackend() const { return m_legacyBackend.get(); }

    /// @brief 获取旧的渲染管线（用于兼容）
    /// @return 渲染管线指针
    class ScriptableRenderPipeline* GetRenderPipe() const { return m_legacyPipeline.get(); }

private:
    // 新接口组件
    std::unique_ptr<IRenderDevice> m_device;
    std::unique_ptr<IResourceManager> m_resourceManager;
    std::shared_ptr<IPipeline> m_mainPipeline;

    // 旧接口组件（兼容性）
    std::unique_ptr<RenderBackend> m_legacyBackend;
    std::unique_ptr<class ScriptableRenderPipeline> m_legacyPipeline;
    std::unique_ptr<class Engine::Graphic::Pipelines::Forward::ForwardPipeline> m_forwardPipeline;

    // 渲染线程
    WorkerThread m_renderThread;
    std::function<void()> m_renderTask;

    // 渲染描述
    RenderSystemDesc m_desc;

    // 统计信息
    mutable RenderStats m_stats = {};

    // GUI回调
    GuiRenderCallback m_guiCallback;

    // === 内部方法 ===

    /// @brief 初始化设备
    /// @param desc 渲染系统描述
    /// @return 是否成功
    bool InitializeDevice(const RenderSystemDesc& desc);

    /// @brief 初始化资源管理器
    /// @return 是否成功
    bool InitializeResourceManager();

    /// @brief 初始化渲染流程
    /// @return 是否成功
    bool InitializePipelines();

    /// @brief 创建适配器（桥接新接口和旧后端）
    /// @return 是否成功
    bool CreateAdapters();

    /// @brief 渲染帧（在渲染线程中执行）
    void RenderFrame();

    /// @brief 更新统计信息
    void UpdateStats(float deltaTime);

    /// @brief 获取渲染上下文
    /// @return 渲染上下文
    RenderContext GetRenderContext() const;
};

} // namespace PrismaEngine::Graphic