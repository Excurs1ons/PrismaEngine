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
#include "app/ProjectConfig.h"
namespace Prisma {
// 前向声明
class Scene;
}  // namespace Prisma

namespace Prisma::Graphic {

class ForwardPipeline;
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
    bool hardwareRayTracing    = false;
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

private:
    int InitializeDevice();
    int InitializeRenderResourceManager();
    int InitializeRenderPipelines();

    RenderSystemDesc m_desc;
    std::unique_ptr<IRenderDevice> m_device;
    std::shared_ptr<RenderResourceManager> m_renderResourceManager;
    std::shared_ptr<IPipeline> m_mainRenderPipeline;
};

}  // namespace Prisma::Graphic
