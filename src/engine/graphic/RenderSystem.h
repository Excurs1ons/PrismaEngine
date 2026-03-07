#pragma once

#include "../Export.h"
#include "../ManagerBase.h"
#include "WorkerThread.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceManager.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace PrismaEngine::Graphic {

class ForwardPipeline;

/// @brief 渲染系统描述
struct RenderSystemDesc {
    RenderAPIType backendType = RenderAPIType::Vulkan;
    void* windowHandle = nullptr;
    uint32_t width = 1600;
    uint32_t height = 900;
    std::string name = "Prisma Engine";
    bool enableDebug = true;
    bool enableValidation = true;
    uint32_t maxFramesInFlight = 3;
};

/// @brief 渲染系统实现
class ENGINE_API RenderSystem : public ManagerBase<RenderSystem> {
public:
    RenderSystem() = default;
    ~RenderSystem();

    bool Initialize() override;
    bool Initialize(const RenderSystemDesc& desc);
    void Shutdown() override;
    void Update(float deltaTime) override;

    // ImGui
    bool InitializeImGui();
    void ShutdownImGui();

    // 帧管理
    void BeginFrame();
    void EndFrame();
    void Present();
    void RenderFrame();

    // 设置
    void Resize(uint32_t width, uint32_t height);
    void SetMainPipeline(std::shared_ptr<ForwardPipeline> pipeline);
    
    using GuiRenderCallback = std::function<void()>;
    void SetGuiRenderCallback(GuiRenderCallback callback);

    // 统计
    struct RenderStats {
        float frameTime = 0.0f;
        float fps = 0.0f;
    };
    RenderStats GetRenderStats() const;
    void ResetStats();

private:
    bool InitializeDevice(const RenderSystemDesc& desc);
    bool InitializeResourceManager();
    bool InitializePipelines();
    void UpdateStats(float deltaTime);

    RenderSystemDesc m_desc;
    std::unique_ptr<IRenderDevice> m_device;
    
    std::shared_ptr<ForwardPipeline> m_mainPipeline;
    std::shared_ptr<ForwardPipeline> m_forwardPipeline;
    std::unique_ptr<IResourceManager> m_resourceManager;

    WorkerThread m_renderThread;
    RenderStats m_stats;
    GuiRenderCallback m_guiCallback;
};

} // namespace PrismaEngine::Graphic