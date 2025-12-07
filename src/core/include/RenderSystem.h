#pragma once
#include "ManagerBase.h"
#include "RenderThread.h"
#include <memory>
#include "RenderBackend.h"

namespace Engine {

class RenderSystem : public ManagerBase<RenderSystem> {
public:
    friend class ManagerBase<RenderSystem>;

public:
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

private:
    std::unique_ptr<RenderBackend> renderBackend;
    WorkerThread renderThread;
};
}  // namespace Engine