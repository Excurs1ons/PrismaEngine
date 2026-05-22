#include "StatsOverlay.h"

#include "graphic/Renderer2D.h"
#include "app/Engine.h"
#include "app/Application.h"
#include "platform/Platform.h"

#include <format>
#include <string>

namespace Prisma {
using namespace Graphic;

StatsOverlay::StatsOverlay(const ApplicationSpecification& spec, Graphic::PathTracingPipeline* pipeline,
                           bool enableNEE, bool usePrimitiveSphere)
    : Component()
    , m_spec(spec)
    , m_pipeline(pipeline)
    , m_enableNEE(enableNEE)
    , m_usePrimitiveSphere(usePrimitiveSphere)
{
}

void StatsOverlay::Update(Timestep ts) {
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
        if (st.TotalTime < 2.0) {
            m_overlayStatusStr = "Status: Balanced (Lead: " + leadStage + ")";
            m_overlayStatusColor = {0.2f, 1.0f, 0.2f, 1.0f};
        } else {
            m_overlayStatusStr = "Status: LEAD " + leadStage;
            m_overlayStatusColor = (leadStage.find("CPU") != std::string::npos)
                                       ? PrismaMath::vec4{1.0f, 0.2f, 0.8f, 1.0f}
                                       : PrismaMath::vec4{1.0f, 0.2f, 0.2f, 1.0f};
        }

        m_overlayResInfo = std::to_string(m_spec.Width) + "x" + std::to_string(m_spec.Height)
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

    Renderer2D::DrawString("[R]Reset [N]NEE [B]Mode [P]Prim|Mesh [ -Samples+ ] [F5]Reload [F6/F7]Scene",
                           {30.0f, 65.0f}, 1.5f, {0.6f, 0.6f, 0.9f, 1.0f});

    if (m_pipeline) {
        uint32_t frameCount = m_pipeline->GetFrameCount();
        uint32_t maxSamples = m_pipeline->GetMaxSamples();

        if (frameCount == 0) {
            m_accumStartTime = now;
            m_cachedSPS = 0;
        } else if (!m_pipeline->IsConverged() && now > m_accumStartTime) {
            double elapsed = now - m_accumStartTime;
            m_cachedSPS = static_cast<uint32_t>(frameCount / elapsed + 0.5);
        }

        std::string ptInfo;
        Prisma::Vector4 ptColor;
        if (m_pipeline->IsConverged()) {
            ptInfo = std::format("[{}] Converged: {}/{} samples  {} S/s  |  {}x{}",
                                 m_pipeline->GetModeName(), frameCount, maxSamples, m_cachedSPS,
                                 m_spec.Width, m_spec.Height);
            ptColor = {0.2f, 1.0f, 0.2f, 1.0f};
        } else {
            std::string maxStr = maxSamples > 0 ? "/" + std::to_string(maxSamples) : "+";
            ptInfo = std::format("[{}] Pt: {}{} samples  {} S/s  |  {}x{}",
                                 m_pipeline->GetModeName(), frameCount, maxStr, m_cachedSPS,
                                 m_spec.Width, m_spec.Height);
            ptColor = {0.9f, 0.6f, 0.2f, 1.0f};
        }
        Renderer2D::DrawString(ptInfo, {30.0f, 130.0f}, 1.5f, ptColor);
    }

    // NEE + Primitive status (drawn every frame, not cached)
    if (m_pipeline) {
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

} // namespace Prisma
