#include "ProfilerSystem.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "logger/Logger.h"

namespace Prisma {
namespace Profiling {

ProfilerSystem::~ProfilerSystem() {
    Shutdown();
}

int ProfilerSystem::Initialize() {
    LOG_INFO("ProfilerSystem", "Initializing");

    // Attempt to create the GPU profiler.
    // If the render device is not yet available (e.g. headless mode), the
    // GPU profiler stays null and CPU-only profiling is used.
    auto& engine = Engine::Get();
    auto* renderSys = engine.GetRenderSystem();
    if (renderSys) {
        auto* device = renderSys->GetDevice();
        if (device) {
            VkDevice vkDev = device->GetVkDevice();
            if (vkDev) {
                m_gpuProfiler = std::make_unique<GpuProfiler>();
                if (!m_gpuProfiler->Initialize(vkDev)) {
                    LOG_WARNING("ProfilerSystem", "GPU profiling unavailable (VkQueryPool creation failed)");
                    m_gpuProfiler.reset();
                } else {
                    LOG_INFO("ProfilerSystem", "GPU profiling ready");
                }
            }
        }
    }

    if (!m_gpuProfiler) {
        LOG_INFO("ProfilerSystem", "CPU-only profiling mode");
    }

    return 0;
}

void ProfilerSystem::Shutdown() {
    if (m_gpuProfiler) {
        m_gpuProfiler->Shutdown();
        m_gpuProfiler.reset();
    }
}

void ProfilerSystem::Update(Timestep ts) {
    PROFILE_FUNCTION();

    // ── 1. Stop the previous frame's timer ─────────────────────────────────
    if (m_frameTimerActive) {
        m_currentCpuFrameTimeMs = m_frameTimer.ElapsedMs();
    }

    // ── 2. Collect GPU results from the completed batch ────────────────────
    if (m_gpuProfiler) {
        m_gpuProfiler->CollectResults();
    }

    // ── 3. Store frame record ──────────────────────────────────────────────
    FrameRecord& rec = m_frameHistory[m_frameIndex % kFrameHistorySize];
    rec.cpuFrameTimeMs = m_currentCpuFrameTimeMs;
    rec.gpuFrameTimeMs = 0.0f;
    rec.cpuSampleCount = CpuProfiler::Get().GetCount();

    if (m_gpuProfiler) {
        const GpuSample* samples = m_gpuProfiler->GetSamples();
        uint32_t count = m_gpuProfiler->GetSampleCount();
        float totalGpuMs = 0.0f;
        for (uint32_t i = 0; i < count; ++i) {
            if (samples[i].available) {
                totalGpuMs += samples[i].durationMs;
            }
        }
        rec.gpuFrameTimeMs = totalGpuMs;
    }

    ++m_frameIndex;

    // ── 4. Swap GPU profiler batches for the next frame ────────────────────
    if (m_gpuProfiler) {
        m_gpuProfiler->NextFrame();
    }

    // ── 5. Restart frame timer ─────────────────────────────────────────────
    m_frameTimer.Reset();
    m_frameTimerActive = true;
}

} // namespace Profiling
} // namespace Prisma
