#pragma once

#include "../Export.h"
#include "../core/ISubSystem.h"  // 继承自这个
#include "ICamera.h"
#include "Logger.h"
#include "RenderResourceManager.h"
#include "interfaces/IPipeline.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceManager.h"
#include "interfaces/RenderTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include "app/ProjectConfig.h"
namespace Prisma {
// 前向声明
class Scene;
}  // namespace Prisma

namespace Prisma::Graphic {

class ForwardPipeline;
class ICommandBuffer;
struct RenderSystemDesc {
    RenderAPIType backendType  = RenderAPIType::Vulkan;
    void* windowHandle         = nullptr;
    void* surface              = nullptr;
    uint32_t width             = 1600;
    uint32_t height            = 900;
    bool enableDebug           = true;
    bool enableValidation      = true;
    bool headless              = false;
    RenderMode renderMode      = RenderMode::Mode3D_Forward;
    PresentMode presentMode    = PresentMode::VSync;
    uint32_t maxSamples        = 512;
    uint32_t maxBounces        = 8;
    RTMode rtMode              = RTMode::HardwareRT;
    uint32_t maxBatchQuads     = 10000;
    uint32_t maxFramesInFlight = 3;
    std::string name           = "PrismaApp";
};

/* 渲染系统 (子系统) */
class ENGINE_API RenderSystem : public ISubSystem {
public:
    RenderSystem(const RenderSystemDesc& desc);
    ~RenderSystem() override;

    // ISubSystem 接口
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "RenderSystem"; }

    // === 帧控制 ===
    void BeginFrame();
    void EndFrame();
    void Present();
    void Resize(uint32_t width, uint32_t height);

    // === 设备访问 ===
    IRenderDevice* GetDevice() const {
        if (!m_device) {
            LOG_ERROR("RenderSystem", "尝试访问未初始化的渲染设备");
            return nullptr;
        }
        return m_device.get();
    }
    IRenderResourceManager* GetRenderResourceManager() const { return m_renderResourceManager.get(); }

    // === 渲染流程 ===
    void SetMainPipeline(std::shared_ptr<IPipeline> pipeline);
    IPipeline* GetMainPipeline() const { return m_mainRenderPipeline.get(); }
    template<typename T>
    std::shared_ptr<T> GetMainPipelineAs() const {
        return std::dynamic_pointer_cast<T>(m_mainRenderPipeline);
    }

    // === 场景渲染 ===
    void RenderScene(::Prisma::Scene* scene, ::Prisma::Graphic::ICamera* camera, ITexture* targetTexture = nullptr);

    // === 渲染模式切换 ===
    /// Runtime render mode switch.
    /// May trigger full device recreation if the new mode requires different Vulkan extensions.
    /// Must be called outside BeginFrame/EndFrame.
    int SetRenderMode(RenderMode newMode);
    int SetRenderMode(RenderMode newMode, RTMode rtMode);

    RenderMode GetCurrentRenderMode() const;
    bool IsRayTracingSupported() const;
    bool IsRayQuerySupported() const;

    /// Set callback invoked after render mode changes (e.g. for scene data re-upload).
    void SetRenderModeChangedCallback(std::function<void(RenderMode, IRenderDevice*)> callback);

    /// Set callback invoked after the main pipeline renders (for water, gizmos, etc.)
    /// Called between main pipeline Execute and EndFrame, with the command buffer still active.
    void SetWaterRenderCallback(std::function<void(ICommandBuffer*, IRenderDevice*)> callback);

private:
    int InitializeDevice();
    int InitializeRenderResourceManager();
    int InitializeRenderPipelines();

    RenderSystemDesc m_desc;
    std::unique_ptr<IRenderDevice> m_device;
    std::shared_ptr<RenderResourceManager> m_renderResourceManager;
    std::shared_ptr<IPipeline> m_mainRenderPipeline;

    std::mutex m_recreationMutex;
    std::atomic<bool> m_isRecreating{false};
    std::function<void(RenderMode, IRenderDevice*)> m_onRenderModeChanged;
    std::function<void(ICommandBuffer*, IRenderDevice*)> m_onWaterRender;

    /// Returns the RT extensions needed for a given render mode.
    RTMode GetRequiredRTMode(RenderMode mode) const;
};

}  // namespace Prisma::Graphic
