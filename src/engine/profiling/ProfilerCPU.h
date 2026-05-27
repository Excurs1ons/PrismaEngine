#pragma once

#include "../Export.h"
#include <chrono>
#include <cstdint>
#include <thread>
#include <atomic>
#include <cstring>

namespace Prisma {
namespace Profiling {

// ─── CpuTimer ───────────────────────────────────────────────────────────────
// RAII-style CPU timer using std::chrono::high_resolution_clock.
//
class ENGINE_API CpuTimer {
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    CpuTimer() = default;

    /// Start / reset the timer and return the start timepoint.
    [[nodiscard]] TimePoint Start() {
        m_start = Clock::now();
        return m_start;
    }

    /// Stop the timer and return elapsed time in nanoseconds.
    [[nodiscard]] uint64_t Stop() {
        auto end = Clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count();
    }

    /// Elapsed time in nanoseconds (does NOT stop the timer).
    [[nodiscard]] uint64_t ElapsedNs() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now() - m_start).count();
    }

    /// Elapsed time in milliseconds.
    [[nodiscard]] float ElapsedMs() const {
        return ElapsedNs() / 1'000'000.0f;
    }

    [[nodiscard]] TimePoint GetStart() const { return m_start; }

    /// Reset without returning a value.
    void Reset() { m_start = Clock::now(); }

private:
    TimePoint m_start{};
};


// ─── CpuSample ──────────────────────────────────────────────────────────────
// A single CPU profiling sample stored in the ring buffer.
//
struct CpuSample {
    const char* name       = nullptr;   // pointer to a string literal
    uint64_t    startNs    = 0;         // absolute nanosecond timestamp
    uint64_t    durationNs = 0;         // duration in nanoseconds
    uint32_t    threadId   = 0;         // hashed std::thread::id
    uint32_t    depth      = 0;         // nesting depth (0 = top-level)
};


// ─── CpuProfiler ────────────────────────────────────────────────────────────
// Thread-local profiler with a fixed-size ring buffer (1024 samples).
// Oldest samples are overwritten when the buffer is full.
//
class ENGINE_API CpuProfiler {
public:
    static constexpr uint32_t kMaxSamples = 1024;

    /// Returns the thread-local CpuProfiler instance.
    static CpuProfiler& Get();

    /// Mark a frame boundary (increments internal frame counter).
    static void FrameMark();

    /// Push a completed sample into the ring buffer.
    void PushSample(const CpuSample& sample);

    // ── accessors ──────────────────────────────────────────────────────────
    uint32_t          GetCount()   const { return m_count; }
    uint32_t          GetHead()    const { return m_head; }
    uint32_t          GetThreadId() const { return m_threadId; }
    uint32_t          GetDepth()   const { return m_depth; }
    uint64_t          GetFrameCount() const { return m_frameCount; }

    const CpuSample&  GetSample(uint32_t index) const;

    /// Bulk-read all currently available samples into an external buffer.
    /// Returns the number of samples written (≤ maxCount).
    uint32_t ReadSamples(CpuSample* outBuffer, uint32_t maxCount) const;

    // ── depth management (called by ScopedTimer) ───────────────────────────
    void EnterScope() { ++m_depth; }
    void LeaveScope() { if (m_depth > 0) --m_depth; }

    CpuProfiler();
    ~CpuProfiler() = default;

private:
    CpuSample  m_samples[kMaxSamples];
    uint32_t   m_head      = 0;
    uint32_t   m_count     = 0;
    uint32_t   m_threadId  = 0;
    uint32_t   m_depth     = 0;
    uint64_t   m_frameCount = 0;
};


// ─── ScopedTimer (RAII) ────────────────────────────────────────────────────
// Starts a CPU timer on construction, pushes the sample to the thread-local
// CpuProfiler on destruction.
//
class ENGINE_API ScopedTimer {
public:
    explicit ScopedTimer(const char* name);
    ~ScopedTimer();

    ScopedTimer(const ScopedTimer&)            = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    /// Allow manual stop (the destructor will be a no-op after this).
    void Stop();

private:
    const char* m_name;
    CpuTimer    m_timer;
    bool        m_active = true;
};


// ─── inline / template implementations ─────────────────────────────────────

inline uint32_t CpuProfiler::ReadSamples(CpuSample* outBuffer, uint32_t maxCount) const {
    const uint32_t available = (m_count < kMaxSamples) ? m_count : kMaxSamples;
    const uint32_t toCopy    = (available < maxCount) ? available : maxCount;

    if (toCopy == 0) return 0;

    if (m_count < kMaxSamples) {
        // Buffer never wrapped – samples are at [0, count)
        std::memcpy(outBuffer, m_samples, toCopy * sizeof(CpuSample));
    } else {
        // Buffer wrapped – samples are at [head, end) and [0, head)
        const uint32_t tailCount = kMaxSamples - m_head;
        if (toCopy <= tailCount) {
            std::memcpy(outBuffer, m_samples + m_head, toCopy * sizeof(CpuSample));
        } else {
            std::memcpy(outBuffer, m_samples + m_head, tailCount * sizeof(CpuSample));
            std::memcpy(outBuffer + tailCount, m_samples, (toCopy - tailCount) * sizeof(CpuSample));
        }
    }
    return toCopy;
}

inline const CpuSample& CpuProfiler::GetSample(uint32_t index) const {
    // If the buffer never wrapped, samples are [0 .. count)
    if (m_count < kMaxSamples)
        return m_samples[index];

    // Otherwise the logical order is: [head .. end) + [0 .. head)
    const uint32_t physical = (m_head + index) % kMaxSamples;
    return m_samples[physical];
}

// ─── CpuProfiler static / member implementations ────────────────────────────

inline CpuProfiler::CpuProfiler()
    : m_threadId(static_cast<uint32_t>(
          std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFFFFFFu)) {
}

inline CpuProfiler& CpuProfiler::Get() {
    thread_local CpuProfiler instance;
    return instance;
}

inline void CpuProfiler::FrameMark() {
    Get().m_frameCount++;
}

inline void CpuProfiler::PushSample(const CpuSample& sample) {
    m_samples[m_head] = sample;
    m_head = (m_head + 1) % kMaxSamples;
    ++m_count;
}

// ─── ScopedTimer implementations ────────────────────────────────────────────

inline ScopedTimer::ScopedTimer(const char* name)
    : m_name(name) {
    auto& profiler = CpuProfiler::Get();
    profiler.EnterScope();
    m_timer.Start();
}

inline ScopedTimer::~ScopedTimer() {
    if (!m_active) return;
    m_active = false;
    uint64_t durationNs = m_timer.Stop();
    auto& profiler = CpuProfiler::Get();
    profiler.LeaveScope();

    CpuSample sample;
    sample.name       = m_name;
    sample.startNs    = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            m_timer.GetStart().time_since_epoch()).count();
    sample.durationNs = durationNs;
    sample.threadId   = profiler.GetThreadId();
    sample.depth      = profiler.GetDepth();
    profiler.PushSample(sample);
}

inline void ScopedTimer::Stop() {
    if (!m_active) return;
    m_active = false;
    uint64_t durationNs = m_timer.Stop();
    auto& profiler = CpuProfiler::Get();
    profiler.LeaveScope();

    CpuSample sample;
    sample.name       = m_name;
    sample.startNs    = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            m_timer.GetStart().time_since_epoch()).count();
    sample.durationNs = durationNs;
    sample.threadId   = profiler.GetThreadId();
    sample.depth      = profiler.GetDepth();
    profiler.PushSample(sample);
}

} // namespace Profiling
} // namespace Prisma
