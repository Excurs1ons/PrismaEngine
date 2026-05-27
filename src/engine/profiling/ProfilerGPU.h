#pragma once

#include "../Export.h"
#include <cstdint>
#include <cstring>
#include <string>

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
#include <vulkan/vulkan.h>
#else
// Stub definitions for non-Vulkan builds (allows header to compile).
typedef void* VkQueryPool;
typedef void* VkCommandBuffer;
typedef void* VkDevice;
#endif

namespace Prisma {
namespace Profiling {

// ─── GpuSample ──────────────────────────────────────────────────────────────
// Result of a completed GPU timer query.
//
struct GpuSample {
    const char* name            = nullptr;
    uint64_t    startTimestamp  = 0;   // raw GPU timestamp ticks
    uint64_t    endTimestamp    = 0;
    float       durationMs      = 0.0f;
    uint32_t    threadId        = 0;   // CPU thread that issued the query
    bool        available       = false;
};


// ─── GpuTimer (RAII) ────────────────────────────────────────────────────────
// Wraps a pair of VK_QUERY_TYPE_TIMESTAMP queries (start / end).
// Written to the GpuProfiler's currently-active batch.
//
class ENGINE_API GpuTimer {
public:
    GpuTimer(class GpuProfiler* profiler, const char* name, VkCommandBuffer cmd);
    ~GpuTimer();

    GpuTimer(const GpuTimer&)            = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;

private:
    class GpuProfiler* m_profiler       = nullptr;
    const char*        m_name           = nullptr;
    VkCommandBuffer    m_cmdBuffer      = nullptr;
    uint32_t           m_queryStartIdx  = ~0u;
    bool               m_active         = false;
};


// ─── GpuProfiler ────────────────────────────────────────────────────────────
// Manages a VkQueryPool (VK_QUERY_TYPE_TIMESTAMP) with 64 entries arranged in
// a ping-pong fashion: when batch 0 (slots 0–31) is being written, batch 1
// (slots 32–63) is being read, and vice versa.
//
// Gracefully handles a missing / null VkDevice (all operations become no-ops).
//
class ENGINE_API GpuProfiler {
public:
    static constexpr uint32_t kPoolSize  = 64;
    static constexpr uint32_t kBatchSize = kPoolSize / 2;   // 32
    static constexpr uint32_t kMaxTimersPerBatch = kBatchSize / 2;  // 16

    GpuProfiler();
    ~GpuProfiler();

    GpuProfiler(const GpuProfiler&)            = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

    /// Create the VkQueryPool.  Returns true on success.
    bool Initialize(VkDevice device);

    /// Destroy the VkQueryPool.
    void Shutdown();

    // ── per-frame lifecycle ─────────────────────────────────────────────────

    /// Read back results from the *other* (non-writing) batch.
    /// Must be called once per frame, before the writing batch is reused.
    void CollectResults();

    /// Swap write/read batches for the next frame.
    void NextFrame();

    // ── timer recording ─────────────────────────────────────────────────────

    /// Begin a GPU-timed scope and write the start timestamp into a command
    /// buffer.  Returns the query-pool index of the start timestamp.
    /// Pass this index + the same command buffer to EndSample().
    uint32_t BeginSample(const char* name, VkCommandBuffer cmd);

    /// End a GPU-timed scope by writing the end timestamp.
    void     EndSample(uint32_t queryStartIndex, VkCommandBuffer cmd);

    // ── helpers for manual command‑buffer recording ────────────────────────
    /// Write a start timestamp.  Slot obtained from BeginSample().
    void WriteTimestampStart(VkCommandBuffer cmd, uint32_t queryStartIndex);
    /// Write an end timestamp.  queryEndIndex = queryStartIndex + 1.
    void WriteTimestampEnd(VkCommandBuffer cmd, uint32_t queryEndIndex);

    // ── accessors ──────────────────────────────────────────────────────────

    VkQueryPool      GetQueryPool()    const { return m_queryPool; }
    bool             IsAvailable()     const { return m_available; }
    uint32_t         GetSampleCount()  const { return m_readCount; }
    const GpuSample* GetSamples()      const { return m_readSamples; }

private:
    // Map a timer-local index to a pool slot.
    uint32_t PoolSlot(uint32_t localIndex) const {
        return m_writeBatch * kBatchSize + localIndex;
    }

    VkQueryPool m_queryPool = nullptr;
    VkDevice    m_device    = nullptr;
    bool        m_available = false;

    // Ping-pong state
    uint32_t m_writeBatch = 0;        // 0 or 1
    uint32_t m_writeIndex = 0;        // next free slot in the write batch

    // Names of in-flight timers (indexed by their start-query slot).
    const char* m_activeNames[kBatchSize] = {};

    // Read-back results (from the completed batch).
    GpuSample  m_readSamples[kMaxTimersPerBatch];
    uint32_t   m_readCount = 0;
};

} // namespace Profiling
} // namespace Prisma
