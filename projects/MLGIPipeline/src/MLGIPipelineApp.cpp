#include "MLGIPipelineApp.h"
#include "StatsOverlay.h"
#include "HeadlessRunner.h"
#include "MLGISystem.h"

#include "graphic/RenderSystem.h"
#include "graphic/Renderer2D.h"
#include "graphic/PrimitiveComponent.h"
#include "graphic/MeshRenderer.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "app/Engine.h"
#include "input/InputManager.h"
#include "platform/Platform.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "Logger.h"

#include <vector>
#include <string>
#include <format>
#include <algorithm>

namespace Prisma {
using namespace Graphic;

// ============================================================================
// MLGIPipelineApp
// ============================================================================

MLGIPipelineApp::MLGIPipelineApp()
    : Application()
{
}

MLGIPipelineApp::~MLGIPipelineApp() = default;

int MLGIPipelineApp::OnInitialize() {
    LOG_INFO("MLGIPipeline", "3D 模板初始化（路径追踪引擎管线版）");

    m_ptPipeline = Engine::Get().GetRenderSystem()->GetMainPipelineAs<Graphic::PathTracingPipeline>();
    m_renderSystem = Engine::Get().GetRenderSystem();
    if (!m_ptPipeline) {
        // 光线追踪不可用（设备不支持），回退到 Forward 模式
        LOG_WARN("MLGIPipeline", "路径追踪管线不可用，设备不支持光线追踪，回退到 Forward 渲染模式");
        m_fallbackMode = true;
    }

    // ---- MLGI capability detection ----
    if (!m_fallbackMode) {
        m_rayQuerySupported = CheckRayQuerySupport();
        m_tlasAvailable = CheckTLASAvailability();
        m_mlgiEnabled = ValidateProbeConfig();

        if (!m_mlgiEnabled) {
            LOG_WARN("MLGIPipeline", "MLGI 已禁用 — 基线 Forward 渲染将继续");
        }
    } else {
        // Fallback mode: no ray tracing at all, MLGI cannot function
        m_mlgiEnabled = false;
        m_rayQuerySupported = false;
        m_tlasAvailable = false;
        LOG_WARN("MLGIPipeline", "回退模式: MLGI 因缺乏光线追踪支持而禁用");
    }

    auto* sceneManager = Engine::Get().GetSceneManager();
    if (sceneManager) {
        m_scene = sceneManager->GetCurrentScene();
    }

    // Default to Primitive (disable all MeshRenderer)
    if (m_scene) {
        for (const auto& node : m_scene->GetNodes()) {
            if (auto meshR = m_scene->GetComponent<Graphic::MeshRenderer>(node))
                meshR->SetEnabled(false);
        }
    }

    // Read pipeline params from spec (CLI or project.jsonc, merged by Engine::Run)
    m_ptMaxSamples = m_Spec.MaxSamples;
    if (m_ptPipeline) {
        m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
        m_ptPipeline->SetMode(m_Spec.PathTraceMode);
        m_enableNEE = m_Spec.EnableNEE;
        m_ptPipeline->EnableNEE(m_enableNEE);
    }

    // 注册渲染模式切换回调
    if (m_renderSystem) {
        m_renderSystem->SetRenderModeChangedCallback([this](Prisma::RenderMode mode, Prisma::Graphic::IRenderDevice* device) {
            (void)device;
            m_currentRenderMode = mode;
            if (mode == Prisma::RenderMode::Mode3D_PathTracing) {
                // 重新获取管线指针（设备重建后旧指针失效）
                m_ptPipeline = Engine::Get().GetRenderSystem()->GetMainPipelineAs<Prisma::Graphic::PathTracingPipeline>();
                if (m_ptPipeline && m_scene) {
                    m_ptPipeline->OnSceneLoaded(m_scene);
                    m_ptPipeline->ResetAccumulation();
                }
            }
            LOG_INFO("MLGIPipeline", "渲染模式切换: {}", 
                     mode == Prisma::RenderMode::Mode3D_PathTracing ? "PathTracing" : "Forward");
        });
    }

    // Store scene info for hot-reload (F5 reload, F6/F7 cycle)
    m_scenePath = m_Spec.EntryScene;
    m_sceneList = m_Spec.Scenes;
    if (!m_sceneList.empty()) {
        // Find current scene index in the list
        auto it = std::find(m_sceneList.begin(), m_sceneList.end(), m_scenePath);
        m_currentSceneIndex = (it != m_sceneList.end())
            ? static_cast<int>(std::distance(m_sceneList.begin(), it))
            : 0;
    }

    // Create StatsOverlay component
    if (m_scene && m_ptPipeline) {
        auto overlayNode = m_scene->CreateNode("StatsOverlay");
        m_statsOverlay = std::make_unique<StatsOverlay>(m_Spec, m_ptPipeline.get(),
                                                        m_enableNEE, m_usePrimitiveSphere,
                                                        m_sampleSource.get(), m_mlgiSystem.get());
        m_statsOverlay->SetOwnerNode(overlayNode, m_scene);
    }

    // Create HeadlessRunner (m_Spec has CLI overrides applied by Engine::Run)
    bool headless = Engine::Get().GetSpecification().Headless;
    if (headless && m_ptPipeline) {
        m_Spec.Width  = m_Spec.HeadlessWidth;
        m_Spec.Height = m_Spec.HeadlessHeight;
        LOG_INFO("MLGIPipeline", "headless模式分辨率: {}x{} (frames={}, output={})",
                 m_Spec.Width, m_Spec.Height, m_Spec.HeadlessFrames, m_Spec.HeadlessOutputPath);
        m_headlessRunner = std::make_unique<HeadlessRunner>(
            headless, m_Spec.HeadlessFrames, m_Spec.HeadlessOutputPath, m_ptPipeline.get(),
            m_mlgiSystem.get());
    }

    m_sampleSource = std::make_unique<SampleSource>(m_sampleSeed, m_probeSampleCount, m_sampleMode);
    m_sampleSource->Generate();
    LOG_INFO("MLGIPipeline", "Sample source: {}", m_sampleSource->ToString());

    m_mlgiConfig.enableMLGI          = m_Spec.enableMLGI;
    m_mlgiConfig.probeGridDim.x      = m_Spec.probeGridDimX;
    m_mlgiConfig.probeGridDim.y      = m_Spec.probeGridDimY;
    m_mlgiConfig.probeGridDim.z      = m_Spec.probeGridDimZ;
    m_mlgiConfig.probeSpacing        = m_Spec.probeSpacing;
    m_mlgiConfig.raysPerProbe        = m_Spec.raysPerProbe;
    m_mlgiConfig.temporalBlendFactor = m_Spec.temporalBlendFactor;
    m_mlgiConfig.debugDumpPath       = m_Spec.debugDumpPath;

    m_mlgiSystem = std::make_unique<MLGISystem>();
    if (m_mlgiEnabled) {
        m_mlgiSystem->Initialize(m_mlgiConfig, m_renderSystem);
        if (!m_mlgiSystem->IsEnabled()) {
            LOG_WARN("MLGIPipeline", "MLGI disabled after initialization (device or config issue)");
            m_mlgiEnabled = false;
        } else {
            LOG_INFO("MLGIPipeline", "MLGI probe GI system enabled");
        }
    } else {
        LOG_INFO("MLGIPipeline", "MLGI disabled by config, system not initialized");
    }

    // Apply initial screen dimensions from spec so screen gather pass
    // matches the actual window size rather than the default 1920x1080.
    if (m_mlgiSystem && m_mlgiSystem->IsReady()) {
        m_mlgiSystem->OnResize(m_Spec.Width, m_Spec.Height);
    }

    LOG_INFO("MLGIPipeline", "R 重置累积，B 切换模式，P 切换 Primitive|Mesh，N 切换 NEE，[/] 调整采样帧数，F5 重载场景，F6/F7 切换场景");
    return 0;
}

// ============================================================================
// MLGI capability detection
// ============================================================================

bool MLGIPipelineApp::CheckRayQuerySupport() {
    auto* device = m_renderSystem ? m_renderSystem->GetDevice() : nullptr;
    if (!device) {
        LOG_WARN("MLGIPipeline", "RayQuery 不支持: 无渲染设备可用");
        m_mlgiEnabled = false;
        return false;
    }

    if (!device->IsRayQuerySupported()) {
        LOG_WARN("MLGIPipeline", "RayQuery 不支持: 设备不支持 VK_KHR_ray_query, MLGI 已禁用");
        m_mlgiEnabled = false;
        return false;
    }

    LOG_INFO("MLGIPipeline", "RayQuery 支持: 设备支持 VK_KHR_ray_query");
    return true;
}

bool MLGIPipelineApp::CheckTLASAvailability() {
    auto* device = m_renderSystem ? m_renderSystem->GetDevice() : nullptr;
    if (!device) {
        LOG_WARN("MLGIPipeline", "TLAS 不可用: 无渲染设备，跳过探针更新");
        return false;
    }

    if (!device->IsRayTracingSupported()) {
        LOG_WARN("MLGIPipeline", "TLAS 不可用: 设备不支持加速结构，跳过探针更新");
        return false;
    }

    LOG_INFO("MLGIPipeline", "TLAS 可用: 加速结构支持已确认");
    return true;
}

bool MLGIPipelineApp::ValidateProbeConfig() {
    if (m_probeGrid.probeCount == 0) {
        LOG_WARN("MLGIPipeline", "探针网格配置无效: probeCount=0, MLGI 已禁用");
        m_mlgiEnabled = false;
        return false;
    }

    if (m_probeGrid.probeCount > ProbeGrid::kMaxProbes) {
        LOG_WARN("MLGIPipeline", "探针网格配置已裁剪: {} 探针 > 最大值 {}, 已限制到 {}",
                 m_probeGrid.probeCount, ProbeGrid::kMaxProbes,
                 std::min(m_probeGrid.probeCount, ProbeGrid::kMaxProbes));
        m_probeGrid.probeCount = std::min(m_probeGrid.probeCount, ProbeGrid::kMaxProbes);
    }

    if (m_probeGrid.dimensions.x == 0 || m_probeGrid.dimensions.y == 0 || m_probeGrid.dimensions.z == 0) {
        LOG_WARN("MLGIPipeline", "探针网格配置无效: 维度为零 ({}x{}x{}), MLGI 已禁用",
                 m_probeGrid.dimensions.x, m_probeGrid.dimensions.y, m_probeGrid.dimensions.z);
        m_mlgiEnabled = false;
        return false;
    }

    if (m_probeGrid.spacing.x <= 0.0f || m_probeGrid.spacing.y <= 0.0f || m_probeGrid.spacing.z <= 0.0f) {
        LOG_WARN("MLGIPipeline", "探针网格间距无效 ({}x{}x{}), 回退到默认间距 1.0",
                 m_probeGrid.spacing.x, m_probeGrid.spacing.y, m_probeGrid.spacing.z);
        m_probeGrid.spacing = glm::vec3(1.0f);
    }

    m_probeGrid.LogConfig();
    LOG_INFO("MLGIPipeline", "探针网格配置验证通过");
    return true;
}

void MLGIPipelineApp::LoadScene(const std::string& path) {
    auto* sceneManager = Engine::Get().GetSceneManager();
    if (!sceneManager) return;

    if (!sceneManager->LoadFromFile(path)) {
        LOG_ERROR("MLGIPipeline", "场景加载失败: {}", path);
        return;
    }

    m_scene = sceneManager->GetCurrentScene();
    m_scenePath = path;

    // Default to Primitive (disable all MeshRenderer)
    if (m_scene) {
        for (const auto& node : m_scene->GetNodes()) {
            if (auto meshR = m_scene->GetComponent<Graphic::MeshRenderer>(node))
                meshR->SetEnabled(false);
        }
    }

    if (m_ptPipeline && m_scene) {
        m_ptPipeline->OnSceneLoaded(m_scene);
        m_ptPipeline->ResetAccumulation();
    }

    if (m_mlgiSystem && m_mlgiSystem->IsReady()) {
        m_mlgiSystem->OnSceneReload();
    }

    LOG_INFO("MLGIPipeline", "场景已切换: {}", path);
}

void MLGIPipelineApp::OnShutdown() {
    if (m_mlgiSystem) {
        m_mlgiSystem->Shutdown();
    }
    Application::OnShutdown();
}

void MLGIPipelineApp::OnRender() {
    // StatsOverlay renders via Renderer2D::DrawString, which requires
    // being inside BeginGizmo()/EndGizmo() (set up by Engine::Run main loop).
    // The Update() call writes to gizmo command queue consumed by
    // PathTracingPipeline::RenderOverlay(). If called too early (e.g. OnUpdate),
    // the text lands in the scene command queue and is never displayed.
    if (m_statsOverlay) {
        m_statsOverlay->SetNEEEnabled(m_enableNEE);
        m_statsOverlay->SetUsePrimitiveSphere(m_usePrimitiveSphere);
        m_statsOverlay->Update(Timestep{});
    }

    // Forward fallback: just clear and present, no path tracing
    if (m_fallbackMode) {
        // In fallback mode, the Forward pipeline handles rendering automatically
        return;
    }

    // MLGI disabled guard: skip all MLGI allocation/dispatch
    if (!m_mlgiEnabled || !m_mlgiSystem || !m_mlgiSystem->IsReady()) {
        return;
    }

    if (!m_tlasAvailable) {
        LOG_WARN("MLGIPipeline", "TLAS 不可用: 跳过探针更新调度");
        return;
    }

    // MLGI per-frame update: advances temporal frame, blends SH data
    m_mlgiSystem->Update(Timestep{});
}

void MLGIPipelineApp::OnUpdate(Timestep ts) {
    // Auto-rebuild PT data when scene is dirty (triggered by P key etc.)
    if (m_scene && m_scene->IsDirty()) {
        if (m_ptPipeline) {
            m_ptPipeline->ReloadSceneData();
        }
        m_scene->SetDirty(false);
    }

    // MLGI per-frame update (disabled guard inside MLGISystem::Update)
    if (m_mlgiSystem && m_mlgiEnabled) {
        m_mlgiSystem->Update(ts);
    }

    if (m_headlessRunner && m_headlessRunner->Update()) {
        Close();
    }
}

void MLGIPipelineApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        if (m_mlgiSystem && m_mlgiSystem->IsReady()) {
            m_mlgiSystem->OnResize(ev.GetWidth(), ev.GetHeight());
        }
        return false;
    });
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        auto key = static_cast<Input::KeyCode>(ev.GetKeyCode());
        bool repeat = ev.IsRepeat();

        if (key == Input::KeyCode::Escape) {
            Close();
            return true;
        }
        if (key == Input::KeyCode::P && !repeat) {
            m_usePrimitiveSphere = !m_usePrimitiveSphere;
            if (m_scene) {
                for (const auto& node : m_scene->GetNodes()) {
                    if (auto prim = m_scene->GetComponent<Graphic::PrimitiveComponent>(node))
                        prim->SetEnabled(m_usePrimitiveSphere);
                    if (auto meshR = m_scene->GetComponent<Graphic::MeshRenderer>(node))
                        meshR->SetEnabled(!m_usePrimitiveSphere);
                }
                m_scene->SetDirty(true);
                LOG_INFO("MLGIPipeline", "全局模式 → {}", m_usePrimitiveSphere ? "Primitive (原生)" : "Mesh (三角化)");
            }
            return true;
        }
        if (key == Input::KeyCode::R && !repeat) {
            if (m_ptPipeline) m_ptPipeline->ResetAccumulation();
            LOG_INFO("MLGIPipeline", "重置路径追踪累积");
            return true;
        }
        if (key == Input::KeyCode::N && !repeat) {
            m_enableNEE = !m_enableNEE;
            if (m_ptPipeline) {
                m_ptPipeline->EnableNEE(m_enableNEE);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("MLGIPipeline", "NEE {}", m_enableNEE ? "启用" : "禁用");
            return true;
        }
        if (key == Input::KeyCode::B && !repeat) {
            if (m_ptPipeline) {
                m_ptPipeline->CycleMode();
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("MLGIPipeline", "模式: {}", m_ptPipeline ? m_ptPipeline->GetModeName() : "?");
            return true;
        }
        if (key == Input::KeyCode::M && !repeat) {
            if (m_renderSystem) {
                if (m_currentRenderMode == Prisma::RenderMode::Mode3D_PathTracing) {
                    m_renderSystem->SetRenderMode(Prisma::RenderMode::Mode3D_Forward);
                } else {
                    // 切换到 PathTracing，使用 HardwareRT 扩展需求（自动降级）
                    m_renderSystem->SetRenderMode(Prisma::RenderMode::Mode3D_PathTracing,
                                                  Prisma::Graphic::RTMode::HardwareRT);
                }
            }
            LOG_INFO("MLGIPipeline", "M 键: 请求切换渲染模式");
            return true;
        }
        if (key == Input::KeyCode::LeftBracket && !repeat) {
            m_ptMaxSamples = (m_ptMaxSamples > 16) ? m_ptMaxSamples - 16 : 0;
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("MLGIPipeline", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        if (key == Input::KeyCode::RightBracket && !repeat) {
            m_ptMaxSamples = std::min(m_ptMaxSamples + 16, 4096u);
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("MLGIPipeline", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        if (key == Input::KeyCode::F5 && !repeat) {
            LOG_INFO("MLGIPipeline", "场景重载: {}", m_scenePath);
            LoadScene(m_scenePath);
            return true;
        }
        if (key == Input::KeyCode::F6 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex + 1) % static_cast<int>(m_sceneList.size());
            LOG_INFO("MLGIPipeline", "场景切换 (F6): [{} / {}] {}",
                     m_currentSceneIndex + 1, m_sceneList.size(), m_sceneList[m_currentSceneIndex]);
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        if (key == Input::KeyCode::F7 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex - 1 + static_cast<int>(m_sceneList.size())) % static_cast<int>(m_sceneList.size());
            LOG_INFO("MLGIPipeline", "场景切换 (F7): [{} / {}] {}",
                     m_currentSceneIndex + 1, m_sceneList.size(), m_sceneList[m_currentSceneIndex]);
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        return false;
    });
}

} // namespace Prisma
