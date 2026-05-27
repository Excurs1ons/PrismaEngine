#pragma once

#include "MemoryAllocator.h"
#include "PoolAllocator.h"
#include "FrameAllocator.h"
#include "StackAllocator.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Prisma::Memory {

/// 内存管理器单例
/// 持有所有分配器实例，提供工厂方法
class MemoryManager {
public:
    static MemoryManager& Get() {
        static MemoryManager instance;
        return instance;
    }

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // ---- Pool Allocator 工厂 ----

    /// 获取或创建指定块大小的 PoolAllocator
    PoolAllocator* GetPoolAllocator(size_t blockSize, size_t maxBlocks = 1024) {
        auto it = m_PoolAllocators.find(blockSize);
        if (it != m_PoolAllocators.end()) {
            return it->second.get();
        }
        auto alloc = std::make_unique<PoolAllocator>(blockSize, maxBlocks);
        PoolAllocator* ptr = alloc.get();
        m_PoolAllocators.emplace(blockSize, std::move(alloc));
        return ptr;
    }

    /// 获取默认帧分配器
    FrameAllocator* GetFrameAllocator() {
        return m_FrameAllocator.get();
    }

    /// 获取默认栈分配器
    StackAllocator* GetStackAllocator() {
        return m_StackAllocator.get();
    }

    /// 设置自定义帧分配器
    void SetFrameAllocator(std::unique_ptr<FrameAllocator> allocator) {
        m_FrameAllocator = std::move(allocator);
    }

    /// 设置自定义栈分配器
    void SetStackAllocator(std::unique_ptr<StackAllocator> allocator) {
        m_StackAllocator = std::move(allocator);
    }

    /// 获取全局分配统计
    AllocationStats GetAllocationStats() const {
        AllocationStats total{};

        for (const auto& [size, pool] : m_PoolAllocators) {
            auto stats = pool->getAllocationStats();
            total.totalAllocated   += stats.totalAllocated;
            total.totalDeallocated += stats.totalDeallocated;
            total.currentUsed      += stats.currentUsed;
            total.allocationCount  += stats.allocationCount;
            total.deallocationCount += stats.deallocationCount;
        }

        if (m_FrameAllocator) {
            auto stats = m_FrameAllocator->getAllocationStats();
            total.currentUsed += stats.currentUsed;
            total.peakUsed     = std::max(total.peakUsed, stats.peakUsed);
            total.totalAllocated += stats.totalAllocated;
        }

        if (m_StackAllocator) {
            auto stats = m_StackAllocator->getAllocationStats();
            total.currentUsed += stats.currentUsed;
            total.peakUsed     = std::max(total.peakUsed, stats.peakUsed);
            total.totalAllocated += stats.totalAllocated;
        }

        return total;
    }

    /// 重置所有分配器
    void ResetAll() {
        for (auto& [size, pool] : m_PoolAllocators) {
            pool->reset();
        }
        if (m_FrameAllocator) m_FrameAllocator->reset();
        if (m_StackAllocator) m_StackAllocator->reset();
    }

    const std::string& GetName() const { return m_Name; }

private:
    MemoryManager()
        : m_FrameAllocator(std::make_unique<FrameAllocator>())
        , m_StackAllocator(std::make_unique<StackAllocator>())
    {
    }

    ~MemoryManager() = default;

    std::unordered_map<size_t, std::unique_ptr<PoolAllocator>> m_PoolAllocators;
    std::unique_ptr<FrameAllocator> m_FrameAllocator;
    std::unique_ptr<StackAllocator> m_StackAllocator;
    std::string m_Name = "MemoryManager";
};

} // namespace Prisma::Memory
