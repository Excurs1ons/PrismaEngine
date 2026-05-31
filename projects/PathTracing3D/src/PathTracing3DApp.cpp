#include "PathTracing3DApp.h"
#include "StatsOverlay.h"
#include "HeadlessRunner.h"

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
// PathTracing3DApp
// ============================================================================

PathTracing3DApp::PathTracing3DApp()
    : Application()
{
}

PathTracing3DApp::~PathTracing3DApp() = default;

int PathTracing3DApp::OnInitialize() {
    LOG_INFO("PathTracing3D", "3D 模板初始化（路径追踪引擎管线版）");

    m_ptPipeline = Engine::Get().GetRenderSystem()->GetMainPipelineAs<Graphic::PathTracingPipeline>();
    if (!m_ptPipeline) {
        // 光线追踪不可用（设备不支持），回退到 Forward 模式
        LOG_WARN("PathTracing3D", "路径追踪管线不可用，设备不支持光线追踪，回退到 Forward 渲染模式");
        m_fallbackMode = true;
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
                                                        m_enableNEE, m_usePrimitiveSphere);
        m_statsOverlay->SetOwnerNode(overlayNode, m_scene);
    }

    // Create HeadlessRunner (m_Spec has CLI overrides applied by Engine::Run)
    bool headless = Engine::Get().GetSpecification().Headless;
    if (headless && m_ptPipeline) {
        m_Spec.Width  = m_Spec.HeadlessWidth;
        m_Spec.Height = m_Spec.HeadlessHeight;
        LOG_INFO("PathTracing3D", "headless模式分辨率: {}x{} (frames={}, output={})",
                 m_Spec.Width, m_Spec.Height, m_Spec.HeadlessFrames, m_Spec.HeadlessOutputPath);
        m_headlessRunner = std::make_unique<HeadlessRunner>(
            headless, m_Spec.HeadlessFrames, m_Spec.HeadlessOutputPath, m_ptPipeline.get());
    }

    LOG_INFO("PathTracing3D", "R 重置累积，B 切换模式，P 切换 Primitive|Mesh，N 切换 NEE，[/] 调整采样帧数，F5 重载场景，F6/F7 切换场景");
    return 0;
}

void PathTracing3DApp::LoadScene(const std::string& path) {
    auto* sceneManager = Engine::Get().GetSceneManager();
    if (!sceneManager) return;

    if (!sceneManager->LoadFromFile(path)) {
        LOG_ERROR("PathTracing3D", "场景加载失败: {}", path);
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

    LOG_INFO("PathTracing3D", "场景已切换: {}", path);
}

void PathTracing3DApp::OnRender() {
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
    }
}

void PathTracing3DApp::OnUpdate(Timestep ts) {
    // Auto-rebuild PT data when scene is dirty (triggered by P key etc.)
    if (m_scene && m_scene->IsDirty()) {
        if (m_ptPipeline) {
            m_ptPipeline->ReloadSceneData();
        }
        m_scene->SetDirty(false);
    }

    if (m_headlessRunner && m_headlessRunner->Update()) {
        Close();
    }
}

void PathTracing3DApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
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
                LOG_INFO("PathTracing3D", "全局模式 → {}", m_usePrimitiveSphere ? "Primitive (原生)" : "Mesh (三角化)");
            }
            return true;
        }
        if (key == Input::KeyCode::R && !repeat) {
            if (m_ptPipeline) m_ptPipeline->ResetAccumulation();
            LOG_INFO("PathTracing3D", "重置路径追踪累积");
            return true;
        }
        if (key == Input::KeyCode::N && !repeat) {
            m_enableNEE = !m_enableNEE;
            if (m_ptPipeline) {
                m_ptPipeline->EnableNEE(m_enableNEE);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("PathTracing3D", "NEE {}", m_enableNEE ? "启用" : "禁用");
            return true;
        }
        if (key == Input::KeyCode::B && !repeat) {
            if (m_ptPipeline) {
                m_ptPipeline->CycleMode();
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("PathTracing3D", "模式: {}", m_ptPipeline ? m_ptPipeline->GetModeName() : "?");
            return true;
        }
        if (key == Input::KeyCode::LeftBracket && !repeat) {
            m_ptMaxSamples = (m_ptMaxSamples > 16) ? m_ptMaxSamples - 16 : 0;
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("PathTracing3D", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        if (key == Input::KeyCode::RightBracket && !repeat) {
            m_ptMaxSamples = std::min(m_ptMaxSamples + 16, 4096u);
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("PathTracing3D", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        if (key == Input::KeyCode::F5 && !repeat) {
            LOG_INFO("PathTracing3D", "场景重载: {}", m_scenePath);
            LoadScene(m_scenePath);
            return true;
        }
        if (key == Input::KeyCode::F6 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex + 1) % static_cast<int>(m_sceneList.size());
            LOG_INFO("PathTracing3D", "场景切换 (F6): [{} / {}] {}",
                     m_currentSceneIndex + 1, m_sceneList.size(), m_sceneList[m_currentSceneIndex]);
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        if (key == Input::KeyCode::F7 && !repeat && !m_sceneList.empty()) {
            m_currentSceneIndex = (m_currentSceneIndex - 1 + static_cast<int>(m_sceneList.size())) % static_cast<int>(m_sceneList.size());
            LOG_INFO("PathTracing3D", "场景切换 (F7): [{} / {}] {}",
                     m_currentSceneIndex + 1, m_sceneList.size(), m_sceneList[m_currentSceneIndex]);
            LoadScene(m_sceneList[m_currentSceneIndex]);
            return true;
        }
        return false;
    });
}

} // namespace Prisma
