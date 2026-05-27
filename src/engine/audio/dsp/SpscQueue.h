#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Prisma::Audio::DSP {

// ============================================================================
// AudioCommand — 跨线程音频命令（无锁传递）
// ============================================================================
struct AudioCommand {
    enum Type : uint8_t {
        NodeCreate,
        NodeRemove,
        NodeConnect,
        NodeDisconnect,
        ParamChange,
        PlayClip,
        StopClip,
        SetVolume,
        SetPan,
        SetPitch
    };

    Type type;
    uint64_t nodeId;
    uint64_t targetId;
    uint32_t paramIndex;

    union {
        float floatValue;
        uint32_t uintValue;
        int32_t intValue;
    };
};

static_assert(std::is_trivial_v<AudioCommand>, "AudioCommand must be trivial for lock-free transfer");

// ============================================================================
// SpscQueue — 单生产者单消费者无锁环形队列
// ============================================================================
template<typename T, size_t Capacity = 256>
class SpscQueue {
    static_assert(Capacity > 0, "Capacity must be greater than 0");

public:
    SpscQueue() = default;

    // 非阻塞推入 — 生产者线程调用
    // 返回 true 表示成功，false 表示队列满
    bool TryPush(const T& item) noexcept {
        // 仅生产者写入 m_head，此处读取 m_tail（消费者写入）
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t tail = m_tail.load(std::memory_order_acquire);

        if ((head - tail) >= Capacity) {
            return false;
        }

        m_buffer[head % Capacity] = item;

        // release：保证 m_buffer 写入在 m_head 递增前对消费者可见
        m_head.store(head + 1, std::memory_order_release);
        return true;
    }

    // 非阻塞弹出 — 消费者线程调用
    // 返回 true 表示成功，false 表示队列空
    bool TryPop(T& item) noexcept {
        // 仅消费者写入 m_tail，此处读取 m_head（生产者写入）
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        const size_t head = m_head.load(std::memory_order_acquire);

        if (head == tail) {
            return false;
        }

        item = m_buffer[tail % Capacity];

        // release：保证 m_buffer 读取在 m_tail 递增前完成
        // 同时通知生产者可以覆写此槽位
        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    }

    // 近似大小（仅调试/统计用，非精确线程安全）
    [[nodiscard]] size_t Size() const noexcept {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        return head - tail;
    }

    [[nodiscard]] bool IsEmpty() const noexcept {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        return head == tail;
    }

    [[nodiscard]] bool IsFull() const noexcept {
        const size_t head = m_head.load(std::memory_order_relaxed);
        const size_t tail = m_tail.load(std::memory_order_relaxed);
        return (head - tail) >= Capacity;
    }

    // 清空队列 — 仅消费者安全调用
    void Reset() noexcept {
        const size_t head = m_head.load(std::memory_order_acquire);
        m_tail.store(head, std::memory_order_release);
    }

private:
    // head/tail 分别对齐到 cacheline 边界以防止伪共享
    alignas(64) std::atomic<size_t> m_head{0};
    alignas(64) std::atomic<size_t> m_tail{0};

    std::array<T, Capacity> m_buffer{};
};

} // namespace Prisma::Audio::DSP
