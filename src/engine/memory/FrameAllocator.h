#pragma once

#include "MemoryAllocator.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <cassert>

namespace Prisma::Memory {

/// 双缓冲帧分配器
/// 两轮换缓冲区（写/帧 N，读/帧 N-1），每帧线性分配
class FrameAllocator final : public MemoryAllocator {
public:
    static constexpr size_t kDefaultBufferSize = 128 * 1024; // 128KB
    static constexpr size_t kDefaultAlignment = 16;

    explicit FrameAllocator(size_t bufferSize = kDefaultBufferSize, size_t alignment = kDefaultAlignment)
        : m_BufferSize(bufferSize)
        , m_Alignment(alignment)
        , m_CurrentBuffer(0)
    {
        assert((m_Alignment & (m_Alignment - 1)) == 0 && "alignment must be power of 2");

        for (size_t i = 0; i < 2; ++i) {
            m_Buffers[i] = static_cast<uint8_t*>(PRISMA_ALIGNED_ALLOC(m_Alignment, m_BufferSize));
            m_Offsets[i] = 0;
        }
    }

    ~FrameAllocator() override {
        reset();
    }

    FrameAllocator(const FrameAllocator&) = delete;
    FrameAllocator& operator=(const FrameAllocator&) = delete;

    FrameAllocator(FrameAllocator&& other) noexcept
        : m_BufferSize(other.m_BufferSize)
        , m_Alignment(other.m_Alignment)
        , m_CurrentBuffer(other.m_CurrentBuffer)
    {
        for (size_t i = 0; i < 2; ++i) {
            m_Buffers[i] = other.m_Buffers[i];
            m_Offsets[i] = other.m_Offsets[i];
            other.m_Buffers[i] = nullptr;
            other.m_Offsets[i] = 0;
        }
    }

    FrameAllocator& operator=(FrameAllocator&& other) noexcept {
        if (this != &other) {
            reset();
            m_BufferSize = other.m_BufferSize;
            m_Alignment = other.m_Alignment;
            m_CurrentBuffer = other.m_CurrentBuffer;
            for (size_t i = 0; i < 2; ++i) {
                m_Buffers[i] = other.m_Buffers[i];
                m_Offsets[i] = other.m_Offsets[i];
                other.m_Buffers[i] = nullptr;
                other.m_Offsets[i] = 0;
            }
        }
        return *this;
    }

    [[nodiscard]] void* allocate(size_t size, size_t alignment) override {
        uint8_t* buffer = m_Buffers[m_CurrentBuffer];
        size_t& offset = m_Offsets[m_CurrentBuffer];

        // 对齐当前偏移
        const size_t mask = alignment - 1;
        const size_t alignedOffset = (offset + mask) & ~mask;

        if (alignedOffset + size > m_BufferSize) {
            return nullptr; // 缓冲区耗尽
        }

        offset = alignedOffset + size;

        if (alignedOffset > m_PeakOffset) {
            m_PeakOffset = alignedOffset;
        }

        return static_cast<void*>(buffer + alignedOffset);
    }

    void deallocate([[maybe_unused]] void* ptr) override {
        // Frame 分配器不支持单个释放，通过 swapBuffers/reset 整块回收
    }

    void reset() override {
        for (size_t i = 0; i < 2; ++i) {
            if (m_Buffers[i]) {
                PRISMA_ALIGNED_FREE(m_Buffers[i]);
                m_Buffers[i] = nullptr;
            }
            m_Offsets[i] = 0;
        }
        m_PeakOffset = 0;
        m_CurrentBuffer = 0;
    }

    /// 重置当前缓冲区的写入位置（不切换）
    void resetCurrent() {
        m_Offsets[m_CurrentBuffer] = 0;
    }

    /// 交换读写缓冲区：重置旧写入端，切换到另一缓冲
    void swapBuffers() {
        const size_t prev = m_CurrentBuffer;
        m_CurrentBuffer = (m_CurrentBuffer + 1) % 2;
        m_Offsets[m_CurrentBuffer] = 0; // 新写入端从头开始
        m_PeakOffset = 0;
    }

    MemoryType getMemoryType() const override { return MemoryType::Frame; }

    const char* getName() const override { return "FrameAllocator"; }

    bool isInitialized() const override {
        return m_Buffers[0] != nullptr && m_Buffers[1] != nullptr;
    }

    AllocationStats getAllocationStats() const override {
        AllocationStats stats{};
        stats.currentUsed = m_Offsets[m_CurrentBuffer];
        stats.peakUsed = m_PeakOffset;
        stats.totalAllocated = m_BufferSize * 2;
        return stats;
    }

    size_t getBufferSize() const { return m_BufferSize; }
    size_t getCurrentOffset() const { return m_Offsets[m_CurrentBuffer]; }
    size_t getAvailableSpace() const { return m_BufferSize - m_Offsets[m_CurrentBuffer]; }

private:
    size_t m_BufferSize;
    size_t m_Alignment;
    uint8_t* m_Buffers[2];
    size_t m_Offsets[2];
    size_t m_CurrentBuffer;
    size_t m_PeakOffset = 0;
};

} // namespace Prisma::Memory
