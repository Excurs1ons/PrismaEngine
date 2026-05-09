#pragma once

#include "Export.h"
#include "ISubSystem.h"
#include "Logger.h"
#include "core/Timestep.h"
#include "Window.h"
#include <memory>
#include <vector>
#include <string>

class CommandLineParser;

namespace Prisma {

class Application;
class AssetManager;
class JobSystem;
class AssetDatabase;
namespace Input { class InputManager; }
namespace Graphic { class RenderSystem; class IRenderResourceManager; }
namespace Scripting { class MonoRuntime; }
namespace Core::ECS { class World; }
class SceneManager;
class PhysicsSystem;
class ThreadManager;

/**
 * @brief 引擎配置规范
 */
struct EngineSpecification {
    std::string Name = "PrismaEngine";
    bool Headless = false;
    // Runtime/游戏默认只读资源元数据库，避免每次启动改写 assets/metadata.json
    bool RefreshAssetDatabaseOnStartup = false;
    LogLevel MinLogLevel = LogLevel::Trace;
    uint32_t MaxFPS = 0; 
    bool EnableVSync = false; // 默认关闭垂直同步，允许高帧率
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

    static Engine& Get() { return *s_Instance; }
    
    // --- Window Management ---
    Window& GetWindow() { return *m_Window; }
    bool IsMinimized() const { return m_Minimized; }

    // --- Fast Track Access ---
    AssetManager* GetAssetManager() { return m_AssetManager; }
    Input::InputManager* GetInputManager() { return m_InputManager; }
    Graphic::RenderSystem* GetRenderSystem() { return m_RenderSystem; }
    Graphic::IRenderResourceManager* GetRenderResourceManager();
    SceneManager* GetSceneManager() { return m_SceneManager; }
    PhysicsSystem* GetPhysicsSystem() { return m_PhysicsSystem; }
    JobSystem* GetJobSystem() { return m_JobSystem; }
    Scripting::MonoRuntime& GetMonoRuntime();
    Core::ECS::World& GetWorld();
    ThreadManager& GetThreadManager();
    CommandLineParser& GetCommandLineParser();
    
    // 通用系统获取
    template<typename T>
    T* GetSystem() {
        for (auto& system : m_Systems) {
            T* ptr = dynamic_cast<T*>(system.get());
            if (ptr) return ptr;
        }
        return nullptr;
    }
    
    // --- Global Managers ---
    Logger& GetLogger() { return Logger::Get(); }
    AssetDatabase& GetAssetDatabase();

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
    void Update(Timestep ts);
    
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
