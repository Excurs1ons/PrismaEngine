#include "ProfilerPanel.h"
#include "ProfilerSystem.h"
#include "app/Engine.h"
#include "core/ISubSystem.h"

// ── ImGui availability ──────────────────────────────────────────────────────
// If imgui.h is reachable the panel renders; otherwise it's a stub.
// The build system should add the imgui include directory and link imgui when
// this file is compiled into a target that needs the panel.
#if __has_include(<imgui.h>)
#include <imgui.h>
#define PRISMA_HAS_IMGUI 1
#else
#define PRISMA_HAS_IMGUI 0
#endif

#include <cstdio>
#include <cmath>

namespace Prisma {
namespace Profiling {

// ── internal helpers ─────────────────────────────────────────────────────────
#if PRISMA_HAS_IMGUI

static Profiling::ProfilerSystem* GetProfiler() {
    auto& engine = Prisma::Engine::Get();
    return engine.GetSystem<Profiling::ProfilerSystem>();
}

static void DrawFrameHistory(Profiling::ProfilerSystem* profiler) {
    if (!profiler) return;

    const uint32_t historySize = Profiling::ProfilerSystem::kFrameHistorySize;
    const uint32_t frameIdx    = profiler->GetFrameIndex();

    // ── CPU frame time histogram (last 60 frames) ──────────────────────────
    if (ImGui::BeginChild("CPUHistogram", ImVec2(0, 120), ImGuiChildFlags_Borders)) {
        ImGui::Text("CPU Frame Time (last %u frames)", historySize);

        float maxTime = 0.0f;
        for (uint32_t i = 0; i < historySize; ++i) {
            float t = profiler->GetFrameRecord(i).cpuFrameTimeMs;
            if (t > maxTime) maxTime = t;
        }
        if (maxTime < 16.667f) maxTime = 16.667f;  // 60 FPS reference

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2       pos = ImGui::GetCursorScreenPos();
        float     width  = ImGui::GetContentRegionAvail().x;
        float     height = 80.0f;
        float     barW   = width / static_cast<float>(historySize);

        // 16.67 ms threshold line (60 FPS)
        float thresholdY = pos.y + height - (16.667f / maxTime) * height;
        draw->AddLine(ImVec2(pos.x, thresholdY), ImVec2(pos.x + width, thresholdY),
                      IM_COL32(0, 255, 0, 100), 1.0f);

        for (uint32_t i = 0; i < historySize; ++i) {
            uint32_t idx = (frameIdx + i) % historySize;
            float    t   = profiler->GetFrameRecord(idx).cpuFrameTimeMs;
            float    h   = (t / maxTime) * height;
            ImU32    color;
            if (t < 16.0f)
                color = IM_COL32(80, 200, 80, 200);
            else if (t < 33.0f)
                color = IM_COL32(200, 200, 80, 200);
            else
                color = IM_COL32(200, 80, 80, 200);

            float x = pos.x + static_cast<float>(i) * barW;
            draw->AddRectFilled(ImVec2(x, pos.y + height - h),
                                ImVec2(x + barW - 1.0f, pos.y + height), color);
        }

        ImGui::Dummy(ImVec2(width, height));
    }
    ImGui::EndChild();

    // ── GPU frame time histogram ───────────────────────────────────────────
    if (ImGui::BeginChild("GPUHistogram", ImVec2(0, 120), ImGuiChildFlags_Borders)) {
        ImGui::Text("GPU Frame Time (last %u frames)", historySize);

        float maxTime = 0.0f;
        bool  hasGpu  = false;
        for (uint32_t i = 0; i < historySize; ++i) {
            float t = profiler->GetFrameRecord(i).gpuFrameTimeMs;
            if (t > 0.0f) hasGpu = true;
            if (t > maxTime) maxTime = t;
        }

        if (!hasGpu) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                               "GPU profiling not available");
        } else {
            if (maxTime < 16.667f) maxTime = 16.667f;

            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2       pos = ImGui::GetCursorScreenPos();
            float     width  = ImGui::GetContentRegionAvail().x;
            float     height = 80.0f;
            float     barW   = width / static_cast<float>(historySize);

            float thresholdY = pos.y + height - (16.667f / maxTime) * height;
            draw->AddLine(ImVec2(pos.x, thresholdY), ImVec2(pos.x + width, thresholdY),
                          IM_COL32(0, 200, 255, 100), 1.0f);

            for (uint32_t i = 0; i < historySize; ++i) {
                uint32_t idx = (frameIdx + i) % historySize;
                float    t   = profiler->GetFrameRecord(idx).gpuFrameTimeMs;
                float    h   = (t / maxTime) * height;
                ImU32    color = IM_COL32(80, 80, 200, 200);

                float x = pos.x + static_cast<float>(i) * barW;
                draw->AddRectFilled(ImVec2(x, pos.y + height - h),
                                    ImVec2(x + barW - 1.0f, pos.y + height), color);
            }

            ImGui::Dummy(ImVec2(width, height));
        }
    }
    ImGui::EndChild();
}

static void DrawWaterfall(Profiling::ProfilerSystem* profiler) {
    if (!profiler) return;

    constexpr uint32_t kWaterfallFrames = 32;

    if (ImGui::BeginChild("Waterfall", ImVec2(0, 300), ImGuiChildFlags_Borders)) {
        ImGui::Text("CPU Waterfall (last %u frames, main thread)", kWaterfallFrames);

        ImDrawList* draw  = ImGui::GetWindowDrawList();
        ImVec2       pos  = ImGui::GetCursorScreenPos();
        float     width   = ImGui::GetContentRegionAvail().x;
        float     rowH    = 28.0f;
        float     totalH  = rowH * kWaterfallFrames;

        // Read samples from the main thread profiler
        CpuSample samples[1024];
        uint32_t sampleCount = profiler->ReadMainThreadSamples(samples, 1024);
        if (sampleCount == 0) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                               "No CPU samples yet");
        } else {
            // Determine the time range covered by the samples
            uint64_t firstNs = samples[0].startNs;
            uint64_t lastNs  = firstNs;
            for (uint32_t i = 0; i < sampleCount; ++i) {
                uint64_t end = samples[i].startNs + samples[i].durationNs;
                if (end > lastNs) lastNs = end;
            }
            double rangeMs = static_cast<double>(lastNs - firstNs) / 1'000'000.0;
            if (rangeMs < 1.0) rangeMs = 16.667;  // minimum span

            // Draw each sample as a coloured bar
            static const ImU32 kColors[] = {
                IM_COL32(255, 100, 100, 200),
                IM_COL32(100, 255, 100, 200),
                IM_COL32(100, 100, 255, 200),
                IM_COL32(255, 255, 100, 200),
                IM_COL32(255, 100, 255, 200),
                IM_COL32(100, 255, 255, 200),
                IM_COL32(200, 200, 100, 200),
                IM_COL32(200, 100, 200, 200),
            };

            for (uint32_t i = 0; i < sampleCount; ++i) {
                const CpuSample& s = samples[i];
                // Skip very short samples
                if (s.durationNs < 1'000) continue;

                double localStart = static_cast<double>(s.startNs - firstNs) / 1'000'000.0;
                double localDur   = static_cast<double>(s.durationNs) / 1'000'000.0;

                float x0 = pos.x + static_cast<float>(localStart / rangeMs) * width;
                float x1 = pos.x + static_cast<float>((localStart + localDur) / rangeMs) * width;
                float y  = pos.y + static_cast<float>(s.depth % kWaterfallFrames) * rowH;

                if (x1 < pos.x || x0 > pos.x + width) continue;
                if (x1 > pos.x + width) x1 = pos.x + width;
                if (x0 < pos.x) x0 = pos.x;
                if (x1 - x0 < 1.0f) continue;

                ImU32 color = kColors[s.depth % 8];
                draw->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + rowH - 2), color);

                // Label if wide enough
                if (x1 - x0 > 40.0f && s.name) {
                    draw->AddText(ImVec2(x0 + 2, y + 2), IM_COL32(255, 255, 255, 220), s.name);
                }
            }
        }

        ImGui::Dummy(ImVec2(width, totalH));
    }
    ImGui::EndChild();
}

static void DrawFrameSummary(Profiling::ProfilerSystem* profiler) {
    if (!profiler) return;

    const auto& rec = profiler->GetFrameRecord(
        (profiler->GetFrameIndex() - 1) % Profiling::ProfilerSystem::kFrameHistorySize);

    if (ImGui::BeginTable("FrameSummary", 2,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Metric");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        auto row = [](const char* label, const char* val) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", label);
            ImGui::TableNextColumn();
            ImGui::Text("%s", val);
        };

        char buf[64];

        auto& engine = Prisma::Engine::Get();
        std::snprintf(buf, sizeof(buf), "%.1f", engine.GetFPS());
        row("FPS", buf);

        std::snprintf(buf, sizeof(buf), "%.3f ms", rec.cpuFrameTimeMs);
        row("CPU Frame Time", buf);

        std::snprintf(buf, sizeof(buf), "%.3f ms", rec.gpuFrameTimeMs);
        row("GPU Frame Time", buf);

        std::snprintf(buf, sizeof(buf), "%u", rec.cpuSampleCount);
        row("CPU Samples", buf);

        ImGui::EndTable();
    }
}

#endif // PRISMA_HAS_IMGUI

// ── public interface ─────────────────────────────────────────────────────────

void ProfilerPanel::OnImGuiRender() {
#if PRISMA_HAS_IMGUI
    if (!s_show) return;

    ImGui::Begin("Profiler (Engine)", &s_show, ImGuiWindowFlags_NoCollapse);

    auto* profiler = GetProfiler();
    if (!profiler) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "ProfilerSystem not available");
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Summary", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawFrameSummary(profiler);
    }

    if (ImGui::CollapsingHeader("Histograms", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawFrameHistory(profiler);
    }

    if (ImGui::CollapsingHeader("Waterfall", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawWaterfall(profiler);
    }

    if (ImGui::CollapsingHeader("GPU Timers")) {
        auto* gpu = profiler->GetGpuProfiler();
        if (gpu && gpu->IsAvailable()) {
            uint32_t count = gpu->GetSampleCount();
            const GpuSample* samples = gpu->GetSamples();
            if (ImGui::BeginTable("GpuTimers", 3,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Duration");
                ImGui::TableSetupColumn("Available");
                ImGui::TableHeadersRow();
                for (uint32_t i = 0; i < count; ++i) {
                    if (!samples[i].available) continue;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", samples[i].name ? samples[i].name : "?");
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f ms", samples[i].durationMs);
                    ImGui::TableNextColumn();
                    ImGui::Text("yes");
                }
                ImGui::EndTable();
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                               "GPU profiling not available");
        }
    }

    ImGui::End();
#else
    (void)s_show;
#endif
}

void ProfilerPanel::Toggle() {
    s_show = !s_show;
}

} // namespace Profiling
} // namespace Prisma
