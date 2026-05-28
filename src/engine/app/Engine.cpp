#include "Engine.h"
#include "Platform.h"
#include "Application.h"
#include "Logger.h"
#include "JobSystem.h"
#include "core/AssetManager.h"
#include "core/AssetDatabase.h"
#include "core/EntityManager.h"
#include "input/InputManager.h"
#include "graphic/RenderSystem.h"

extern "C" { char* SDL_GetBasePath(void); void SDL_free(void* ptr); }
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/Shader.h"
#include "graphic/Renderer2D.h"
#include "SceneManager.h"
#include "PhysicsSystem.h"
#include "core/ECS.h"
#include "scripting/MonoRuntime.h"
#include "scripting/CoreCLRHost.h"
#include "scripting/ScriptEngine.h"
#include "memory/MemorySystem.h"
#include "audio/AudioAPI.h"
#include "audio/AudioTypes.h"
#include <cassert>
#include <typeinfo>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <glaze/glaze.hpp>
#include "app/ProjectConfig.h"
#include "threading/ThreadManager.h"
#include "app/CommandLineParser.h"
#include "scene/Scene.h"
#include "console/ConsoleSystem.h"
#include "console/ConsoleUI.h"
#include "profiling/ProfilerSystem.h"
#include "animation/AnimationSystem.h"
#include "particles/ParticleSystem.h"
#include "terrain/TerrainSystem.h"
#include "navigation/NavigationSystem.h"
#include "ai/AISystem.h"
#include "water/WaterSystem.h"
#include "network/NetworkSystem.h"
#include "localization/LocalizationSystem.h"







namespace Prisma {

Engine* Engine::s_Instance = nullptr;

Engine& Engine::Get() {
    return *s_Instance;
}

Engine::Engine(const EngineSpecification& spec)
    : m_Spec(spec), m_Initialized(false), m_Running(false) {
    s_Instance = this;
    m_entityManager = std::make_unique<EntityManager>();
#if PRISMA_ENABLE_SCRIPTING > 0
    m_coreCLRHost = std::make_unique<Scripting::CoreCLRHost>();
    m_scriptEngine = std::make_unique<Scripting::ScriptEngine>();
#endif
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

    m_MemorySystem = AddSystem<Memory::MemorySystem>();
    m_AnimationSystem = AddSystem<Animation::AnimationSystem>();
    m_JobSystem = AddSystem<JobSystem>();
    m_AssetManager = AddSystem<AssetManager>();
    m_InputManager = AddSystem<Input::InputManager>();
    m_SceneManager = AddSystem<SceneManager>();
    m_PhysicsSystem = AddSystem<PhysicsSystem>();
    AddSystem<ConsoleSystem>();
    AddSystem<Graphic::ShaderLibrary>();
    AddSystem<Particles::ParticleSystem>();
    m_TerrainSystem = AddSystem<Terrain::TerrainSystem>();
    m_AISystem = AddSystem<AI::AISystem>();
    m_WaterSystem = AddSystem<Water::WaterSystem>();
    m_NavigationSystem = AddSystem<Navigation::NavigationSystem>();
    m_NetworkSystem = AddSystem<Network::NetworkSystem>();
    m_LocalizationSystem = AddSystem<Localization::LocalizationSystem>();

    for (auto& sys : m_Systems) {
        if (sys->Initialize() != 0) {
            LOG_FATAL("Engine", "系统初始化失败！");
            return -1;
        }
    }

    // 音频子系统初始化 (非致命 — 允许无音频运行)
    {
        std::string cliDisable;
        if (m_Spec.Headless) {
            LOG_INFO("Engine", "无头模式: 跳过音频初始化");
        } else {
            Audio::AudioDesc audioDesc;
            audioDesc.deviceType = Audio::AudioDeviceType::SDL3;
            audioDesc.outputFormat = Audio::AudioFormat(48000, 2, 32);
            audioDesc.bufferSize = 256;
            audioDesc.enableEffects = true;
            m_audioDevice = Audio::AudioAPI::CreateDevice(audioDesc.deviceType, audioDesc);
            if (m_audioDevice && m_audioDevice->Initialize(audioDesc)) {
                LOG_INFO("Engine", "音频系统已初始化 ({}), 设备: {}",
                         Audio::AudioAPI::GetDeviceName(audioDesc.deviceType),
                         m_audioDevice->GetDeviceInfo().name);
            } else {
                LOG_WARNING("Engine", "音频初始化失败, 将继续运行 (静音模式)");
                m_audioDevice.reset();
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

    // 将控制台 UI 层添加到应用层栈
    if (auto* console = GetSystem<ConsoleSystem>()) {
        if (auto* ui = console->GetConsoleUI()) {
            m_CurrentApp->PushOverlay(ui);
        }
    }

    // 从项目名称对应的文件读取配置
    auto scriptingBackend = ScriptingBackend::CoreCLR;
    auto renderMode = RenderMode::Mode3D_Forward;
    {
        auto& spec = m_CurrentApp->GetSpecification();
        std::string projName = m_Spec.Name;
        
        std::vector<std::string> projPaths = {
            projName + ".jsonc",
            projName + ".json",
            "assets/" + projName + ".jsonc",
            "assets/" + projName + ".json",
            "project.jsonc", // 回退兼容
            "assets/project.jsonc"
        };
        for (const auto& p : projPaths) {
            // 手动读取文件（可处理 BOM）
            std::ifstream fileStream(p, std::ios::binary | std::ios::ate);
            if (!fileStream) continue;
            std::streamsize sz = fileStream.tellg();
            fileStream.seekg(0);
            std::string buf(static_cast<size_t>(sz), '\0');
            fileStream.read(buf.data(), buf.size());

            // 跳过 UTF-8 BOM (EF BB BF) — 使用 unsigned char 避免 sign 扩展问题
            if (buf.size() >= 3) {
                const auto* uBuf = reinterpret_cast<const unsigned char*>(buf.data());
                if (uBuf[0] == 0xEF && uBuf[1] == 0xBB && uBuf[2] == 0xBF) {
                    buf.erase(0, 3);
                }
            }

            ProjectConfig config;
            auto err = glz::read_jsonc(config, buf);
            if (!err) {
                spec.Name               = config.name;
                spec.EntryScene         = config.entryScene;
                spec.Width              = config.window.width;
                spec.Height             = config.window.height;
                spec.Fullscreen         = config.window.fullscreen;
                spec.Resizable          = config.window.resizable;
                spec.PresentMode        = config.window.vsync;
                spec.MaxFPS             = config.window.maxFPS;
                spec.MaxSamples         = config.rendering.maxSamples;
                spec.MaxBounces         = config.rendering.maxBounces;
                spec.HardwareRayTracing = config.rendering.hardwareRayTracing;
                spec.PathTraceMode      = config.rendering.pathTraceMode;
                spec.EnableNEE          = config.rendering.enableNEE;
                spec.HeadlessFrames     = config.headless.frames;
                spec.HeadlessWidth      = config.headless.width;
                spec.HeadlessHeight     = config.headless.height;
                spec.HeadlessOutputPath = config.headless.outputPath;

                // CLI args 覆盖 project.jsonc 默认值（0/空 = 不覆盖）
                if (m_Spec.HeadlessFrames)    spec.HeadlessFrames     = m_Spec.HeadlessFrames;
                if (m_Spec.HeadlessWidth)     spec.HeadlessWidth      = m_Spec.HeadlessWidth;
                if (m_Spec.HeadlessHeight)    spec.HeadlessHeight     = m_Spec.HeadlessHeight;
                if (!m_Spec.HeadlessOutputPath.empty()) spec.HeadlessOutputPath = m_Spec.HeadlessOutputPath;
                if (m_Spec.MaxSamples)        spec.MaxSamples         = m_Spec.MaxSamples;

                spec.Scenes = config.scenes;

                if (m_AssetManager) {
                    for (auto& ap : config.assets)
                        m_AssetManager->AddSearchPath(ap);
                }
                scriptingBackend = config.scriptingBackend;
                renderMode       = config.renderMode;
                LOG_INFO("Engine",
                         "项目配置已加载: {0} ({1}x{2}), 渲染模式: {3}, 脚本后端: {4}",
                         config.name,
                         config.window.width,
                         config.window.height,
                         renderMode == RenderMode::SRP                  ? "SRP"
                         : renderMode == RenderMode::Mode2D             ? "2D"
                         : renderMode == RenderMode::Mode3D_PathTracing ? "PathTracing"
                                                                        : "3D",
                         scriptingBackend == ScriptingBackend::Off    ? "Off"
                         : scriptingBackend == ScriptingBackend::Mono ? "Mono"
                                                                      : "CoreCLR");
            } else {
                LOG_ERROR("Engine", "项目配置文件解析失败: {} (错误: {})", p, glz::format_error(err, buf));
            }
            break;
        }
    }

    LOG_INFO("Engine", "[诊断] renderMode={}, 是否PathTracing={}",
             static_cast<int>(renderMode),
             renderMode == RenderMode::Mode3D_PathTracing ? "是" : "否");

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
        rDesc.renderMode       = renderMode;
        LOG_INFO("Engine", "渲染模式: {}",
                 renderMode == RenderMode::Mode3D_PathTracing ? "PathTracing" :
                 renderMode == RenderMode::Mode3D_Forward ? "Forward" :
                 renderMode == RenderMode::Mode2D ? "2D" : "Other");
        rDesc.maxSamples         = appSpec.MaxSamples;
        rDesc.maxBounces         = appSpec.MaxBounces;
        rDesc.hardwareRayTracing = appSpec.HardwareRayTracing;
        rDesc.enableValidation   = false;
        
        m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
        if (m_RenderSystem->Initialize() != 0) {
            LOG_FATAL("Engine", "初始化渲染系统失败！");
            return -1;
        }

        if (m_RenderSystem->GetDevice()) {
            m_GPUName = m_RenderSystem->GetDevice()->GetGPUName();
        }

#if PRISMA_ENABLE_SCRIPTING > 0
        // 初始化 C# 脚本引擎（根据项目设置决定）
        if (scriptingBackend == ScriptingBackend::CoreCLR) {
            // Host 运行时搜索路径（PrismaEngine.Host 自包含发布目录）
            std::string hostDir;

            // 优先使用 CMake 编译时嵌入的路径（精确、不受 CWD 影响）
#ifdef PRISMA_HOST_DIR
            if (std::filesystem::exists(PRISMA_HOST_DIR "/PrismaEngine.Host.runtimeconfig.json")) {
                hostDir = PRISMA_HOST_DIR;
            }
#endif

            // 回退：运行时相对路径搜索（兼容手动运行等场景）
            if (hostDir.empty()) {
                std::vector<std::string> hostPaths = {
                    ".",
                    "scripts",
                    "../scripts",
                    "host",
                    "../host",
                    "Host/linux-x64/publish",
                    "Host/linux-arm64/publish",
                    "Host/win-x64/publish",
                    "../build/PrismaEngine.Host/linux-x64/publish",
                    "../build/PrismaEngine.Host/linux-arm64/publish",
                    "../build/PrismaEngine.Host/win-x64/publish",
                    "../../PrismaEngine.Host/linux-x64/publish",
                    "../../PrismaEngine.Host/linux-arm64/publish",
                    "../../PrismaEngine.Host/win-x64/publish",
                    "PrismaEngine.Host/linux-x64/publish",
                    "PrismaEngine.Host/linux-arm64/publish",
                    "PrismaEngine.Host/win-x64/publish",
                };
                for (const auto& p : hostPaths) {
                    if (std::filesystem::exists(p + "/PrismaEngine.Host.runtimeconfig.json")) {
                        hostDir = std::filesystem::canonical(p).string();
                        break;
                    }
                }
            }

            // 游戏 DLL 搜索：基于 exe 目录（不依赖 CWD）+ 项目名动态推导 + 硬编码回退
            std::vector<std::string> gamePaths;

            // 0. exe 所在目录（SDL_GetBasePath 跨平台，CMake post-build 复制 DLL 到此处）
            char* sdlBase = SDL_GetBasePath();
            std::string exeDir = sdlBase ? sdlBase : ".";
            SDL_free(sdlBase);
            while (!exeDir.empty() && (exeDir.back() == '/' || exeDir.back() == '\\'))
                exeDir.pop_back();
            gamePaths.push_back(exeDir);
            gamePaths.push_back(exeDir + "/scripts");
            LOG_INFO("Engine", "Exe dir: {0}", exeDir);

            // 0b. CoreCLR host 发布目录（DLL 放这里可避免 AppContext 隔离问题）
            if (!hostDir.empty()) {
                gamePaths.push_back(hostDir);
            }

            // 1. 从 Application Name 动态推导项目源码构建输出路径
            if (m_CurrentApp) {
                std::string projName = m_CurrentApp->GetSpecification().Name;
                if (!projName.empty() && projName != "Prisma App") {
                    gamePaths.push_back(
                        "../projects/" + projName + "/scripts/GameScripts/bin/Release/net10.0");
                    LOG_INFO("Engine", "DLL search: project path for '{0}'", projName);
                }
            }

            // 2. 硬编码回退路径
            for (auto& p : std::vector<std::string>{
                     ".", "scripts", "../scripts",
                     "../projects/Prisma2D/scripts/GameScripts/bin/Release/net10.0",
                     "../projects/PrismaCraft/scripts/GameScripts/bin/Release/net10.0",
                     "../projects/SRP2D/scripts/GameScripts/bin/Release/net10.0",
                 }) {
                gamePaths.push_back(p);
            }

            std::string gameDir;
            std::string gameDll;
            for (const auto& p : gamePaths) {
                if (!std::filesystem::exists(p)) {
                    LOG_DEBUG("Engine", "  DLL path skip (not exist): {0}", p);
                    continue;
                }
                for (const auto& entry : std::filesystem::directory_iterator(p)) {
                    auto name = entry.path().filename().string();
                    if (name.ends_with("_Managed.dll")) {
                        gameDll = name;
                        gameDir = std::filesystem::canonical(p).string();
                        break;
                    }
                }
                if (!gameDir.empty()) {
                    LOG_INFO("Engine", "  Found game DLL: {0}/{1}", gameDir, gameDll);
                    break;
                }
            }

            if (!hostDir.empty()) {
                if (m_coreCLRHost->Initialize(hostDir)) {
                    if (m_scriptEngine->Initialize(*m_coreCLRHost, gameDir)) {
                        LOG_INFO("Engine", "C# 脚本系统已启动 (CoreCLR)");
                    }
                }
            } else {
                LOG_WARNING("Engine", "未找到 PrismaEngine.Host 发布目录（先执行 dotnet publish PrismaEngine.Host --self-contained）");
            }
        } else if (scriptingBackend == ScriptingBackend::Mono) {
#if PRISMA_ENABLE_MONO
            // TODO: Mono 运行时初始化
            LOG_INFO("Engine", "Mono 脚本后端将在后续版本实现");
#else
            LOG_WARNING("Engine", "项目配置为 Mono 后端，但引擎编译时未启用 Mono 支持");
#endif
        } else {
            LOG_INFO("Engine", "C# 脚本已关闭（项目配置）");
        }
#endif

        m_Window->SetEventCallback([this](Event& e) {
            EventDispatcher dispatcher(e);
            
            dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent&) {
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
                if (m_CurrentApp) {
                    m_CurrentApp->GetSpecification().Width  = event.GetWidth();
                    m_CurrentApp->GetSpecification().Height = event.GetHeight();
                }
                return false;
            });

            if (m_CurrentApp) m_CurrentApp->OnEvent(e);
        });
    } else {
        // 无头模式：创建无窗口渲染系统
        auto& appSpec = m_CurrentApp->GetSpecification();
        Graphic::RenderSystemDesc rDesc;
        rDesc.windowHandle         = nullptr;
        rDesc.width                = appSpec.Width;
        rDesc.height               = appSpec.Height;
        rDesc.enableDebug          = false;
        rDesc.enableValidation     = false;
        rDesc.headless             = true;
        rDesc.renderMode           = renderMode;
        rDesc.presentMode          = appSpec.PresentMode;

        m_RenderSystem = AddSystem<Graphic::RenderSystem>(rDesc);
        if (m_RenderSystem->Initialize() != 0) {
            LOG_FATAL("Engine", "无头渲染系统初始化失败！");
            return -1;
        }

        if (m_RenderSystem->GetDevice()) {
            m_GPUName = m_RenderSystem->GetDevice()->GetGPUName();
        }

        // 性能分析子系统（在渲染系统就绪后初始化）
        m_ProfilerSystem = AddSystem<Profiling::ProfilerSystem>();
        if (m_ProfilerSystem->Initialize() != 0) {
            LOG_WARNING("Engine", "性能分析系统初始化失败，继续运行");
            m_ProfilerSystem = nullptr;
        }
    }

    // 自动加载入口场景
    if (m_SceneManager) {
        auto& spec = m_CurrentApp->GetSpecification();
        if (!spec.EntryScene.empty()) {
            m_SceneManager->LoadFromFile(spec.EntryScene);
        }
        // 相机视口同步到实际窗口尺寸
        auto* scene = m_SceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera && m_Window) camera->SetViewport(m_Window->GetWidth(), m_Window->GetHeight());
        }
        // 通知主渲染管线场景已加载（自动构建几何数据）
        if (scene && m_RenderSystem) {
            auto pipeline = m_RenderSystem->GetMainPipeline();
            if (pipeline) pipeline->OnSceneLoaded(scene);
        }
    }

    // [修复] 所有初始化数据（场景 + C# Bootstrap）已写入 Write (layout A)，
    // 交换一次使 Read 获得完整数据。
    // 若不交换：第一帧 World::Step 的 SyncActiveBuffers 从空 Read 覆盖 Write，
    // 导致场景方块位置丢失（永远在 0,0）。
    EntityManager::Get().SwapBuffers();

    if (m_CurrentApp->OnInitialize() != 0) return -1;

    auto& actualAppSpec = m_CurrentApp->GetSpecification();
    if (m_Spec.MaxFPS == 0) m_Spec.MaxFPS = actualAppSpec.MaxFPS;

    double lastFrameTime = Platform::GetTimeSeconds();

    while (m_Running && m_CurrentApp->IsRunning()) {
        if (m_Window) m_Window->OnUpdate();
        else {
            Platform::PumpEvents();
            // 无头模式下手动限制帧率，防止 CPU 空转
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        double time = Platform::GetTimeSeconds();
        float deltaTime = static_cast<float>(time - lastFrameTime);
        lastFrameTime = time;
        if (deltaTime > 0.0f) m_FrameStats.FPS = 1.0f / deltaTime;
        
        if (!m_Running) break;

        if (m_Spec.Headless || !m_Minimized) {
            Update(Timestep(std::min(deltaTime, 0.1f)));

            // 音频更新 (非阻塞设备轮询)
            if (m_audioDevice) m_audioDevice->Update(std::min(deltaTime, 0.1f));

            // C# 脚本更新（在 App OnUpdate 之后、渲染之前）
#if PRISMA_ENABLE_SCRIPTING > 0
            if (m_scriptEngine->IsInitialized())
                m_scriptEngine->Update(std::min(deltaTime, 0.1f));
#endif

            // [正确双缓冲] C++ 是唯一的交换权威。
            // C# 每帧通过 GetTransformA(Write)/GetTransformB(Read) 重新查询指针，
            // C++ 在这里交换，使下一帧 C# 拿到正确的 Read/Write。
#if PRISMA_ENABLE_SCRIPTING > 0
            EntityManager::Get().SwapBuffers();
#endif

            if (GetRenderSystem()) {
                double t0 = Platform::GetTimeSeconds();
                GetRenderSystem()->BeginFrame();
                double t1 = Platform::GetTimeSeconds();

#if PRISMA_ENABLE_SCRIPTING > 0
                if (m_scriptEngine->IsInitialized())
                    m_scriptEngine->Render(std::min(deltaTime, 0.1f));
#endif

                Graphic::Renderer2D::BeginGizmo();
                m_CurrentApp->OnRender(); 
                Graphic::Renderer2D::EndGizmo();
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
#if PRISMA_ENABLE_SCRIPTING > 0
Scripting::MonoRuntime& Engine::GetMonoRuntime() { return Scripting::MonoRuntime::Get(); }
#endif
AssetDatabase& Engine::GetAssetDatabase() { return AssetDatabase::Get(); }
Core::ECS::World& Engine::GetWorld() { return Core::ECS::World::Get(); }
Scene& Engine::GetScene() {
    auto* scene = m_SceneManager ? m_SceneManager->GetCurrentScene() : nullptr;
    assert(scene && "No active scene. Ensure a scene is loaded before calling GetScene().");
    return *scene;
}
ThreadManager& Engine::GetThreadManager() { return *ThreadManager::Get(); }
CommandLineParser& Engine::GetCommandLineParser() { return CommandLineParser::Get(); }
ConsoleSystem* Engine::GetConsoleSystem() { return GetSystem<ConsoleSystem>(); }
Particles::ParticleSystem* Engine::GetParticleSystem() { return GetSystem<Particles::ParticleSystem>(); }
Terrain::TerrainSystem* Engine::GetTerrainSystem() { return GetSystem<Terrain::TerrainSystem>(); }
Water::WaterSystem* Engine::GetWaterSystem() { return GetSystem<Water::WaterSystem>(); }

const std::string& Engine::GetGPUName() const { return m_GPUName; }
const EngineSpecification& Engine::GetSpecification() const { return m_Spec; }

void Engine::Update(Timestep ts) {
    ExecuteMainThreadQueue();
    for (auto& sys : m_Systems) sys->Update(ts);
    if (m_CurrentApp) m_CurrentApp->OnUpdate(ts);
}

void Engine::SubmitToMainThread(std::function<void()>&& func) {
    std::lock_guard<std::mutex> lock(m_MainThreadQueueMutex);
    m_MainThreadQueue.emplace_back(std::move(func));
}

void Engine::ExecuteMainThreadQueue() {
    std::vector<std::function<void()>> queue;
    {
        std::lock_guard<std::mutex> lock(m_MainThreadQueueMutex);
        queue = std::move(m_MainThreadQueue);
        m_MainThreadQueue.clear();
    }

    for (auto& func : queue) {
        func();
    }
}

void Engine::Shutdown() {
    if (!m_Initialized) return;
    LOG_INFO("Engine", "正在关闭引擎...");

    // 先关脚本引擎（C# 世界析构 + CoreCLR 卸载），再关 C++ 系统
    if (m_scriptEngine) m_scriptEngine->Shutdown();
    if (m_coreCLRHost) m_coreCLRHost->Shutdown();

    {
        for (auto it = m_Systems.rbegin(); it != m_Systems.rend(); ++it) {
            if (it->get() == m_RenderSystem) continue;
            (*it)->Shutdown();
        }
    }
#if PRISMA_ENABLE_SCRIPTING > 0
    // SRPGraphicsAPI（C# SRP）持有 GPU 资源（PSO、着色器等）
    // 必须在渲染系统关闭之前释放，避免 DLL 静态析构时
    // VulkanPipelineState 析构访问已销毁的 VkDevice 句柄导致崩溃
    Scripting::SRPGraphicsAPI::Get().Shutdown();
#endif
    if (m_audioDevice) { m_audioDevice->Shutdown(); m_audioDevice.reset(); }
    if (m_RenderSystem) m_RenderSystem->Shutdown();
    if (m_Window) { m_Window->Shutdown(); m_Window.reset(); }
    m_Systems.clear(); m_CurrentApp = nullptr; m_AssetManager = nullptr; m_InputManager = nullptr; m_RenderSystem = nullptr; m_SceneManager = nullptr; m_PhysicsSystem = nullptr; m_JobSystem = nullptr; m_audioDevice = nullptr; m_Initialized = false; m_Running = false;
}

} // namespace Prisma
