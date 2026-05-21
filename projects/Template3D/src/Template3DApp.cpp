#include "Template3DApp.h"

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

namespace Prisma {
using namespace Graphic;

// ============================================================================
// Template3DApp
// ============================================================================

Template3DApp::Template3DApp()
    : Application()
{
}

Template3DApp::~Template3DApp() = default;

int Template3DApp::OnInitialize() {
    LOG_INFO("Template3D", "3D 模板初始化（路径追踪引擎管线版）");

    // 无头模式：CLI 值优先，未提供的从 project.json 补全
    if (Engine::Get().GetSpecification().Headless) {
        m_headlessCfg.enabled = true;
        m_headlessCfg.totalFrames   = m_headlessCfg.totalFrames   ? m_headlessCfg.totalFrames   : m_Spec.HeadlessFrames;
        m_headlessCfg.width         = m_headlessCfg.width         ? m_headlessCfg.width         : m_Spec.HeadlessWidth;
        m_headlessCfg.height        = m_headlessCfg.height        ? m_headlessCfg.height        : m_Spec.HeadlessHeight;
        m_headlessCfg.outputPath    = m_headlessCfg.outputPath.empty() ? m_Spec.HeadlessOutputPath : m_headlessCfg.outputPath;
        m_Spec.Width  = m_headlessCfg.width;
        m_Spec.Height = m_headlessCfg.height;
        LOG_INFO("Template3D", "headless模式分辨率: {}x{} (frames={}, output={})",
                 m_Spec.Width, m_Spec.Height, m_headlessCfg.totalFrames, m_headlessCfg.outputPath);
    }

    m_ptPipeline = Engine::Get().GetRenderSystem()->GetMainPipelineAs<Graphic::PathTracingPipeline>();
    if (!m_ptPipeline) {
        LOG_ERROR("Template3D", "获取路径追踪管线失败（renderMode 不匹配？）");
        return -1;
    }

    // 通过 SceneManager 获取已加载的场景（Engine 已从 project.json 的 entryScene 自动加载）
    // 相机已作为场景节点（Camera 组件）由 Scene::Deserialize 自动创建，Engine 已同步 viewport
    auto* sceneManager = Engine::Get().GetSceneManager();
    if (sceneManager) {
        m_scene = sceneManager->GetCurrentScene();
    }

    // 默认用 Primitive（禁用所有 MeshRenderer）
    if (m_scene) {
        for (const auto& node : m_scene->GetNodes()) {
            if (auto meshR = m_scene->GetComponent<Graphic::MeshRenderer>(node))
                meshR->SetEnabled(false);
        }
    }

    // 从 project.json 读取管线参数（可被 CLI --samples 覆盖）
    m_ptPipeline->SetMaxSamples(m_ptMaxSamples > 0 ? m_ptMaxSamples : m_Spec.MaxSamples);

    // 从 project.jsonc 读取路径追踪模式
    m_ptPipeline->SetMode(m_Spec.PathTraceMode);

    // 从 project.json 读取 NEE 开关
    m_enableNEE = m_Spec.EnableNEE;
    m_ptPipeline->EnableNEE(m_enableNEE);

    LOG_INFO("Template3D", "P/R 重置累积，[/] 调整采样帧数，N 切换 NEE");
    return 0;
}

void Template3DApp::OnRender() {
    if (!m_ptPipeline) return;

    DrawStatsOverlay();
}

void Template3DApp::SavePathTracingOutput() {
    if (m_ptPipeline) {
        m_ptPipeline->SaveOutput(m_headlessCfg.outputPath);
    }
}

void Template3DApp::DrawStatsOverlay() {
    float winW = static_cast<float>(m_Spec.Width);
    float winH = static_cast<float>(m_Spec.Height);

    // 使用成员变量替代 static 局部变量（M1）
    double now = Platform::GetTimeSeconds();
    float dt = (m_overlayLastTime > 0.0) ? static_cast<float>(now - m_overlayLastTime) : 0.016f;
    m_overlayLastTime = now;

    const auto& st = Engine::Get().GetFrameStats();
    m_overlayRefreshTimer += dt;

    if (m_overlayRefreshTimer >= 1.0f) {
        // 用 std::format 替代 snprintf（L2）
        m_overlayTimingInfo = std::format(
            "BF={:.2f} Render={:.2f} EF={:.2f} Present={:.2f} Total={:.2f} (ms)",
            st.BeginFrameTime, st.RenderTime, st.EndFrameTime, st.PresentTime, st.TotalTime
        );

        double maxTime = st.BeginFrameTime;
        std::string leadStage = "BF";
        if (st.RenderTime > maxTime) { maxTime = st.RenderTime; leadStage = "Render(CPU)"; }
        if (st.EndFrameTime > maxTime) { maxTime = st.EndFrameTime; leadStage = "EF(GPU)"; }
        if (st.PresentTime > maxTime) { maxTime = st.PresentTime; leadStage = "Present"; }
        if (st.TotalTime < 2.0) {
            m_overlayStatusStr = "Status: Balanced (Lead: " + leadStage + ")";
            m_overlayStatusColor = {0.2f, 1.0f, 0.2f, 1.0f};
        } else {
            m_overlayStatusStr = "Status: LEAD " + leadStage;
            m_overlayStatusColor = (leadStage.find("CPU") != std::string::npos)
                                       ? PrismaMath::vec4{1.0f, 0.2f, 0.8f, 1.0f}
                                       : PrismaMath::vec4{1.0f, 0.2f, 0.2f, 1.0f};
        }

        m_overlayResInfo = std::to_string(m_Spec.Width) + "x" + std::to_string(m_Spec.Height)
                + " @ " + std::to_string(static_cast<int>(Engine::Get().GetFPS())) + " FPS";
        m_overlayRefreshTimer = 0.0f;
    }



    Renderer2D::DrawString(m_overlayTimingInfo, {30.0f, winH - 45.0f}, 1.5f, {0.2f, 1.0f, 0.2f, 1.0f});
    Renderer2D::DrawString(m_overlayStatusStr, {30.0f, winH - 85.0f}, 1.5f, m_overlayStatusColor);

    float resW = Renderer2D::GetStringWidth(m_overlayResInfo, 3.0f);
    Renderer2D::DrawString(m_overlayResInfo, {winW - resW - 30.0f, winH - 50.0f}, 3.0f, {0.4f, 0.7f, 0.4f, 1.0f});

    std::string gpuName = Engine::Get().GetGPUName();
    if (!gpuName.empty()) {
        float gW = Renderer2D::GetStringWidth(gpuName, 2.0f);
        Renderer2D::DrawString(gpuName, {winW - gW - 30.0f, winH - 95.0f}, 2.0f, {0.5f, 0.5f, 0.5f, 1.0f});
    }

    float escW = Renderer2D::GetStringWidth("ESC to exit", 2.0f);
    Renderer2D::DrawString("ESC to exit", {winW - escW - 30.0f, 30.0f}, 2.0f, {0.4f, 0.4f, 0.4f, 1.0f});
    Renderer2D::DrawString("Template3D (PathTracing)",
                           {30.0f, 30.0f}, 2.0f, {0.6f, 0.6f, 0.6f, 1.0f});

    Renderer2D::DrawString("[P/R] Reset  [N] NEE  [M] Mode  [ -Samples+ ]",
                           {30.0f, 65.0f}, 1.5f, {0.6f, 0.6f, 0.9f, 1.0f});

    if (m_ptPipeline) {
        uint32_t frameCount = m_ptPipeline->GetFrameCount();
        uint32_t maxSamples = m_ptPipeline->GetMaxSamples();

        // SPS = 自上次重置以来的平均 samples/sec
        if (frameCount == 0) {
            m_accumStartTime = now;
            m_cachedSPS = 0;
        } else if (!m_ptPipeline->IsConverged() && now > m_accumStartTime) {
            double elapsed = now - m_accumStartTime;
            m_cachedSPS = static_cast<uint32_t>(frameCount / elapsed + 0.5);
        }

        std::string ptInfo;
        Prisma::Vector4 ptColor;
        if (m_ptPipeline->IsConverged()) {
            ptInfo = std::format("[{}] Converged: {}/{} samples  {} S/s  |  {}x{}",
                                 m_ptPipeline->GetModeName(), frameCount, maxSamples, m_cachedSPS,
                                 m_Spec.Width, m_Spec.Height);
            ptColor = {0.2f, 1.0f, 0.2f, 1.0f};
        } else {
            std::string maxStr = maxSamples > 0 ? "/" + std::to_string(maxSamples) : "+";
            ptInfo = std::format("[{}] Pt: {}{} samples  {} S/s  |  {}x{}",
                                 m_ptPipeline->GetModeName(), frameCount, maxStr, m_cachedSPS,
                                 m_Spec.Width, m_Spec.Height);
            ptColor = {0.9f, 0.6f, 0.2f, 1.0f};
        }
        Renderer2D::DrawString(ptInfo, {30.0f, 130.0f}, 1.5f, ptColor);
    }

    // 实时 NEE + Primitive 状态（每帧绘制，不缓存）
    if (m_ptPipeline) {
        Prisma::Vector4 neeColor = m_enableNEE
            ? Prisma::Vector4{0.2f, 1.0f, 0.2f, 1.0f}
            : Prisma::Vector4{0.6f, 0.6f, 0.6f, 1.0f};
        Renderer2D::DrawString(m_enableNEE ? "NEE: ON" : "NEE: OFF", {30.0f, 165.0f}, 1.5f, neeColor);
        Prisma::Vector4 primColor = m_usePrimitiveSphere
            ? Prisma::Vector4{0.2f, 1.0f, 0.2f, 1.0f}
            : Prisma::Vector4{0.6f, 0.6f, 0.6f, 1.0f};
        Renderer2D::DrawString(m_usePrimitiveSphere ? "Primitive: ON" : "Primitive: OFF", {30.0f, 185.0f}, 1.5f, primColor);
    }
}

void Template3DApp::OnUpdate([[maybe_unused]] Timestep ts) {
    if (m_headlessCfg.enabled && m_ptPipeline) {
        if (m_ptPipeline->GetFrameCount() >= m_headlessCfg.totalFrames) {
            LOG_INFO("Template3D", "headless模式完成，保存输出...");
            SavePathTracingOutput();
            Close();
        }
    }
}

void Template3DApp::OnEvent(Event& e) {
    Application::OnEvent(e);

    EventDispatcher d(e);
    d.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& ev) {
        // H2: 用 InputManager::KeyCode 替代 SDL_SCANCODE_*
        auto key = static_cast<Input::KeyCode>(ev.GetKeyCode());
        bool repeat = ev.IsRepeat();

        if (key == Input::KeyCode::Escape) {
            Close();
            return true;
        }
        if (key == Input::KeyCode::P && !repeat) {
            // 全局切换 PrimitiveComponent ↔ MeshRenderer
            m_usePrimitiveSphere = !m_usePrimitiveSphere;
            if (m_scene) {
                for (const auto& node : m_scene->GetNodes()) {
                    if (auto prim = m_scene->GetComponent<Graphic::PrimitiveComponent>(node))
                        prim->SetEnabled(m_usePrimitiveSphere);
                    if (auto meshR = m_scene->GetComponent<Graphic::MeshRenderer>(node))
                        meshR->SetEnabled(!m_usePrimitiveSphere);
                }
                m_scene->SetDirty(true);
                LOG_INFO("Template3D", "全局模式 → {}", m_usePrimitiveSphere ? "Primitive (原生)" : "Mesh (三角化)");
            }
            return true;
        }
        if (key == Input::KeyCode::R && !repeat) {
            if (m_ptPipeline) m_ptPipeline->ResetAccumulation();
            LOG_INFO("Template3D", "重置路径追踪累积");
            return true;
        }
        if (key == Input::KeyCode::N && !repeat) {
            m_enableNEE = !m_enableNEE;
            if (m_ptPipeline) {
                m_ptPipeline->EnableNEE(m_enableNEE);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("Template3D", "NEE {}", m_enableNEE ? "启用" : "禁用");
            return true;
        }
        if (key == Input::KeyCode::M && !repeat) {
            if (m_ptPipeline) {
                m_ptPipeline->CycleMode();
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("Template3D", "模式: {}", m_ptPipeline ? m_ptPipeline->GetModeName() : "?");
            return true;
        }
        if (key == Input::KeyCode::LeftBracket && !repeat) {
            m_ptMaxSamples = (m_ptMaxSamples > 16) ? m_ptMaxSamples - 16 : 0;
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("Template3D", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        if (key == Input::KeyCode::RightBracket && !repeat) {
            m_ptMaxSamples = std::min(m_ptMaxSamples + 16, 4096u);
            if (m_ptPipeline) {
                m_ptPipeline->SetMaxSamples(m_ptMaxSamples);
                m_ptPipeline->ResetAccumulation();
            }
            LOG_INFO("Template3D", "最大采样帧数: {}", m_ptMaxSamples);
            return true;
        }
        return false;
    });
}

} // namespace Prisma
