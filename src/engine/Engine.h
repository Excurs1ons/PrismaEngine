#pragma once

#include "Export.h"
#include "ISubSystem.h"
#include "Logger.h"
#include "core/Timestep.h"
#include "Window.h"
#include <memory>
#include <vector>
#include <string>

namespace Prisma {

class Application;
class AssetManager;
class JobSystem;
namespace Input { class InputManager; }
namespace Graphic { class RenderSystem; }
class SceneManager;
class PhysicsSystem;

/**
 * @brief 引擎配置规范
 */
struct EngineSpecification {
    std::string Name = "PrismaEngine";
    bool Headless = false;
    LogLevel MinLogLevel = LogLevel::Trace;
    uint32_t MaxFPS = 0; 
};

/**
 * @brief 引擎核心类
 */
class ENGINE_API Engine {
public:
    Engine(const EngineSpecification& spec = EngineSpecification());
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    int Initialize();
    int Run(std::unique_ptr<Application> app);
    void Shutdown();

    // --- 外部驱动接口 (External Driving Interface) ---
    void BeginFrame();
    void Update(Timestep ts);
    void Render();
    void EndFrame();
    void Present();

    static Engine& Get() { return *s_Instance; }
    
    // --- Window Management ---
    Window& GetWindow() { return *m_Window; }
    bool IsMinimized() const { return m_Minimized; }

    // --- Fast Track Access ---
    Input::InputManager* GetInputManager() { return m_InputManager; }
    Graphic::RenderSystem* GetRenderSystem() { return m_RenderSystem; }
    SceneManager* GetSceneManager() { return m_SceneManager; }
    PhysicsSystem* GetPhysicsSystem() { return m_PhysicsSystem; }
    JobSystem* GetJobSystem() { return m_JobSystem; }

    const EngineSpecification& GetSpecification() const { return m_Spec; }
    bool IsRunning() const { return m_Running; }

    // 通用系统添加 (用于非核心扩展)
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        m_Systems.push_back(std::move(system));
        return ptr;
    }

private:
    EngineSpecification m_Spec;
    std::vector<std::unique_ptr<ISubSystem>> m_Systems;
    std::unique_ptr<Application> m_CurrentApp;
    std::unique_ptr<Window> m_Window;
    
    // 核心系统指针缓存 (快车道)
    AssetManager* m_AssetManager = nullptr;
    Input::InputManager* m_InputManager = nullptr;
    Graphic::RenderSystem* m_RenderSystem = nullptr;
    SceneManager* m_SceneManager = nullptr;
    PhysicsSystem* m_PhysicsSystem = nullptr;
    JobSystem* m_JobSystem = nullptr;

    bool m_Initialized = false;
    bool m_Running = false;
    bool m_Minimized = false;

    static Engine* s_Instance;
};

} // namespace Prisma
