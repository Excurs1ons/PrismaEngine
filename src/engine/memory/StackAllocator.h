#pragma once

#include "MemoryAllocator.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cassert>

namespace Prisma::Memory {

/// 栈分配器（标记/回退模式）
/// Marker = size_t（距基址的字节偏移）
class StackAllocator final : public MemoryAllocator {
public:
    using Marker = size_t;

    static constexpr size_t kDefaultCapacity = 1024 * 1024; // 1MB

    explicit StackAllocator(size_t capacity = kDefaultCapacity, size_t alignment = alignof(std::max_align_t))
        : m_Capacity(capacity)
        , m_Alignment(alignment)
        , m_Offset(0)
    {
        assert((m_Alignment & (m_Alignment - 1)) == 0 && "alignment must be power of 2");
        m_Memory = static_cast<uint8_t*>(std::aligned_alloc(m_Alignment, m_Capacity));
    }

    ~StackAllocator() override {
        reset();
    }

    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;

    StackAllocator(StackAllocator&& other) noexcept
        : m_Capacity(other.m_Capacity)
        , m_Alignment(other.m_Alignment)
        , m_Memory(other.m_Memory)
        , m_Offset(other.m_Offset)
    {
        other.m_Memory = nullptr;
        other.m_Offset = 0;
    }

    StackAllocator& operator=(StackAllocator&& other) noexcept {
        if (this != &other) {
            reset();
            m_Capacity = other.m_Capacity;
            m_Alignment = other.m_Alignment;
            m_Memory = other.m_Memory;
            m_Offset = other.m_Offset;
            other.m_Memory = nullptr;
            other.m_Offset = 0;
        }
        return *this;
    }

    [[nodiscard]] void* allocate(size_t size, size_t alignment) override {
        const size_t mask = alignment - 1;
        const size_t alignedOffset = (m_Offset + mask) & ~mask;

        if (alignedOffset + size > m_Capacity) {
            return nullptr;
        }

        m_Offset = alignedOffset + size;
        if (m_Offset > m_PeakOffset) m_PeakOffset = m_Offset;
        return static_cast<void*>(m_Memory + alignedOffset);
    }

    void deallocate([[maybe_unused]] void* ptr) override {
        // 栈分配器不支持任意顺序释放，使用 mark/rewind 代替
    }

    void reset() override {
        if (m_Memory) {
            std::free(m_Memory);
            m_Memory = nullptr;
        }
        m_Offset = 0;
    }

    /// 获取当前栈顶标记
    Marker mark() const {
        return m_Offset;
    }

    /// 回退到指定标记位置
    void rewind(Marker marker) {
        assert(marker <= m_Offset && "cannot rewind past current offset");
        m_Offset = marker;
    }

    /// 清除所有分配（重置偏移，保留内存）
    void clear() {
        m_Offset = 0;
    }

    MemoryType getMemoryType() const override { return MemoryType::Stack; }

    const char* getName() const override { return "StackAllocator"; }

    bool isInitialized() const override { return m_Memory != nullptr; }

    AllocationStats getAllocationStats() const override {
        AllocationStats stats{};
        stats.currentUsed = m_Offset;
        stats.peakUsed = m_PeakOffset;
        stats.totalAllocated = m_Capacity;
        return stats;
    }

    size_t getUsed() const { return m_Offset; }
    size_t getCapacity() const { return m_Capacity; }
    size_t getAvailable() const { return m_Capacity - m_Offset; }

private:
    size_t m_Capacity;
    size_t m_Alignment;
    uint8_t* m_Memory;
    size_t m_Offset;
    size_t m_PeakOffset = 0;
};

} // namespace Prisma::Memory
