#include "StatsOverlay.h"

#include "graphic/Renderer2D.h"
#include "app/Engine.h"
#include "app/Application.h"
#include "platform/Platform.h"

#include <format>
#include <string>

namespace Prisma {
using namespace Graphic;

StatsOverlay::StatsOverlay(const ApplicationSpecification& spec)
    : Component()
    , m_spec(spec)
{
}

void StatsOverlay::Update(Timestep ts)
{
    float winW = static_cast<float>(m_spec.Width);
    float winH = static_cast<float>(m_spec.Height);

    double now = Platform::GetTimeSeconds();
    float dt = (m_overlayLastTime > 0.0) ? static_cast<float>(now - m_overlayLastTime) : 0.016f;
    m_overlayLastTime = now;

    const auto& st = Engine::Get().GetFrameStats();
    m_overlayRefreshTimer += dt;

    if (m_overlayRefreshTimer >= 1.0f) {
        m_overlayTimingInfo = std::format(
            "BF={:.2f} Render={:.2f} EF={:.2f} Present={:.2f} Total={:.2f} (ms)",
            st.BeginFrameTime, st.RenderTime, st.EndFrameTime, st.PresentTime, st.TotalTime
        );

        double maxTime = st.BeginFrameTime;
        std::string leadStage = "BF";
        if (st.RenderTime > maxTime) { maxTime = st.RenderTime; leadStage = "Render(CPU)"; }
        if (st.EndFrameTime > maxTime) { maxTime = st.EndFrameTime; leadStage = "EF(GPU)"; }
        if (st.PresentTime > maxTime) { maxTime = st.PresentTime; leadStage = "Present"; }
        m_overlayStatusStr = (st.TotalTime < 2.0)
            ? "Status: Balanced (Lead: " + leadStage + ")"
            : "Status: LEAD " + leadStage;

        m_overlayResInfo = std::to_string(m_spec.Width) + "x" + std::to_string(m_spec.Height)
            + " @ " + std::to_string(static_cast<int>(Engine::Get().GetFPS())) + " FPS";

        m_gBufferInfo = std::format("GBuffer: {}x{} RGBA16F(x2) RGBA8 RGBA16F D32F",
            m_spec.Width, m_spec.Height);

        m_passInfo = "Passes: Geometry Skybox Lighting Transparent Composition";

        m_overlayRefreshTimer = 0.0f;
    }

    Renderer2D::DrawString(m_overlayTimingInfo, {30.0f, winH - 45.0f}, 1.5f, {0.2f, 1.0f, 0.2f, 1.0f});
    Renderer2D::DrawString(m_overlayStatusStr, {30.0f, winH - 85.0f}, 1.5f, {0.9f, 0.6f, 0.2f, 1.0f});

    float resW = Renderer2D::GetStringWidth(m_overlayResInfo, 3.0f);
    Renderer2D::DrawString(m_overlayResInfo, {winW - resW - 30.0f, winH - 50.0f}, 3.0f, {0.4f, 0.7f, 0.4f, 1.0f});

    std::string gpuName = Engine::Get().GetGPUName();
    if (!gpuName.empty()) {
        float gW = Renderer2D::GetStringWidth(gpuName, 2.0f);
        Renderer2D::DrawString(gpuName, {winW - gW - 30.0f, winH - 95.0f}, 2.0f, {0.5f, 0.5f, 0.5f, 1.0f});
    }

    Renderer2D::DrawString("Deferred3D (Deferred Rendering Debug)",
                           {30.0f, 30.0f}, 2.0f, {0.6f, 0.6f, 0.6f, 1.0f});

    Renderer2D::DrawString(
        "[G]GBuffer [ -Target+ ] [F5]Reload [F6/F7]Scene",
        {30.0f, 65.0f}, 1.5f, {0.6f, 0.6f, 0.9f, 1.0f});

    Renderer2D::DrawString(m_gBufferInfo,
                           {30.0f, 130.0f}, 1.5f, {0.2f, 1.0f, 0.2f, 1.0f});

    Renderer2D::DrawString(m_passInfo,
                           {30.0f, 165.0f}, 1.5f, {0.9f, 0.6f, 0.2f, 1.0f});

    float escW = Renderer2D::GetStringWidth("ESC to exit", 2.0f);
    Renderer2D::DrawString("ESC to exit", {winW - escW - 30.0f, 30.0f}, 2.0f, {0.4f, 0.4f, 0.4f, 1.0f});
}

} // namespace Prisma
