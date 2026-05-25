#pragma once

#include "Export.h"
#include "ISubSystem.h"
#include "Logger.h"
#include "core/Timestep.h"
#include "Window.h"
#include "graphic/interfaces/RenderTypes.h"
#include <memory>
#include <vector>
#include <string>

class CommandLineParser;

namespace Audio { class IAudioDevice; struct AudioDesc; }

namespace Prisma {

class Application;
class AssetManager;
class JobSystem;
class AssetDatabase;
namespace Input { class InputManager; }
namespace Graphic { class RenderSystem; class IRenderResourceManager; }
namespace Scripting { class CoreCLRHost; class ScriptEngine; class MonoRuntime; }
namespace Core::ECS { class World; }
class SceneManager;
class PhysicsSystem;
class ThreadManager;
class EntityManager;

/**
 * @brief 引擎配置规范
 */
struct EngineSpecification {
    const char* Name = "Prisma Engine";
    bool Headless = false;
    // Runtime/游戏默认只读资源元数据库，避免每次启动改写 assets/metadata.json
    bool RefreshAssetDatabaseOnStartup = false;
    LogLevel MinLogLevel = LogLevel::Trace;
    uint32_t MaxFPS = 0; 
    Graphic::PresentMode PresentMode = Graphic::PresentMode::VSync;

    // CLI 覆盖字段（0/空 = 使用 project.jsonc 值）
    uint32_t HeadlessFrames = 0;
    uint32_t HeadlessWidth = 0;
    uint32_t HeadlessHeight = 0;
    std::string HeadlessOutputPath;
    uint32_t MaxSamples = 0;
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

    static Engine& Get();
    
    struct FrameStats {
        double BeginFrameTime  = 0.0;
        double RenderTime      = 0.0;
        double EndFrameTime    = 0.0;
        double PresentTime     = 0.0;
        double TotalTime       = 0.0;
        float  FPS             = 0.0f;
    };

    const FrameStats& GetFrameStats() const { return m_FrameStats; }
    float GetFPS() const { return m_FrameStats.FPS; }
    const std::string& GetGPUName() const;
    
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
#if PRISMA_ENABLE_SCRIPTING > 0
    Scripting::MonoRuntime& GetMonoRuntime();
    Scripting::CoreCLRHost& GetCoreCLRHost() { return *m_coreCLRHost; }
    Scripting::ScriptEngine& GetScriptEngine() { return *m_scriptEngine; }
#endif
    EntityManager& GetEntityManager() { return *m_entityManager; }
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

    const EngineSpecification& GetSpecification() const;
    bool IsRunning() const { return m_Running; }

    /**
     * @brief 提交一个函数到主线程执行 (线程安全)
     */
    void SubmitToMainThread(std::function<void()>&& func);

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
    void ExecuteMainThreadQueue();
    
    EngineSpecification m_Spec;
    std::vector<std::unique_ptr<ISubSystem>> m_Systems;
    std::unique_ptr<Application> m_CurrentApp;
    std::unique_ptr<Window> m_Window;

    // 线程安全队列
    std::vector<std::function<void()>> m_MainThreadQueue;
    std::mutex m_MainThreadQueueMutex;
    
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

    FrameStats m_FrameStats;
    std::string m_GPUName;

#if PRISMA_ENABLE_SCRIPTING > 0
    // C# 脚本系统（unique_ptr 避免头文件包含膨胀）
    std::unique_ptr<Scripting::CoreCLRHost> m_coreCLRHost;
    std::unique_ptr<Scripting::ScriptEngine> m_scriptEngine;
#endif
    std::unique_ptr<EntityManager> m_entityManager;

    // 音频设备 (optional, 通过 IAudioDevice 接口)
    std::unique_ptr<Audio::IAudioDevice> m_audioDevice;

    static Engine* s_Instance;
};

} // namespace Prisma
