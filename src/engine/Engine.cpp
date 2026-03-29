#include "Engine.h"
#include "Platform.h"
#include "Application.h"
#include "Logger.h"
#include "JobSystem.h"
#include "core/AssetManager.h"
#include "input/InputManager.h"
#include "graphic/RenderSystem.h"
#include "graphic/Shader.h"
#include "SceneManager.h"
#include "PhysicsSystem.h"

namespace Prisma {

Engine* Engine::s_Instance = nullptr;

Engine::Engine(const EngineSpecification& spec)
    : m_Spec(spec), m_Initialized(false), m_Running(false) {
    s_Instance = this;
}

Engine::~Engine() {
    Shutdown();
    s_Instance = nullptr;
}

int Engine::Initialize() {
    if (m_Initialized) return 0;
    
    // 基础系统先行
    Logger::Get().SetMinLevel(m_Spec.MinLogLevel);
    LOG_INFO("Engine", "Prisma Engine Initializing: {0}", m_Spec.Name);

    if (!Platform::IsInitialized()) {
        Platform::Initialize();
    }

    // 显式注册核心系统
    m_JobSystem = AddSystem<JobSystem>();
    m_AssetManager = AddSystem<AssetManager>();
    m_InputManager = AddSystem<Input::InputManager>();
    m_SceneManager = AddSystem<SceneManager>();
    m_PhysicsSystem = AddSystem<PhysicsSystem>();
    AddSystem<Graphic::ShaderLibrary>();

    // [架构调整] 渲染系统初始化
    // 如果不是纯 Headless 模式（如编辑器需要 Viewport），或者即便 Headless 但需要离屏渲染
    if (!m_Spec.Headless) {
        // 原有的窗口模式初始化逻辑移到了这里，但编辑器目前走下面的路径
    } else {
        // 在编辑器模式下，我们虽然设为 Headless（因为不想要引擎自带窗口），
        // 但我们仍然需要 RenderSystem 来支持离屏渲染。
        Graphic::RenderSystemDesc rDesc;
        rDesc.windowHandle = nullptr; // 离屏渲染不需要窗口
        rDesc.width = 1280;
        rDesc.height = 720;
        
        m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
    }
    
    // 初始化所有子系统
    for (auto& sys : m_Systems) {
        if (sys->Initialize() != 0) {
            LOG_FATAL("Engine", "System initialization failed!");
            return -1;
        }
    }

    m_Initialized = true;
    return 0;
}

void Engine::BeginFrame() {
    if (m_RenderSystem) m_RenderSystem->BeginFrame();
}

void Engine::Render() {
    if (m_CurrentApp) m_CurrentApp->OnRender();
}

void Engine::EndFrame() {
    if (m_RenderSystem) m_RenderSystem->EndFrame();
}

void Engine::Present() {
    if (m_RenderSystem) m_RenderSystem->Present();
}

void Engine::Update(Timestep ts) {
    for (auto& sys : m_Systems) sys->Update(ts);
    if (m_CurrentApp) m_CurrentApp->OnUpdate(ts);
}

int Engine::Run(std::unique_ptr<Application> app) {
    if (!m_Initialized || !app) return -1;
    
    m_CurrentApp = std::move(app);
    m_Running = true;

    // 1. 初始化窗口 (如果不是 Headless 且尚未创建)
    if (!m_Spec.Headless && !m_Window) {
        WindowProps props;
        auto& appSpec = m_CurrentApp->GetSpecification();
        props.Title = appSpec.Name;
        props.Width = appSpec.Width;
        props.Height = appSpec.Height;

        m_Window = Window::Create(props);
        if (!m_Window) {
            LOG_FATAL("Engine", "Failed to create window!");
            return -1;
        }

        // 如果在 Run 阶段创建了窗口，我们需要重新配置或创建一个带窗口的 RenderSystem
        // 但目前主线逻辑已移至独立 Editor，这里保持向后兼容
        if (!m_RenderSystem) {
            Graphic::RenderSystemDesc rDesc;
            rDesc.windowHandle = m_Window->GetNativeWindow();
            rDesc.width = m_Window->GetWidth();
            rDesc.height = m_Window->GetHeight();
            m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
            m_RenderSystem->Initialize();
        }

        // 2. 窗口事件统一分发
        m_Window->SetEventCallback([this](Event& e) {
            EventDispatcher dispatcher(e);
            dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) {
                m_Running = false;
                return true;
            });
            dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
                if (event.GetWidth() == 0 || event.GetHeight() == 0) {
                    m_Minimized = true;
                    return false;
                }
                m_Minimized = false;
                if (m_RenderSystem) m_RenderSystem->Resize(event.GetWidth(), event.GetHeight());
                return false;
            });
            if (m_CurrentApp) m_CurrentApp->OnEvent(e);
        });
    }

    if (m_CurrentApp->OnInitialize() != 0) return -1;

    double lastFrameTime = Platform::GetTimeSeconds();

    while (m_Running && m_CurrentApp->IsRunning()) {
        double time = Platform::GetTimeSeconds();
        float deltaTime = static_cast<float>(time - lastFrameTime);
        lastFrameTime = time;

        if (m_Window) m_Window->OnUpdate();
        
        if (!m_Running) break;

        if (m_Spec.Headless || !m_Minimized) {
            Update(Timestep(std::min(deltaTime, 0.1f)));
            if (GetRenderSystem()) {
                BeginFrame();
                Render(); 
                EndFrame();
                Present();
            }
        } else {
            Platform::SleepMilliseconds(10);
        }

        if (m_Spec.MaxFPS > 0) {
            float targetFrameTime = 1.0f / m_Spec.MaxFPS;
            while (Platform::GetTimeSeconds() - time < targetFrameTime) {
                if (targetFrameTime - (Platform::GetTimeSeconds() - time) > 0.002f) {
                    Platform::SleepMilliseconds(1);
                }
            }
        }
    }

    m_CurrentApp->OnShutdown();
    if (GetRenderSystem() && GetRenderSystem()->GetDevice()) {
        GetRenderSystem()->GetDevice()->WaitForIdle();
    }
    m_CurrentApp.reset();
    return 0;
}

void Engine::Shutdown() {
    if (!m_Initialized) return;
    LOG_INFO("Engine", "Shutting down engine...");
    for (auto it = m_Systems.rbegin(); it != m_Systems.rend(); ++it) {
        if (it->get() == m_RenderSystem) continue;
        (*it)->Shutdown();
    }
    if (m_RenderSystem) m_RenderSystem->Shutdown();
    m_Systems.clear();
    m_CurrentApp = nullptr;
    m_AssetManager = nullptr;
    m_InputManager = nullptr;
    m_RenderSystem = nullptr;
    m_SceneManager = nullptr;
    m_PhysicsSystem = nullptr;
    m_JobSystem = nullptr;
    m_Initialized = false;
    m_Running = false;
}

} // namespace Prisma
