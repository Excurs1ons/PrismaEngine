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
#include <imgui.h>

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
    
    // 新增：ShaderLibrary 子系统
    AddSystem<Graphic::ShaderLibrary>();
    
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

int Engine::Run(std::unique_ptr<Application> app) {
    if (!m_Initialized || !app) return -1;
    
    m_CurrentApp = std::move(app);
    m_Running = true;

    // 1. 初始化窗口与渲染系统 (非 Headless)
    if (!m_Spec.Headless) {
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

        Graphic::RenderSystemDesc rDesc;
        rDesc.windowHandle = m_Window->GetNativeWindow();
        rDesc.width = m_Window->GetWidth();
        rDesc.height = m_Window->GetHeight();
        
        m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
        if (m_RenderSystem->Initialize() != 0) {
            LOG_FATAL("Engine", "Failed to initialize RenderSystem!");
            return -1;
        }

        // 2. 窗口事件统一分发 (Engine 级处理)
        m_Window->SetEventCallback([this](Event& e) {
            EventDispatcher dispatcher(e);
            
            dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) {
                LOG_INFO("Engine", "Window close requested (Event: {0})", event.GetName());
                m_Running = false;
                return true;
            });

            dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
                if (event.GetWidth() == 0 || event.GetHeight() == 0) {
                    LOG_INFO("Engine", "Window minimized: {0}x{1}", event.GetWidth(), event.GetHeight());
                    m_Minimized = true;
                    return false;
                }
                LOG_INFO("Engine", "Window resized to {0}x{1}", event.GetWidth(), event.GetHeight());
                m_Minimized = false;
                if (m_RenderSystem) m_RenderSystem->Resize(event.GetWidth(), event.GetHeight());
                return false;
            });

            if (m_CurrentApp) m_CurrentApp->OnEvent(e);
        });
    }

    if (m_CurrentApp->OnInitialize() != 0) return -1;

    double lastFrameTime = Platform::GetTimeSeconds();

    // --- 核心主循环 ---
    while (m_Running && m_CurrentApp->IsRunning()) {
        double time = Platform::GetTimeSeconds();
        float deltaTime = static_cast<float>(time - lastFrameTime);
        lastFrameTime = time;

        // A. 事件泵送 (如果是非 Headless 模式)
        if (m_Window) {
            m_Window->OnUpdate();
        }
        
        if (!m_Running) break;

        // B. 逻辑与渲染更新
        if (m_Spec.Headless || !m_Minimized) {
            // 1. 逻辑更新 (Subsystems & App)
            Update(Timestep(std::min(deltaTime, 0.1f)));
            
            // 2. 渲染流程
            if (GetRenderSystem()) {
                // 同步 ImGui 上下文 (跨 DLL 必备)
                if (auto ctx = (ImGuiContext*)m_CurrentApp->GetImGuiContext()) {
                    ImGui::SetCurrentContext(ctx);
                }

                GetRenderSystem()->BeginFrame();
                m_CurrentApp->OnRender(); 
                m_CurrentApp->OnImGuiRender();
                GetRenderSystem()->EndFrame();
                GetRenderSystem()->Present();
            }
        } else {
            Platform::SleepMilliseconds(10);
        }

        // C. FPS 帧同步控制
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
    
    // 确保在销毁应用程序（及其中资源，如被析构的 VulkanBuffer/Texture）之前，GPU 已完成所有工作。
    // 这可以防止触发 VUID-vkDestroyBuffer-buffer-00922 等验证错误。
    if (GetRenderSystem() && GetRenderSystem()->GetDevice()) {
        GetRenderSystem()->GetDevice()->WaitForIdle();
    }

    return 0;
}

void Engine::Update(Timestep ts) {
    for (auto& sys : m_Systems) sys->Update(ts);
    if (m_CurrentApp) m_CurrentApp->OnUpdate(ts);
}

void Engine::Shutdown() {
    if (!m_Initialized) return;
    
    LOG_INFO("Engine", "Shutting down engine...");
    for (auto it = m_Systems.rbegin(); it != m_Systems.rend(); ++it) (*it)->Shutdown();
    m_Systems.clear();
    
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
