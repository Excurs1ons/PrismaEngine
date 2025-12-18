#pragma once
#include "ManagerBase.h"
#include "RenderBackend.h"
#include "WorkerThread.h"
#include "ScriptableRenderPipeline.h"
#include "pipelines/forward/ForwardPipeline.h"
#include "RenderSystemNew.h"
#include <memory>
#include <functional>

namespace Engine {

class RenderSystem : public ManagerBase<RenderSystem> {
public:
    friend class ManagerBase<RenderSystem>;
    static constexpr std::string GetName() { return R"(RenderSystem)"; }

    // 添加带参数的初始化方法
    bool Initialize(
        Platform* platform, RenderBackendType renderBackendType, WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height);
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    // GUI 渲染回调
    using GuiRenderCallback = std::function<void(void*)>;
    void SetGuiRenderCallback(GuiRenderCallback callback);

    // 渲染流程控制
    void BeginFrame();
    void EndFrame();
    void Present();
    void Resize(uint32_t width, uint32_t height);

    RenderBackend* GetRenderBackend() const { return renderBackend.get(); }
    ScriptableRenderPipeline* GetRenderPipe() const { return renderPipe.get(); }

private:
    std::unique_ptr<RenderBackend> renderBackend;
    std::unique_ptr<ScriptableRenderPipeline> renderPipe;
    std::unique_ptr<Graphic::Pipelines::Forward::ForwardPipeline> forwardPipeline;
    WorkerThread renderThread;

    // 渲染任务函数
    std::function<void()> m_renderTask;

    // 适配器（内部使用新接口）
    class Adapter {
    public:
        Adapter(RenderSystem* renderSystem);
        bool Initialize(Platform* platform, RenderBackendType renderBackendType,
                       WindowHandle windowHandle, void* surface, uint32_t width, uint32_t height);
        void Shutdown();
        void Update(float deltaTime);
        void BeginFrame();
        void EndFrame();
        void Present();
        void Resize(uint32_t width, uint32_t height);
        void SetGuiRenderCallback(GuiRenderCallback callback);
    private:
        RenderSystem* m_renderSystem;
        PrismaEngine::Graphic::RenderSystemNew m_newRenderSystem;
    };
    std::unique_ptr<Adapter> m_adapter;

    // 在渲染线程中执行
    void RenderFrame();

};
}  // namespace Engine