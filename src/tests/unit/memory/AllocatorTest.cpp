#include <gtest/gtest.h>
#include "memory/StackAllocator.h"
#include "memory/PoolAllocator.h"
#include "memory/FrameAllocator.h"
#include "memory/MemoryManager.h"

using namespace Prisma::Memory;

// ---- StackAllocator ----

TEST(StackAllocator, AllocateAndReset) {
    StackAllocator alloc(1024);

    void* p1 = alloc.allocate(16, 4);
    EXPECT_NE(p1, nullptr);

    void* p2 = alloc.allocate(32, 8);
    EXPECT_NE(p2, nullptr);
    EXPECT_NE(p1, p2);

    auto marker = alloc.mark();
    void* p3 = alloc.allocate(64, 16);
    EXPECT_NE(p3, nullptr);

    alloc.rewind(marker);
    void* p4 = alloc.allocate(64, 16);
    EXPECT_EQ(p3, p4);

    alloc.clear();
    EXPECT_EQ(alloc.getUsed(), 0);

    StackAllocator small(16);
    void* pFail = small.allocate(32, 4);
    EXPECT_EQ(pFail, nullptr);
}

TEST(StackAllocator, Alignment) {
    // Use 32-byte alignment for the underlying buffer to support 32-byte aligned allocations
    StackAllocator alloc(256, 32);

    void* p1 = alloc.allocate(8, 4);
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p1) % 4, 0);

    void* p2 = alloc.allocate(16, 16);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p2) % 16, 0);

    void* p3 = alloc.allocate(32, 32);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p3) % 32, 0);

    // Offsets: p1=8 (align4→offset8), p2=16 (align16→offset32), p3=32 (align32→offset64)
    EXPECT_EQ(alloc.getUsed(), 64);
}

// ---- PoolAllocator ----

TEST(PoolAllocator, AllocateDeallocate) {
    constexpr size_t blockSize = 32;
    constexpr size_t numBlocks = 4;
    PoolAllocator pool(blockSize, numBlocks);

    EXPECT_EQ(pool.getBlockSize(), blockSize);
    EXPECT_EQ(pool.getAllocatedCount(), 0);
    EXPECT_EQ(pool.getFreeCount(), numBlocks);

    void* blocks[4];
    for (size_t i = 0; i < numBlocks; ++i) {
        blocks[i] = pool.allocate(blockSize, 4);
        EXPECT_NE(blocks[i], nullptr);
    }
    EXPECT_EQ(pool.getAllocatedCount(), numBlocks);
    EXPECT_EQ(pool.getFreeCount(), 0);

    void* overflow = pool.allocate(blockSize, 4);
    EXPECT_EQ(overflow, nullptr);

    for (size_t i = 0; i < numBlocks; ++i)
        pool.deallocate(blocks[i]);
    EXPECT_EQ(pool.getAllocatedCount(), 0);
}

TEST(PoolAllocator, Reuse) {
    constexpr size_t blockSize = 64;
    constexpr size_t numBlocks = 3;
    PoolAllocator pool(blockSize, numBlocks);

    void* p1 = pool.allocate(blockSize, 4);
    ASSERT_NE(p1, nullptr);
    pool.deallocate(p1);

    void* p2 = pool.allocate(blockSize, 4);
    ASSERT_NE(p2, nullptr);
    EXPECT_EQ(p1, p2);
}

// ---- FrameAllocator ----

TEST(FrameAllocator, DoubleBuffer) {
    FrameAllocator frame(256);

    EXPECT_TRUE(frame.isInitialized());
    EXPECT_EQ(frame.getCurrentOffset(), 0);

    void* p1 = frame.allocate(32, 4);
    ASSERT_NE(p1, nullptr);
    EXPECT_GT(frame.getCurrentOffset(), 0);

    frame.swapBuffers();
    EXPECT_EQ(frame.getCurrentOffset(), 0);

    void* p2 = frame.allocate(32, 4);
    ASSERT_NE(p2, nullptr);
    EXPECT_GT(frame.getCurrentOffset(), 0);

    frame.swapBuffers();
    EXPECT_EQ(frame.getCurrentOffset(), 0);
}

TEST(FrameAllocator, FrameBoundary) {
    FrameAllocator frame(128);

    void* p1 = frame.allocate(64, 16);
    ASSERT_NE(p1, nullptr);

    frame.swapBuffers();
    EXPECT_EQ(frame.getCurrentOffset(), 0);

    for (int i = 0; i < 10; ++i) {
        void* p = frame.allocate(16, 4);
        if (frame.getAvailableSpace() >= 16)
            EXPECT_NE(p, nullptr);
        frame.swapBuffers();
    }
}

// ---- MemoryManager ----

TEST(MemoryManager, Lifecycle) {
    MemoryManager& mgr1 = MemoryManager::Get();
    MemoryManager& mgr2 = MemoryManager::Get();
    EXPECT_EQ(&mgr1, &mgr2);

    PoolAllocator* pool = mgr1.GetPoolAllocator(64);
    ASSERT_NE(pool, nullptr);
    EXPECT_EQ(pool->getBlockSize(), 64);

    PoolAllocator* pool2 = mgr1.GetPoolAllocator(64);
    EXPECT_EQ(pool, pool2);

    PoolAllocator* pool3 = mgr1.GetPoolAllocator(128);
    EXPECT_NE(pool, pool3);

    FrameAllocator* frame = mgr1.GetFrameAllocator();
    ASSERT_NE(frame, nullptr);
    EXPECT_TRUE(frame->isInitialized());

    StackAllocator* stack = mgr1.GetStackAllocator();
    ASSERT_NE(stack, nullptr);
    EXPECT_TRUE(stack->isInitialized());
}

TEST(MemoryManager, Stats) {
    MemoryManager& mgr = MemoryManager::Get();

    AllocationStats stats = mgr.GetAllocationStats();
    EXPECT_GT(stats.totalAllocated, 0);

    PoolAllocator* pool = mgr.GetPoolAllocator(32);
    ASSERT_NE(pool, nullptr);

    AllocationStats before = mgr.GetAllocationStats();

    void* p = pool->allocate(32, 4);
    ASSERT_NE(p, nullptr);

    AllocationStats after = mgr.GetAllocationStats();
    EXPECT_GE(after.currentUsed, before.currentUsed);
    EXPECT_GE(after.allocationCount, before.allocationCount);
}
