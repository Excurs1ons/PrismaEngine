#pragma once

#include "../Export.h"
#include "../core/ISubSystem.h"
#include "../core/Timestep.h"
#include "ProfilerCPU.h"
#include "ProfilerGPU.h"
#include <memory>
#include <array>

namespace Prisma {
namespace Profiling {

// ─── FrameRecord ────────────────────────────────────────────────────────────
// Summary of one frame's profiling data, consumed by the ProfilerPanel.
//
struct FrameRecord {
    float cpuFrameTimeMs  = 0.0f;   // wall-clock time of the frame
    float gpuFrameTimeMs  = 0.0f;   // total GPU time (if available)
    uint32_t cpuSampleCount = 0;    // CPU samples recorded this frame
    uint32_t threadCount    = 0;    // threads that contributed samples
};


// ─── ProfilerSystem ─────────────────────────────────────────────────────────
// Engine subsystem that owns the GPU profiler and accumulates per-frame
// profiling data for the ImGui panel.
//
class ENGINE_API ProfilerSystem : public ISubSystem {
public:
    ProfilerSystem()  = default;
    ~ProfilerSystem() override;

    int                Initialize()            override;
    void               Shutdown()              override;
    void               Update(Timestep ts)     override;
    const char*        GetName() const         override { return "ProfilerSystem"; }

    // ── accessors ──────────────────────────────────────────────────────────
    GpuProfiler*       GetGpuProfiler()              { return m_gpuProfiler.get(); }
    const GpuProfiler* GetGpuProfiler()        const { return m_gpuProfiler.get(); }

    static constexpr uint32_t kFrameHistorySize = 60;

    const FrameRecord& GetFrameRecord(uint32_t index) const {
        return m_frameHistory[index % kFrameHistorySize];
    }
    uint32_t GetFrameIndex() const { return m_frameIndex; }

    // Cumulative CPU frame time for the current frame.
    float GetCurrentCpuFrameTime() const { return m_currentCpuFrameTimeMs; }

    /// Read all CPU samples from the main-thread profiler.
    uint32_t ReadMainThreadSamples(CpuSample* out, uint32_t max) const {
        return CpuProfiler::Get().ReadSamples(out, max);
    }

private:
    std::unique_ptr<GpuProfiler> m_gpuProfiler;

    // Frame history ring buffer
    std::array<FrameRecord, kFrameHistorySize> m_frameHistory{};
    uint32_t m_frameIndex = 0;

    // Current-frame accumulation
    CpuTimer  m_frameTimer;
    float     m_currentCpuFrameTimeMs = 0.0f;
    bool      m_frameTimerActive = false;
};

} // namespace Profiling
} // namespace Prisma
