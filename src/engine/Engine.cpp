#include "Engine.h"
#include "Platform.h"
#include "Application.h"
#include "Logger.h"
#include "core/AssetManager.h"
#include "input/InputManager.h"
#include "graphic/RenderSystem.h"

namespace Prisma {

Engine* Engine::s_Instance = nullptr;

Engine::Engine(const EngineSpecification& spec)
    : m_Spec(spec), m_Running(false), m_Initialized(false) {
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
    m_AssetManager = AddSystem<AssetManager>();
    m_InputManager = AddSystem<Input::InputManager>();
    
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
        m_CurrentApp->InitWindow();

        Graphic::RenderSystemDesc rDesc;
        auto& window = m_CurrentApp->GetWindow();
        rDesc.windowHandle = window.GetNativeWindow();
        rDesc.width = window.GetWidth();
        rDesc.height = window.GetHeight();
        
        m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
        if (m_RenderSystem->Initialize() != 0) {
            LOG_FATAL("Engine", "Failed to initialize RenderSystem!");
            return -1;
        }
    }

    if (m_CurrentApp->OnInitialize() != 0) return -1;

    // 2. 窗口事件统一分发
    m_CurrentApp->GetWindow().SetEventCallback([this](Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
            if (m_RenderSystem) m_RenderSystem->Resize(event.GetWidth(), event.GetHeight());
            return false;
        });

        if (m_InputManager) m_InputManager->OnEvent(e);
        m_CurrentApp->OnEvent(e);
    });

    double lastFrameTime = Platform::GetTimeSeconds();

    // --- 核心主循环 ---
    while (m_Running && m_CurrentApp->IsRunning()) {
        double time = Platform::GetTimeSeconds();
        float deltaTime = static_cast<float>(time - lastFrameTime);
        lastFrameTime = time;

        // A. 事件泵送
        m_CurrentApp->GetWindow().OnUpdate();
        if (!m_Running) break;

        // B. 逻辑与渲染更新
        if (m_Spec.Headless || !m_CurrentApp->IsMinimized()) {
            // 1. 逻辑更新 (Subsystems & App)
            Update(Timestep(std::min(deltaTime, 0.1f)));
            
            // 2. 渲染流程 (交给 RenderSystem 统筹)
            if (m_RenderSystem) {
                m_RenderSystem->BeginFrame();

                // 提交阶段：让应用层把东西扔进渲染队列
                // 注意：这里不再手动凑 RenderContext，由 RenderSystem 或 App 内部处理
                m_CurrentApp->OnRender(); 

                // UI 渲染 (ImGui)
                m_CurrentApp->OnImGuiRender();

                m_RenderSystem->EndFrame();
            }
        } else {
            Platform::SleepMilliseconds(10);
        }

        // C. FPS 帧同步控制 (Linus 批准版)
        if (m_Spec.MaxFPS > 0) {
            float targetFrameTime = 1.0f / m_Spec.MaxFPS;
            while (Platform::GetTimeSeconds() - time < targetFrameTime) {
                // 剩余时间较多时可以 Sleep 释放 CPU
                if (targetFrameTime - (Platform::GetTimeSeconds() - time) > 0.002f) {
                    Platform::SleepMilliseconds(1);
                }
            }
        }
    }

    m_CurrentApp->OnShutdown();
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
    m_Initialized = false;
    m_Running = false;
}

} // namespace Prisma
