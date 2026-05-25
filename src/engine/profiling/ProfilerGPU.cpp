#include "ProfilerGPU.h"
#include <vulkan/vulkan.h>
#include <cstring>

namespace Prisma {
namespace Profiling {

// ─── GpuTimer ────────────────────────────────────────────────────────────────

GpuTimer::GpuTimer(GpuProfiler* profiler, const char* name, VkCommandBuffer cmd)
    : m_profiler(profiler)
    , m_name(name)
    , m_cmdBuffer(cmd) {
    if (!m_profiler || !m_profiler->IsAvailable() || !cmd) {
        m_active = false;
        return;
    }
    m_queryStartIdx = m_profiler->BeginSample(m_name, cmd);
    m_active = (m_queryStartIdx != ~0u);
}

GpuTimer::~GpuTimer() {
    if (!m_active) return;
    m_active = false;
    m_profiler->EndSample(m_queryStartIdx, m_cmdBuffer);
}

// ─── GpuProfiler ─────────────────────────────────────────────────────────────

GpuProfiler::GpuProfiler() = default;

GpuProfiler::~GpuProfiler() {
    Shutdown();
}

bool GpuProfiler::Initialize(VkDevice device) {
    if (!device) return false;

    m_device = device;

    VkQueryPoolCreateInfo poolInfo{};
    poolInfo.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    poolInfo.queryType          = VK_QUERY_TYPE_TIMESTAMP;
    poolInfo.queryCount         = kPoolSize;
    poolInfo.pipelineStatistics = 0;

    VkResult res = vkCreateQueryPool(m_device, &poolInfo, nullptr, &m_queryPool);
    if (res != VK_SUCCESS) {
        m_device    = nullptr;
        m_queryPool = nullptr;
        return false;
    }

    vkResetQueryPool(m_device, m_queryPool, 0, kPoolSize);

    m_available   = true;
    m_writeBatch  = 0;
    m_writeIndex  = 0;
    m_readCount   = 0;
    std::memset(m_activeNames, 0, sizeof(m_activeNames));
    return true;
}

void GpuProfiler::Shutdown() {
    if (!m_available || !m_device || !m_queryPool) return;

    vkDestroyQueryPool(m_device, m_queryPool, nullptr);
    m_queryPool = nullptr;
    m_device    = nullptr;
    m_available = false;
    m_readCount = 0;
}

void GpuProfiler::CollectResults() {
    if (!m_available) return;

    const uint32_t readBatch = 1 - m_writeBatch;
    const uint32_t readBatchSlots = m_writeIndex;

    if (readBatchSlots == 0) {
        m_readCount = 0;
        return;
    }

    const uint32_t timerCount = readBatchSlots / 2;
    if (timerCount == 0) {
        m_readCount = 0;
        return;
    }

    uint64_t timestamps[kBatchSize];
    VkResult res = vkGetQueryPoolResults(
        m_device,
        m_queryPool,
        readBatch * kBatchSize,
        readBatchSlots,
        readBatchSlots * sizeof(uint64_t),
        timestamps,
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    if (res != VK_SUCCESS) {
        m_readCount = 0;
        return;
    }

    m_readCount = (timerCount < kMaxTimersPerBatch) ? timerCount : kMaxTimersPerBatch;
    for (uint32_t i = 0; i < m_readCount; ++i) {
        GpuSample& s = m_readSamples[i];
        s.name           = m_activeNames[readBatch * kBatchSize + i * 2];
        s.startTimestamp = timestamps[i * 2];
        s.endTimestamp   = timestamps[i * 2 + 1];

        uint64_t delta = (s.endTimestamp > s.startTimestamp)
                             ? (s.endTimestamp - s.startTimestamp)
                             : 0ULL;

        s.durationMs = static_cast<float>(delta) / 1'000'000.0f;
        s.threadId   = 0;
        s.available  = true;
    }

    vkResetQueryPool(m_device, m_queryPool, readBatch * kBatchSize, readBatchSlots);
}

void GpuProfiler::NextFrame() {
    if (!m_available) return;

    m_writeIndex = 0;
    m_writeBatch = 1 - m_writeBatch;
}

uint32_t GpuProfiler::BeginSample(const char* name, VkCommandBuffer cmd) {
    if (!m_available || !cmd) return ~0u;

    if (m_writeIndex + 2 > kBatchSize) return ~0u;

    const uint32_t startSlot = PoolSlot(m_writeIndex);

    m_activeNames[startSlot] = name;
    m_writeIndex += 2;

    WriteTimestampStart(cmd, startSlot);
    return startSlot;
}

void GpuProfiler::EndSample(uint32_t queryStartIndex, VkCommandBuffer cmd) {
    if (!m_available || !cmd || queryStartIndex == ~0u) return;
    WriteTimestampEnd(cmd, queryStartIndex + 1);
}

void GpuProfiler::WriteTimestampStart(VkCommandBuffer cmd, uint32_t queryStartIndex) {
    if (!m_available || !cmd) return;
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_queryPool, queryStartIndex);
}

void GpuProfiler::WriteTimestampEnd(VkCommandBuffer cmd, uint32_t queryEndIndex) {
    if (!m_available || !cmd) return;
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_queryPool, queryEndIndex);
}

} // namespace Profiling
} // namespace Prisma
