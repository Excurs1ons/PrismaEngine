#include "Engine.h"
#include "Platform.h"
#include "Application.h"
#include "Logger.h"
#include "JobSystem.h"
#include "core/AssetManager.h"
#include "core/AssetDatabase.h"
#include "input/InputManager.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/Shader.h"
#include "SceneManager.h"
#include "PhysicsSystem.h"
#include "scripting/MonoRuntime.h"
#include "core/ECS.h"
#include <typeinfo>
#include <filesystem>
#include <glaze/glaze.hpp>
#include "threading/ThreadManager.h"
#include "app/CommandLineParser.h"
#include "scene/Scene.h"
#include "graphic/OrthographicCamera.h"

#if defined(PRISMA_ENABLE_MCP)
#include "mcp/MCPSubSystem.h"
#include "mcp/tools/SceneTools.h"
#include "mcp/tools/ECSTools.h"
#include "mcp/tools/EngineTools.h"
#include "mcp/transport/TransportTCP.h"
#endif

namespace Prisma {
    // 资产搜索路径配置结构
    struct ProjectAssetsConfig { 
        std::vector<std::string> assets; 
    };
}

// Glaze 特化 (修复语法错误，移除 invalid using enum)
template <>
struct glz::meta<Prisma::ProjectAssetsConfig> {
    static constexpr auto value = glz::object("assets", &Prisma::ProjectAssetsConfig::assets);
};

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
    
    Logger::Get().Initialize();
    Logger::Get().SetMinLevel(m_Spec.MinLogLevel);
    LOG_INFO("Engine", "Prisma 引擎正在初始化: {0}", m_Spec.Name);

    if (!Platform::IsInitialized()) {
        Platform::Initialize();
    }

    AssetDatabase::Get().Load("assets/metadata.json");
    if (m_Spec.RefreshAssetDatabaseOnStartup) {
        AssetDatabase::Get().Refresh("assets");
    }

    m_JobSystem = AddSystem<JobSystem>();
    m_AssetManager = AddSystem<AssetManager>();
    m_InputManager = AddSystem<Input::InputManager>();
    m_SceneManager = AddSystem<SceneManager>();
    m_PhysicsSystem = AddSystem<PhysicsSystem>();
    AddSystem<Graphic::ShaderLibrary>();

#if defined(PRISMA_ENABLE_MCP)
    LOG_INFO("Engine", "MCP 子系统正在初始化");
    auto& cli = CommandLineParser::Get();
    std::string transportType = "stdio";
    uint16_t tcpPort = 3100;
    if (cli.IsOptionSet("mcp-transport")) transportType = cli.GetOptionValue("mcp-transport");
    if (cli.IsOptionSet("mcp-port")) {
        auto portStr = cli.GetOptionValue("mcp-port");
        if (!portStr.empty()) tcpPort = static_cast<uint16_t>(std::stoul(portStr));
    }
    auto* mcp = AddSystem<MCP::MCPSubSystem>();
    if (transportType == "tcp") {
        LOG_INFO("MCP", "TCP transport selected (port: {})", tcpPort);
        mcp->SetTransport(std::make_unique<MCP::TransportTCP>(tcpPort));
    } else {
        LOG_INFO("MCP", "Stdio transport selected (default)");
    }
    mcp->RegisterTool<MCP::SceneHierarchyTool>(this);
    mcp->RegisterTool<MCP::SceneEntityTool>(this);
    mcp->RegisterTool<MCP::SceneCreateEntityTool>(this);
    mcp->RegisterTool<MCP::SceneDeleteEntityTool>(this);
    mcp->RegisterTool<MCP::ECSComponentListTool>(this);
    mcp->RegisterTool<MCP::ECSComponentGetTool>(this);
    mcp->RegisterTool<MCP::EngineStatusTool>(this);
    mcp->RegisterTool<MCP::EngineStateHashTool>(this);
    mcp->RegisterTool<MCP::EngineBuildInfoTool>(this);
#endif
    
    for (auto& sys : m_Systems) {
        if (sys->Initialize() != 0) {
            LOG_FATAL("Engine", "系统初始化失败！");
            return -1;
        }
    }

    // 从 project.json 读取资产搜索路径
    if (m_AssetManager) {
        std::vector<std::string> projPaths = {"project.json", "assets/project.json"};
        for (const auto& p : projPaths) {
            if (std::filesystem::exists(p)) {
                ProjectAssetsConfig pac;
                std::string buffer;
                auto err = glz::read_file_json(pac, p, buffer);
                if (!err) {
                    for (auto& ap : pac.assets) m_AssetManager->AddSearchPath(ap);
                    LOG_INFO("Engine", "从 {0} 加载了 {1} 个资产搜索路径", p, pac.assets.size());
                }
                break;
            }
        }
    }

    m_Initialized = true;
    return 0;
}

int Engine::Run(std::unique_ptr<Application> app) {
    if (!m_Initialized || !app) return -1;
    
    m_CurrentApp = std::move(app);
    m_Running = true;

    if (!m_Spec.Headless) {
        WindowProps props;
        auto& appSpec = m_CurrentApp->GetSpecification();
        props.Title = appSpec.Name;
        props.Width = appSpec.Width;
        props.Height = appSpec.Height;
        props.Resizable = appSpec.Resizable;
        props.fullScreenMode = appSpec.Fullscreen ? FullScreenMode::FullScreen : FullScreenMode::Window;

        m_Window = Window::Create(props);
        if (!m_Window) {
            LOG_FATAL("Engine", "创建窗口失败！");
            return -1;
        }

        m_CurrentApp->GetSpecification().Width  = m_Window->GetWidth();
        m_CurrentApp->GetSpecification().Height = m_Window->GetHeight();

        Graphic::RenderSystemDesc rDesc;
        rDesc.windowHandle = m_Window->GetNativeWindow();
        rDesc.width = m_Window->GetWidth();
        rDesc.height = m_Window->GetHeight();
        rDesc.enableDebug      = false;
        rDesc.presentMode      = appSpec.PresentMode;
        rDesc.enableValidation = false;
        
        m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
        if (m_RenderSystem->Initialize() != 0) {
            LOG_FATAL("Engine", "初始化渲染系统失败！");
            return -1;
        }

        if (m_RenderSystem->GetDevice()) {
            m_GPUName = m_RenderSystem->GetDevice()->GetGPUName();
        }

        m_Window->SetEventCallback([this](Event& e) {
            EventDispatcher dispatcher(e);
            
            dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& [[maybe_unused]] event) {
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
                if (m_SceneManager) {
                    auto* scene = m_SceneManager->GetCurrentScene();
                    if (scene) {
                        auto camera = scene->GetMainCamera();
                        if (camera) camera->SetViewport(event.GetWidth(), event.GetHeight());
                    }
                }
                return false;
            });

            if (m_CurrentApp) m_CurrentApp->OnEvent(e);
        });
    }

    if (m_CurrentApp->OnInitialize() != 0) return -1;

    // 场景加载后，将主相机视口同步到实际窗口尺寸
    if (m_SceneManager) {
        auto* scene = m_SceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) camera->SetViewport(m_Window->GetWidth(), m_Window->GetHeight());
        }
    }

    auto& actualAppSpec = m_CurrentApp->GetSpecification();
    if (m_Spec.MaxFPS == 0) m_Spec.MaxFPS = actualAppSpec.MaxFPS;

    double lastFrameTime = Platform::GetTimeSeconds();

    while (m_Running && m_CurrentApp->IsRunning()) {
        double time = Platform::GetTimeSeconds();
        float deltaTime = static_cast<float>(time - lastFrameTime);
        lastFrameTime = time;

        if (m_Window) m_Window->OnUpdate();
        else Platform::PumpEvents();
        
        if (!m_Running) break;

        if (m_Spec.Headless || !m_Minimized) {
            Update(Timestep(std::min(deltaTime, 0.1f)));
            if (GetRenderSystem()) {
                double t0 = Platform::GetTimeSeconds();
                GetRenderSystem()->BeginFrame();
                double t1 = Platform::GetTimeSeconds();
                m_CurrentApp->OnRender(); 
                double t2 = Platform::GetTimeSeconds();
                GetRenderSystem()->EndFrame();
                double t3 = Platform::GetTimeSeconds();
                GetRenderSystem()->Present();
                double t4 = Platform::GetTimeSeconds();

                m_FrameStats.BeginFrameTime = (t1 - t0) * 1000.0;
                m_FrameStats.RenderTime     = (t2 - t1) * 1000.0;
                m_FrameStats.EndFrameTime   = (t3 - t2) * 1000.0;
                m_FrameStats.PresentTime    = (t4 - t3) * 1000.0;
                m_FrameStats.TotalTime      = (t4 - t0) * 1000.0;

                static int s_frameCount = 0;
                static double s_fpsTimer = 0.0;
                s_frameCount++;
                double frameEnd = Platform::GetTimeSeconds();
                s_fpsTimer += frameEnd - time;
                if (s_fpsTimer >= 1.0) {
                    m_FrameStats.FPS = static_cast<float>(s_frameCount / s_fpsTimer);
                    s_frameCount = 0;
                    s_fpsTimer = 0.0;
                }
            }
        } else {
            Platform::SleepMilliseconds(10);
        }

        if (m_Spec.MaxFPS > 0) {
            float targetFrameTime = 1.0f / m_Spec.MaxFPS;
            while (Platform::GetTimeSeconds() - time < targetFrameTime) {
                if (targetFrameTime - (Platform::GetTimeSeconds() - time) > 0.002f) Platform::SleepMilliseconds(1);
            }
        }
    }

    m_CurrentApp->OnShutdown();
    if (GetRenderSystem() && GetRenderSystem()->GetDevice()) GetRenderSystem()->GetDevice()->WaitForIdle();
    m_CurrentApp.reset();
    return 0;
}

Graphic::IRenderResourceManager* Engine::GetRenderResourceManager() { return m_RenderSystem ? m_RenderSystem->GetRenderResourceManager() : nullptr; }
Scripting::MonoRuntime& Engine::GetMonoRuntime() { return Scripting::MonoRuntime::Get(); }
AssetDatabase& Engine::GetAssetDatabase() { return AssetDatabase::Get(); }
Core::ECS::World& Engine::GetWorld() { return Core::ECS::World::Get(); }
ThreadManager& Engine::GetThreadManager() { return *ThreadManager::Get(); }
CommandLineParser& Engine::GetCommandLineParser() { return CommandLineParser::Get(); }

void Engine::Update(Timestep ts) {
    for (auto& sys : m_Systems) sys->Update(ts);
    if (m_CurrentApp) m_CurrentApp->OnUpdate(ts);
}

void Engine::Shutdown() {
    if (!m_Initialized) return;
    LOG_INFO("Engine", "正在关闭引擎...");
    {
        for (auto it = m_Systems.rbegin(); it != m_Systems.rend(); ++it) {
            if (it->get() == m_RenderSystem) continue;
            (*it)->Shutdown();
        }
    }
    if (m_RenderSystem) m_RenderSystem->Shutdown();
    if (m_Window) { m_Window->Shutdown(); m_Window.reset(); }
    m_Systems.clear(); m_CurrentApp = nullptr; m_AssetManager = nullptr; m_InputManager = nullptr; m_RenderSystem = nullptr; m_SceneManager = nullptr; m_PhysicsSystem = nullptr; m_JobSystem = nullptr; m_Initialized = false; m_Running = false;
}

} // namespace Prisma
