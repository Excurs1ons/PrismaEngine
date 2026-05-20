#include "ProfilerPanel.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "core/EntityManager.h"
#include "core/Node.h"
#include <imgui.h>
#include <cstdio>

namespace Prisma {

static void formatBytes(char* buf, size_t bufSize, uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIdx = 0;
    double val = (double)bytes;
    while (val >= 1024.0 && unitIdx < 4) {
        val /= 1024.0;
        unitIdx++;
    }
    snprintf(buf, bufSize, "%.2f %s", val, units[unitIdx]);
}

static void drawFrameTiming(const Engine::FrameStats& stats) {
    if (ImGui::BeginTable("FrameTiming", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Phase");
        ImGui::TableSetupColumn("ms");
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();

        auto row = [](const char* label, double ms) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", label);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", ms);
            ImGui::TableNextColumn();
            float fraction = (float)(ms / 16.667);
            if (fraction > 1.0f) fraction = 1.0f;
            char buf[32]; snprintf(buf, sizeof(buf), "%.1f%%", fraction * 100.0f);
            ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0), buf);
        };
        row("BeginFrame", stats.BeginFrameTime);
        row("Render",      stats.RenderTime);
        row("EndFrame",    stats.EndFrameTime);
        row("Present",     stats.PresentTime);
        row("Total",       stats.TotalTime);

        ImGui::EndTable();
    }
}

static void drawResourceStats(IResourceFactory* factory) {
    if (!factory) { ImGui::TextColored(ImVec4(1,0,0,1), "ResourceFactory unavailable"); return; }

    auto creationStats = factory->GetCreationStats();
    uint64_t budget, usage;
    factory->GetMemoryBudget(budget, usage);

    if (ImGui::BeginTable("ResourceStats", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Resource");
        ImGui::TableSetupColumn("Count");
        ImGui::TableHeadersRow();

        auto row = [](const char* label, uint32_t count) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", label);
            ImGui::TableNextColumn(); ImGui::Text("%u", count);
        };
        row("Textures Created", creationStats.texturesCreated);
        row("Buffers Created",  creationStats.buffersCreated);
        row("Shaders Created",  creationStats.shadersCreated);
        row("Pipelines Created", creationStats.pipelinesCreated);
        row("Samplers Created", creationStats.samplersCreated);
        row("Textures Pooled",  creationStats.texturesPooled);

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Memory Budget");
    {
        char buf[64];
        formatBytes(buf, sizeof(buf), usage);
        ImGui::Text("Allocated:    %s", buf);
        if (budget > 0) {
            formatBytes(buf, sizeof(buf), budget);
            ImGui::Text("Budget Limit: %s", buf);
            float frac = (float)((double)usage / budget);
            char pct[32]; snprintf(pct, sizeof(pct), "%.1f%%", frac * 100.0f);
            ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0), pct);
        } else {
            ImGui::Text("Budget Limit: unlimited");
        }
    }
}

void ProfilerPanel::OnImGuiRender() {
    if (!s_show) return;

    ImGui::Begin("Profiler", &s_show);

    if (ImGui::CollapsingHeader("Frame Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& stats = Engine::Get().GetFrameStats();
        ImGui::Text("FPS: %.1f", stats.FPS);
        drawFrameTiming(stats);
    }

    auto* renderSystem = Engine::Get().GetRenderSystem();
    auto* device = renderSystem ? renderSystem->GetDevice() : nullptr;

    if (ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (device) {
            ImGui::Text("GPU: %s", device->GetGPUName().c_str());

            auto memInfo = device->GetGPUMemoryInfo();
            if (memInfo.totalMemory > 0) {
                if (ImGui::BeginTable("GPUMem", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Field");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableHeadersRow();
                    auto row = [](const char* label, uint64_t bytes) {
                        char buf[64]; formatBytes(buf, sizeof(buf), bytes);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", label);
                        ImGui::TableNextColumn(); ImGui::Text("%s", buf);
                    };
                    row("Total VRAM",     memInfo.totalMemory);
                    row("Used VRAM",      memInfo.usedMemory);
                    row("Available VRAM", memInfo.availableMemory);

                    if (memInfo.totalMemory > 0) {
                        float frac = (float)((double)memInfo.usedMemory / memInfo.totalMemory);
                        char pct[32]; snprintf(pct, sizeof(pct), "%.1f%%", frac * 100.0f);
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("Utilization");
                        ImGui::TableNextColumn();
                        ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0), pct);
                    }
                    ImGui::EndTable();
                }
            } else {
                ImGui::Text("VRAM info: N/A (release build)");
            }
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "Render device not available");
        }
    }

    if (ImGui::CollapsingHeader("Resources", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (device) {
            drawResourceStats(device->GetResourceFactory());
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "Resource factory not available");
        }
    }

    if (ImGui::CollapsingHeader("SoA Entity Pool")) {
        auto& em = Engine::Get().GetEntityManager();
        ImGui::Text("Alive Entities:  %u",  em.GetAliveCount());
        ImGui::Text("Committed Slots: %u",  em.GetCommittedCount());
        ImGui::Text("Max Capacity:    %u",  kMaxVirtualEntities);
        {
            char buf[64];
            formatBytes(buf, sizeof(buf), (uint64_t)em.GetCommittedCount() * 64);
            ImGui::Text("Committed Mem:  %s", buf);
        }
        {
            char buf[64];
            formatBytes(buf, sizeof(buf), (uint64_t)kMaxVirtualEntities * 64);
            ImGui::Text("Reserved Virt:  %s", buf);
        }
    }

    ImGui::End();
}

}
