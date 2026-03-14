#pragma once

#include "../Export.h"
#include "../ISubSystem.h" // 继承自这个
#include "interfaces/IPipeline.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceManager.h"
#include "interfaces/RenderTypes.h"
#include <functional>
#include <memory>
#include <string>

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
    uint32_t maxFramesInFlight = 3;
    std::string name           = "PrismaApp";
};

/**
 * @brief 渲染系统 (子系统)
 * 没有任何单例，由 Engine 拥有。
 */
class ENGINE_API RenderSystem : public ISubSystem {
public:
    RenderSystem(const RenderSystemDesc& desc);
    ~RenderSystem() override;

    // ISubSystem 接口
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    // === 帧控制 ===
    void BeginFrame();
    void EndFrame();
    void Present();
    void Resize(uint32_t width, uint32_t height);

    // === 设备访问 ===
    IRenderDevice* GetDevice() const { return m_device.get(); }
    IResourceManager* GetResourceManager() const { return m_resourceManager.get(); }

    // === 渲染流程 ===
    void SetMainPipeline(std::shared_ptr<IPipeline> pipeline);
    IPipeline* GetMainPipeline() const { return m_mainPipeline.get(); }

    // === ImGui ===
    bool InitializeImGui();
    void ShutdownImGui();
    using GuiRenderCallback = std::function<void(IRenderDevice*)>;
    void SetGuiRenderCallback(GuiRenderCallback callback);

private:
    bool InitializeDevice();
    bool InitializeResourceManager();
    bool InitializePipelines();

    RenderSystemDesc m_desc;
    std::unique_ptr<IRenderDevice> m_device;
    std::unique_ptr<IResourceManager> m_resourceManager;
    std::shared_ptr<IPipeline> m_mainPipeline;
    
    bool m_imguiInitialized = false;
    GuiRenderCallback m_guiCallback;
};

}  // namespace Prisma::Graphic
