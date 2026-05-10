#pragma once

#include "Export.h"
#include "core/LayerStack.h"
#include "core/Timestep.h"
#include "core/Event.h"
#include "Window.h"
#include "graphic/interfaces/RenderTypes.h"

namespace Prisma {

/**
 * @brief 应用程序配置规范
 */
struct ApplicationSpecification {
    std::string Name = "Prisma App";
    std::string EntryScene = "";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool Fullscreen = false;
    bool Resizable = true;
    Graphic::PresentMode PresentMode = Graphic::PresentMode::VSync;
    uint32_t MaxFPS = 0;
};

class ENGINE_API Application {
public:
    Application(const ApplicationSpecification& spec = ApplicationSpecification());
    virtual ~Application();

    static Application& Get() { return *s_Instance; }

    // --- Lifecycle Hooks ---
    virtual int OnInitialize()      = 0;
    virtual int OnImGuiInitialize() { return -1; }
    virtual void OnShutdown();
    virtual void OnUpdate(Timestep ts);
    virtual void OnRender();
    virtual void OnImGuiRender();
    virtual void OnEvent(Event& e);
    virtual bool ShouldCloseOnWindowClose() const { return true; }

    virtual void* GetImGuiContext() { return nullptr; }

    // --- State Control ---
    void Close() { m_Running = false; }
    bool IsRunning() const { return m_Running; }
    
    // --- Accessors ---
    const ApplicationSpecification& GetSpecification() const { return m_Spec; }
    LayerStack& GetLayerStack() { return m_LayerStack; }

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

protected:
    static Application* s_Instance;
    ApplicationSpecification m_Spec;
    LayerStack m_LayerStack;
    bool m_Running = true;
};

} // namespace Prisma
