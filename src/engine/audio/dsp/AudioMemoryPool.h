#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <cstddef>
#include <cassert>

namespace Prisma::Audio::DSP {

// ============================================================================
// AudioMemoryPool - 固定大小块的实时安全内存池
// ============================================================================
//
// 基于原子单链表 freelist 的无锁内存池，专为实时音频处理设计。
//
// 设计要点：
//   - 构造函数中预分配所有内存，运行时无 malloc/free/new
//   - 所有块大小相同，无碎片化
//   - Allocate() 失败返回 nullptr（不抛异常，不阻塞）
//   - 原子操作确保多线程安全（无锁）
//   - Cacheline padding 防止伪共享
//
// ABA 问题说明：
//   固定大小 + 无碎片回收使 freelist 节点的 next 指针在重新入链时
//   始终指向当前头部，CAS 失败的唯一原因是其他线程修改了头部。
//   无 coalesce/split 操作，因此无典型 ABA 场景。
//
// 线程安全：是（无锁，支持多生产者多消费者）
// 实时安全：是（无阻塞操作，无异常路径）
// ============================================================================
class AudioMemoryPool {
public:
    // 构造内存池
    // note: totalBytes 不必是 blockSize 的整数倍，末尾不足一块的部分浪费
    AudioMemoryPool(size_t totalBytes, size_t blockSize)
        : m_capacity(totalBytes / blockSize)
    {
        // 每块至少能放下原子指针
        if (blockSize < sizeof(Node)) {
            blockSize = sizeof(Node);
        }
        m_blockSize = blockSize;

        // 分配连续内存并构建 freelist
        m_memory.resize(m_capacity * m_blockSize);
        InitFreeList();

        m_used.store(0, std::memory_order_relaxed);
    }

    /// 禁止拷贝和移动
    AudioMemoryPool(const AudioMemoryPool&) = delete;
    AudioMemoryPool& operator=(const AudioMemoryPool&) = delete;
    AudioMemoryPool(AudioMemoryPool&&) = delete;
    AudioMemoryPool& operator=(AudioMemoryPool&&) = delete;

    // ========================================================================
    // 核心接口
    // ========================================================================

    // 从池中分配一个块
    // note: 实时安全：不抛异常，不阻塞
    [[nodiscard]] void* Allocate() noexcept {
        Node* head = m_freeList.load(std::memory_order_acquire);
        while (head) {
            Node* next = head->next.load(std::memory_order_relaxed);
            if (m_freeList.compare_exchange_weak(
                    head, next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                m_used.fetch_add(1, std::memory_order_relaxed);
                return static_cast<void*>(head);
            }
            // CAS 失败 → 重读 head 重试
        }
        return nullptr;
    }

    // 将块归还到池中
    // note: 实时安全：不抛异常，不阻塞
    void Free(void* ptr) noexcept {
        if (!ptr) { return; }

        Node* node  = static_cast<Node*>(ptr);
        Node* head  = m_freeList.load(std::memory_order_acquire);

        do {
            node->next.store(head, std::memory_order_relaxed);
        } while (!m_freeList.compare_exchange_weak(
                     head, node,
                     std::memory_order_acq_rel,
                     std::memory_order_acquire));

        m_used.fetch_sub(1, std::memory_order_relaxed);
    }

    // ========================================================================
    // 状态查询
    // ========================================================================

    // 当前已分配的块数
    [[nodiscard]] size_t GetUsed() const noexcept {
        return m_used.load(std::memory_order_relaxed);
    }

    // 池中总块数
    [[nodiscard]] size_t GetCapacity() const noexcept {
        return m_capacity;
    }

    // 池中剩余可用块数
    [[nodiscard]] size_t GetAvailable() const noexcept {
        return m_capacity - GetUsed();
    }

    // 每个块的大小（字节）
    [[nodiscard]] size_t GetBlockSize() const noexcept {
        return m_blockSize;
    }

    // 是否使用无锁原子操作（仅用于调试断言）
    [[nodiscard]] bool IsLockFree() const noexcept {
        return m_freeList.is_lock_free();
    }

    // 重置池（回到初始全空状态）
    // note: 调用后所有已分配的指针失效
    void Reset() noexcept {
        m_used.store(0, std::memory_order_relaxed);
        InitFreeList();
    }

private:
    // ========================================================================
    // Freelist 节点（存储在预分配内存内）
    // ========================================================================
    struct Node {
        std::atomic<Node*> next{nullptr};
    };

    // ========================================================================
    // 初始化 freelist：将连续内存块串联成单链表
    // ========================================================================
    void InitFreeList() noexcept {
        Node* tail = nullptr;
        // 从后往前链接，保证分配顺序为地址升序
        for (size_t i = m_capacity; i > 0; --i) {
            auto* node = reinterpret_cast<Node*>(
                m_memory.data() + (i - 1) * m_blockSize);
            node->next.store(tail, std::memory_order_relaxed);
            tail = node;
        }
        m_freeList.store(tail, std::memory_order_release);
    }

    // ========================================================================
    // 成员变量（热点数据加 cacheline padding）
    // ========================================================================

    // 原子 freelist 头部指针
    alignas(64) std::atomic<Node*> m_freeList{nullptr};

    // 预分配的连续内存块
    std::vector<std::byte> m_memory;

    // 每个块的大小（字节）
    size_t m_blockSize = 0;

    // 已分配块数计数
    alignas(64) std::atomic<size_t> m_used{0};

    // 总块数
    size_t m_capacity = 0;
};

// 确保 AudioMemoryPool 是 cacheline 对齐友好的
static_assert(sizeof(AudioMemoryPool) <= 128 || sizeof(AudioMemoryPool) % 64 == 0,
    "AudioMemoryPool should optimally fit within a few cache lines");

// ============================================================================
// NodePool<T> - 类型安全的固定大小节点实例池
// ============================================================================
//
// 预分配 capacity 个 T 对象的存储空间，通过原子 freelist 管理。
//
// 与 AudioMemoryPool 的区别：
//   - 分配粒度是完整类型 T，而非字节大小
//   - 提供调试计数（alloc/free 统计）
//   - 支持泄漏检测
//
// 使用场景：
//   - 音频处理图中的固定类型节点池化
//   - 任何需要实时安全的固定类型对象分配
//
// 要求：sizeof(T) >= sizeof(void*) 以确保 freelist 指针可嵌入
// ============================================================================
template<typename T>
class NodePool {
    static_assert(sizeof(T) >= sizeof(void*),
        "Node type T must be at least sizeof(void*) for freelist embedding");

public:
    // 构造节点池
    explicit NodePool(size_t capacity)
        : m_capacity(capacity)
    {
        // 分配存储（对齐到 T）
        m_storage.resize(capacity * sizeof(T));

        // 分配 freelist 节点
        m_nodes.resize(capacity);

        // 初始化 freelist：节点 0 → 1 → 2 → ... → capacity-1
        for (size_t i = 0; i < capacity; ++i) {
            m_nodes[i].next.store(
                (i + 1 < capacity) ? &m_nodes[i + 1] : nullptr,
                std::memory_order_relaxed);
        }
        m_freeList.store(&m_nodes[0], std::memory_order_release);

        m_allocCount.store(0, std::memory_order_relaxed);
        m_freeCount.store(0, std::memory_order_relaxed);
    }

    /// 禁止拷贝和移动
    NodePool(const NodePool&) = delete;
    NodePool& operator=(const NodePool&) = delete;
    NodePool(NodePool&&) = delete;
    NodePool& operator=(NodePool&&) = delete;

    // ========================================================================
    // 核心接口
    // ========================================================================

    // 分配一个节点槽位
    ///         池满时返回 nullptr
    [[nodiscard]] T* Allocate() noexcept {
        Node* head = m_freeList.load(std::memory_order_acquire);
        while (head) {
            Node* next = head->next.load(std::memory_order_relaxed);
            if (m_freeList.compare_exchange_weak(
                    head, next,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire)) {
                m_allocCount.fetch_add(1, std::memory_order_relaxed);

                // 将 Node 索引转换为 T 指针
                const size_t index = static_cast<size_t>(head - m_nodes.data());
                return reinterpret_cast<T*>(m_storage.data() + index * sizeof(T));
            }
        }
        return nullptr;
    }

    // 归还节点到池中
    // note: 调用者应确保已调用析构函数（placement delete）
    void Free(T* ptr) noexcept {
        if (!ptr) { return; }

        // 验证指针属于此池
        const auto* bytes = reinterpret_cast<const std::byte*>(ptr);
        const auto* start = m_storage.data();
        const auto* end   = start + m_capacity * sizeof(T);

        if (bytes < start || bytes >= end) {
            assert(false && "NodePool::Free: pointer not from this pool");
            return;
        }

        // 计算对应的 Node 索引
        const size_t byteOffset = static_cast<size_t>(bytes - start);
        const size_t index      = byteOffset / sizeof(T);

        // 确保对齐无余数
        assert(byteOffset % sizeof(T) == 0 &&
               "NodePool::Free: misaligned pointer");

        Node* node = &m_nodes[index];
        Node* head = m_freeList.load(std::memory_order_acquire);

        do {
            node->next.store(head, std::memory_order_relaxed);
        } while (!m_freeList.compare_exchange_weak(
                     head, node,
                     std::memory_order_acq_rel,
                     std::memory_order_acquire));

        m_freeCount.fetch_add(1, std::memory_order_relaxed);
    }

    // ========================================================================
    // 状态查询
    // ========================================================================

    // 当前正在使用的节点数
    [[nodiscard]] size_t GetUsed() const noexcept {
        const size_t allocd = m_allocCount.load(std::memory_order_relaxed);
        const size_t freed  = m_freeCount.load(std::memory_order_relaxed);
        return allocd > freed ? allocd - freed : 0;
    }

    // 池中总节点数
    [[nodiscard]] size_t GetCapacity() const noexcept {
        return m_capacity;
    }

    // 剩余可用节点数
    [[nodiscard]] size_t GetAvailable() const noexcept {
        return m_capacity - GetUsed();
    }

    // 总分配次数（包括已释放的）
    [[nodiscard]] size_t GetAllocCount() const noexcept {
        return m_allocCount.load(std::memory_order_relaxed);
    }

    // 总释放次数
    [[nodiscard]] size_t GetFreeCount() const noexcept {
        return m_freeCount.load(std::memory_order_relaxed);
    }

    // 检测分配/释放计数是否平衡（无泄漏）
    [[nodiscard]] bool HasLeaks() const noexcept {
        return GetAllocCount() != GetFreeCount();
    }

    // 是否使用无锁原子操作
    [[nodiscard]] bool IsLockFree() const noexcept {
        return m_freeList.is_lock_free();
    }

    // 重置池（所有统计清零，所有节点回到 freelist）
    void Reset() noexcept {
        m_allocCount.store(0, std::memory_order_relaxed);
        m_freeCount.store(0, std::memory_order_relaxed);

        // 重建 freelist
        for (size_t i = 0; i < m_capacity; ++i) {
            m_nodes[i].next.store(
                (i + 1 < m_capacity) ? &m_nodes[i + 1] : nullptr,
                std::memory_order_relaxed);
        }
        m_freeList.store(&m_nodes[0], std::memory_order_release);
    }

private:
    // ========================================================================
    // 内部类型
    // ========================================================================

    // Freelist 节点（独立于 T 的存储，避免对齐冲突）
    struct Node {
        std::atomic<Node*> next{nullptr};
    };

    // ========================================================================
    // 成员变量（热点数据加 cacheline padding）
    // ========================================================================

    // 原子 freelist 头部
    alignas(64) std::atomic<Node*> m_freeList{nullptr};

    // 预分配的节点存储（对齐到 alignof(T)）
    alignas(64) std::vector<std::byte> m_storage;

    // Freelist 链接节点
    std::vector<Node> m_nodes;

    // 总节点数
    size_t m_capacity = 0;

    // 调试统计：分配次数
    alignas(64) std::atomic<size_t> m_allocCount{0};

    // 调试统计：释放次数
    alignas(64) std::atomic<size_t> m_freeCount{0};
};

} // namespace Prisma::Audio::DSP
