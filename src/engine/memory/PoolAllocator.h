#pragma once

#include "MemoryAllocator.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <cassert>

namespace Prisma::Memory {

/// 固定大小块分配器（侵入式空闲链表）
/// 空闲块自身存储 next 指针，无需额外链表节点
class PoolAllocator final : public MemoryAllocator {
public:
    PoolAllocator(size_t blockSize, size_t maxBlocks, size_t alignment = alignof(std::max_align_t))
        : m_BlockSize(blockSize)
        , m_MaxBlocks(maxBlocks)
        , m_Alignment(alignment)
        , m_Memory(nullptr)
        , m_FreeHead(nullptr)
        , m_AllocatedCount(0)
    {
        // 每个块至少能存储一个指针（用于空闲链表）
        const size_t minBlockSize = sizeof(void*);
        if (m_BlockSize < minBlockSize) {
            m_BlockSize = minBlockSize;
        }

        // 确保对齐是 2 的幂
        assert((m_Alignment & (m_Alignment - 1)) == 0 && "alignment must be power of 2");

        // 对齐 blockSize 到 alignment 边界
        m_BlockSize = (m_BlockSize + m_Alignment - 1) & ~(m_Alignment - 1);

        const size_t totalSize = m_BlockSize * m_MaxBlocks + m_Alignment;
        m_Memory = static_cast<uint8_t*>(std::aligned_alloc(m_Alignment, totalSize));
        if (!m_Memory) return;

        // 构建空闲链表
        uint8_t* aligned = alignPtr(m_Memory, m_Alignment);
        m_FreeHead = reinterpret_cast<FreeNode*>(aligned);
        FreeNode* current = m_FreeHead;

        for (size_t i = 1; i < m_MaxBlocks; ++i) {
            FreeNode* next = reinterpret_cast<FreeNode*>(aligned + i * m_BlockSize);
            current->next = next;
            current = next;
        }
        current->next = nullptr;
    }

    ~PoolAllocator() override {
        reset();
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    PoolAllocator(PoolAllocator&& other) noexcept
        : m_BlockSize(other.m_BlockSize)
        , m_MaxBlocks(other.m_MaxBlocks)
        , m_Alignment(other.m_Alignment)
        , m_Memory(other.m_Memory)
        , m_FreeHead(other.m_FreeHead)
        , m_AllocatedCount(other.m_AllocatedCount)
    {
        other.m_Memory = nullptr;
        other.m_FreeHead = nullptr;
        other.m_AllocatedCount = 0;
    }

    PoolAllocator& operator=(PoolAllocator&& other) noexcept {
        if (this != &other) {
            reset();
            m_BlockSize = other.m_BlockSize;
            m_MaxBlocks = other.m_MaxBlocks;
            m_Alignment = other.m_Alignment;
            m_Memory = other.m_Memory;
            m_FreeHead = other.m_FreeHead;
            m_AllocatedCount = other.m_AllocatedCount;
            other.m_Memory = nullptr;
            other.m_FreeHead = nullptr;
            other.m_AllocatedCount = 0;
        }
        return *this;
    }

    [[nodiscard]] void* allocate(size_t size, size_t alignment) override {
        if (!m_FreeHead) return nullptr;
        if (size > m_BlockSize) return nullptr;
        if (alignment > m_Alignment) return nullptr;

        FreeNode* block = m_FreeHead;
        m_FreeHead = m_FreeHead->next;
        ++m_AllocatedCount;
        return static_cast<void*>(block);
    }

    void deallocate(void* ptr) override {
        if (!ptr) return;

        FreeNode* block = static_cast<FreeNode*>(ptr);
        block->next = m_FreeHead;
        m_FreeHead = block;
        --m_AllocatedCount;
    }

    void reset() override {
        if (m_Memory) {
            std::free(m_Memory);
            m_Memory = nullptr;
        }
        m_FreeHead = nullptr;
        m_AllocatedCount = 0;
    }

    MemoryType getMemoryType() const override { return MemoryType::Pool; }

    const char* getName() const override { return "PoolAllocator"; }

    bool isInitialized() const override { return m_Memory != nullptr; }

    AllocationStats getAllocationStats() const override {
        AllocationStats stats{};
        stats.totalAllocated = m_AllocatedCount * m_BlockSize;
        stats.currentUsed = stats.totalAllocated;
        stats.allocationCount = m_AllocatedCount;
        return stats;
    }

    size_t getBlockSize() const { return m_BlockSize; }
    size_t getMaxBlocks() const { return m_MaxBlocks; }
    size_t getAllocatedCount() const { return m_AllocatedCount; }
    size_t getFreeCount() const { return m_MaxBlocks - m_AllocatedCount; }

private:
    struct FreeNode {
        FreeNode* next;
    };

    static uint8_t* alignPtr(uint8_t* ptr, size_t alignment) {
        const size_t mask = alignment - 1;
        const size_t offset = alignment - (reinterpret_cast<size_t>(ptr) & mask);
        if (offset == alignment) return ptr;
        return ptr + offset;
    }

    size_t m_BlockSize;
    size_t m_MaxBlocks;
    size_t m_Alignment;
    uint8_t* m_Memory;
    FreeNode* m_FreeHead;
    size_t m_AllocatedCount;
};

} // namespace Prisma::Memory
