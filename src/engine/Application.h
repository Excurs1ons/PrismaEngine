#pragma once

#include "Export.h"
#include "core/LayerStack.h"
#include "core/Timestep.h"
#include "core/Event.h"
#include "Window.h"

namespace Prisma {

/**
 * @brief 应用程序配置规范
 */
struct ApplicationSpecification {
    std::string Name = "Prisma App";
    uint32_t Width = 1280;
    uint32_t Height = 720;
};

class ENGINE_API Application {
public:
    Application(const ApplicationSpecification& spec = ApplicationSpecification());
    virtual ~Application();

    static Application& Get() { return *s_Instance; }

    bool Initialize();
    void InitWindow();
    void Run();
    void Close();

    virtual int OnInitialize() = 0;
    virtual void OnShutdown() = 0;

    // 事件处理
    virtual void OnEvent(Event& e);

    // 每帧生命周期钩子
    virtual void OnUpdate(Timestep ts);
    virtual void OnRender();
    virtual void OnImGuiRender();

    // 状态控制
    bool IsRunning() const { return m_Running; }
    bool IsMinimized() const { return m_Minimized; }

    // 访问器
    Window& GetWindow() { return *m_Window; }
    LayerStack& GetLayerStack() { return m_LayerStack; }
    const ApplicationSpecification& GetSpecification() const { return m_Spec; }

    // Layer 管理
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

protected:
    static Application* s_Instance;
    ApplicationSpecification m_Spec;
    std::unique_ptr<Window> m_Window;
    LayerStack m_LayerStack;
    bool m_Running = true;
    bool m_Minimized = false;
    float m_LastFrameTime = 0.0f;
};

// ============================================================================
// 客户端 (DLL) 必须定义的工厂函数
// ============================================================================
extern "C" {
    ENGINE_API Application* CreateApplication();
}

} // namespace Prisma
