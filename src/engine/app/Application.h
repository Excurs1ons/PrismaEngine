#pragma once

#include "Export.h"

// GAME_API: 各游戏/应用项目对外导出的统一符号。
// 与 PRISMA_PLUGIN_API 等价（总是 dllexport/visibility("default")），
// 但语义明确表明这是"游戏项目"的入口，而非"引擎插件"。
#if defined(_MSC_VER)
    #define GAME_API __declspec(dllexport)
#else
    #define GAME_API __attribute__((visibility("default")))
#endif
#include <string>
#include "core/LayerStack.h"
#include "core/Timestep.h"
#include "core/Event.h"
#include "Window.h"
#include "graphic/interfaces/RenderTypes.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"

namespace Prisma {

// 应用程序配置规范
struct ApplicationSpecification {
    std::string Name = "Prisma App";
    std::string EntryScene = "";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool Fullscreen = false;
    bool Resizable = true;
    Graphic::PresentMode PresentMode = Graphic::PresentMode::VSync;
    uint32_t MaxFPS = 0;
    uint32_t MaxSamples = 512;
    uint32_t MaxBounces = 4;
    bool HardwareRayTracing = false;
    Graphic::PathTraceMode PathTraceMode = Graphic::PathTraceMode::BVH;
    bool EnableNEE = false;
    uint32_t HeadlessFrames = 500;
    uint32_t HeadlessWidth = 1080;
    uint32_t HeadlessHeight = 1080;
    std::string HeadlessOutputPath = "pt_output.png";
    std::vector<std::string> Scenes;  // 可选多场景列表（F6/F7 切换，放末尾避免破坏聚合初始化）
};

class ENGINE_API Application {
public:
    Application(const ApplicationSpecification& spec = ApplicationSpecification());
    virtual ~Application();

    static Application& Get();

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
    const ApplicationSpecification& GetSpecification() const;
    ApplicationSpecification& GetSpecification();
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
