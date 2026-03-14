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
    WindowProps WindowProperties;
};

class ENGINE_API Application {
public:
    Application(const ApplicationSpecification& spec = ApplicationSpecification());
    virtual ~Application();

    static Application& Get() { return *s_Instance; }

    // 由 Engine 调用
    void InitWindow();

    virtual int OnInitialize() = 0;
    virtual void OnShutdown() = 0;

    // 事件处理
    virtual void OnEvent(Event& e);

    // 每帧生命周期钩子
    virtual void OnUpdate(Timestep ts);
    
    /**
     * @brief 渲染提交钩子
     * 应用层应在此处通过 Renderer::Submit 提交渲染指令。
     */
    virtual void OnRender() {}

    virtual void OnImGuiRender();

    // 状态控制
    bool IsRunning() const { return m_Running; }
    void Close() { m_Running = false; }
    bool IsMinimized() const { return m_Minimized; }

    // 访问器
    Window& GetWindow() { return *m_Window; }
    LayerStack& GetLayerStack() { return m_LayerStack; }
    const ApplicationSpecification& GetSpecification() const { return m_Spec; }

    template<typename T>
    T* GetSystem();

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
};

// ============================================================================
// 客户端 (DLL) 必须定义的工厂函数
// ============================================================================
extern "C" {
    ENGINE_API Application* CreateApplication();
}

} // namespace Prisma
