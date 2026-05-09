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
#include "threading/ThreadManager.h"
#include "app/CommandLineParser.h"

#if defined(PRISMA_ENABLE_MCP)
#include "mcp/MCPSubSystem.h"
#include "mcp/tools/SceneTools.h"
#include "mcp/tools/ECSTools.h"
#include "mcp/tools/EngineTools.h"
#include "mcp/transport/TransportTCP.h"
#endif

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
    Logger::Get().Initialize();
    Logger::Get().SetMinLevel(m_Spec.MinLogLevel);
    LOG_INFO("Engine", "Prisma 引擎正在初始化: {0}", m_Spec.Name);

    if (!Platform::IsInitialized()) {
        Platform::Initialize();
    }

    // 初始化全局资源数据库
    AssetDatabase::Get().Load("assets/metadata.json");
    if (m_Spec.RefreshAssetDatabaseOnStartup) {
        AssetDatabase::Get().Refresh("assets");
    }

    // 显式注册核心系统
    m_JobSystem = AddSystem<JobSystem>();
    m_AssetManager = AddSystem<AssetManager>();
    m_InputManager = AddSystem<Input::InputManager>();
    m_SceneManager = AddSystem<SceneManager>();
    m_PhysicsSystem = AddSystem<PhysicsSystem>();
    
    // 新增：ShaderLibrary 子系统
    AddSystem<Graphic::ShaderLibrary>();

#if defined(PRISMA_ENABLE_MCP)
    LOG_INFO("Engine", "MCP 子系统正在初始化");

    // 读取命令行配置 MCP 传输
    auto& cli = CommandLineParser::Get();
    std::string transportType = "stdio";
    uint16_t tcpPort = 3100;

    if (cli.IsOptionSet("mcp-transport")) {
        transportType = cli.GetOptionValue("mcp-transport");
    }
    if (cli.IsOptionSet("mcp-port")) {
        auto portStr = cli.GetOptionValue("mcp-port");
        if (!portStr.empty()) tcpPort = static_cast<uint16_t>(std::stoul(portStr));
    }

    // 创建 MCP 子系统并设置传输
    auto* mcp = AddSystem<MCP::MCPSubSystem>();

    if (transportType == "tcp") {
        LOG_INFO("MCP", "TCP transport selected (port: {})", tcpPort);
        mcp->SetTransport(std::make_unique<MCP::TransportTCP>(tcpPort));
    } else {
        LOG_INFO("MCP", "Stdio transport selected (default)");
    }

    // 注册引擎核心工具
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
    
    // 初始化所有子系统
    for (auto& sys : m_Systems) {
        if (sys->Initialize() != 0) {
            LOG_FATAL("Engine", "系统初始化失败！");
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
            LOG_FATAL("Engine", "创建窗口失败！");
            return -1;
        }

        Graphic::RenderSystemDesc rDesc;
        rDesc.windowHandle = m_Window->GetNativeWindow();
        rDesc.width = m_Window->GetWidth();
        rDesc.height = m_Window->GetHeight();
        // === 引擎级开关（后续可改为配置文件） ===
        rDesc.enableDebug      = false;
        rDesc.enableVSync      = false;
        rDesc.enableValidation = false;  // true=开 Vulkan 验证层(大幅降低性能)
        
        m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
        if (m_RenderSystem->Initialize() != 0) {
            LOG_FATAL("Engine", "初始化渲染系统失败！");
            return -1;
        }

        // 2. 窗口事件统一分发 (Engine 级处理)
        m_Window->SetEventCallback([this](Event& e) {
            EventDispatcher dispatcher(e);
            
            dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) {
                if (m_CurrentApp && !m_CurrentApp->ShouldCloseOnWindowClose()) {
                    LOG_INFO("Engine", "应用拒绝关闭窗口请求");
                    return false;
                }
                LOG_INFO("Engine", "收到关闭窗口请求，设置 m_Running=false");
                m_Running = false;
                return true;
            });

            dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
                if (event.GetWidth() == 0 || event.GetHeight() == 0) {
                    LOG_INFO("Engine", "窗口最小化: {0}x{1}", event.GetWidth(), event.GetHeight());
                    m_Minimized = true;
                    return false;
                }
                LOG_INFO("Engine", "窗口大小调整为: {0}x{1}", event.GetWidth(), event.GetHeight());
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
        } else {
            Platform::PumpEvents();
            if (Platform::ShouldClose(nullptr)) {
                LOG_INFO("Engine", "平台层收到关闭请求");
                m_Running = false;
            }
        }
        
        if (!m_Running) break;

        // B. 逻辑与渲染更新
        if (m_Spec.Headless || !m_Minimized) {
            // 1. 逻辑更新 (Subsystems & App)
            Update(Timestep(std::min(deltaTime, 0.1f)));
            
            // 2. 渲染流程
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

                // 每 5 秒打印一次单帧各阶段耗时
                static double lastFrameLog = 0.0;
                double now = Platform::GetTimeSeconds();
                if (now - lastFrameLog >= 5.0) {
                    LOG_INFO("Engine", "帧耗时 BF={:.2f}ms Render={:.2f}ms EF={:.2f}ms Present={:.2f}ms Total={:.2f}ms",
                        (t1 - t0) * 1000.0, (t2 - t1) * 1000.0, (t3 - t2) * 1000.0, (t4 - t3) * 1000.0,
                        (t4 - t0) * 1000.0);
                    lastFrameLog = now;
                }
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

    LOG_INFO("Engine", "应用 OnShutdown...");
    m_CurrentApp->OnShutdown();
    LOG_INFO("Engine", "应用 OnShutdown 完成");
    
    // 确保在销毁应用程序（及其中资源，如被析构的 VulkanBuffer/Texture）之前，GPU 已完成所有工作。
    // 这可以防止触发 VUID-vkDestroyBuffer-buffer-00922 等验证错误。
    LOG_INFO("Engine", "等待 GPU 空闲...");
    if (GetRenderSystem() && GetRenderSystem()->GetDevice()) {
        GetRenderSystem()->GetDevice()->WaitForIdle();
    }
    LOG_INFO("Engine", "GPU 已空闲");

    // Application/Layer 可能仍持有 ITexture/IBuffer 等 GPU 资源。
    // 必须在 RenderSystem/VMA allocator 关闭前释放它们，避免资源晚于 allocator 析构。
    LOG_INFO("Engine", "销毁应用 (释放 GPU 资源)...");
    m_CurrentApp.reset();
    LOG_INFO("Engine", "应用已销毁，Run() 即将返回");

    return 0;
}

Graphic::IRenderResourceManager* Engine::GetRenderResourceManager() {
    return m_RenderSystem ? m_RenderSystem->GetRenderResourceManager() : nullptr;
}

Scripting::MonoRuntime& Engine::GetMonoRuntime() {
    return Scripting::MonoRuntime::Get();
}

AssetDatabase& Engine::GetAssetDatabase() {
    return AssetDatabase::Get();
}

Core::ECS::World& Engine::GetWorld() {
    return Core::ECS::World::Get();
}

ThreadManager& Engine::GetThreadManager() {
    return *ThreadManager::Get();
}

CommandLineParser& Engine::GetCommandLineParser() {
    return CommandLineParser::Get();
}

void Engine::Update(Timestep ts) {
    for (auto& sys : m_Systems) sys->Update(ts);
    if (m_CurrentApp) m_CurrentApp->OnUpdate(ts);
}

void Engine::Shutdown() {
    if (!m_Initialized) return;
    
    LOG_INFO("Engine", "正在关闭引擎...");

    // RenderSystem 必须最后关闭。
    // 其他系统、场景对象以及 Application/Layer 可能还持有 GPU 资源，
    // 若先销毁 VMA allocator，会在这些资源稍后析构时触发未释放断言。
    {
        int idx = 0;
        for (auto it = m_Systems.rbegin(); it != m_Systems.rend(); ++it) {
            const char* name = typeid(**it).name();
            if (it->get() == m_RenderSystem) {
                LOG_INFO("Engine", "跳过 RenderSystem ({}) [{}]", idx, name);
                continue;
            }
            LOG_INFO("Engine", "关闭子系统 #{} [{}]...", idx, name);
            (*it)->Shutdown();
            LOG_INFO("Engine", "子系统 #{} [{}] 已关闭", idx, name);
            ++idx;
        }
    }

    LOG_INFO("Engine", "正在关闭渲染系统...");
    if (m_RenderSystem) {
        m_RenderSystem->Shutdown();
    }
    LOG_INFO("Engine", "渲染系统已关闭");

    // 显式销毁渲染窗口，避免 SDL_DestroyWindow 推迟到 Engine 析构
    // 在 Vulkan 下，若 VMA/Device 已 shutdown 后仍持有窗口，窗口关闭可能无响应
    LOG_INFO("Engine", "正在销毁窗口...");
    if (m_Window) {
        m_Window->Shutdown();
        m_Window.reset();
    }
    LOG_INFO("Engine", "窗口已销毁");

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
