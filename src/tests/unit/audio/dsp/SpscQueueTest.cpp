#include <gtest/gtest.h>
#include <audio/dsp/SpscQueue.h>

#include <thread>
#include <vector>
#include <atomic>

using namespace Prisma::Audio::DSP;

// ============================================================================
// 基本操作
// ============================================================================

TEST(SpscQueueTest, InitiallyEmpty) {
    SpscQueue<int, 64> q;
    EXPECT_TRUE(q.IsEmpty());
    EXPECT_FALSE(q.IsFull());
    EXPECT_EQ(q.Size(), 0);
}

TEST(SpscQueueTest, PushAndPop) {
    SpscQueue<int, 64> q;
    EXPECT_TRUE(q.TryPush(42));
    EXPECT_FALSE(q.IsEmpty());
    EXPECT_EQ(q.Size(), 1);

    int val = 0;
    EXPECT_TRUE(q.TryPop(val));
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(q.IsEmpty());
    EXPECT_EQ(q.Size(), 0);
}

TEST(SpscQueueTest, PushMultipleAndPop) {
    SpscQueue<int, 64> q;
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(q.TryPush(i * 10));
    }
    EXPECT_EQ(q.Size(), 10);

    for (int i = 0; i < 10; ++i) {
        int val = -1;
        EXPECT_TRUE(q.TryPop(val));
        EXPECT_EQ(val, i * 10);
    }
    EXPECT_TRUE(q.IsEmpty());
}

TEST(SpscQueueTest, FIFOOrder) {
    SpscQueue<int, 64> q;
    constexpr int kCount = 32;
    for (int i = 0; i < kCount; ++i) {
        EXPECT_TRUE(q.TryPush(i));
    }
    for (int i = 0; i < kCount; ++i) {
        int val = -1;
        EXPECT_TRUE(q.TryPop(val));
        EXPECT_EQ(val, i);
    }
}

// ============================================================================
// 边界行为：满队列 / 空队列
// ============================================================================

TEST(SpscQueueTest, PushUntilFull) {
    SpscQueue<int, 8> q;
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_TRUE(q.TryPush(static_cast<int>(i)));
    }
    EXPECT_TRUE(q.IsFull());
    EXPECT_EQ(q.Size(), 8);

    // Next push should fail
    EXPECT_FALSE(q.TryPush(99));
    EXPECT_TRUE(q.IsFull());
}

TEST(SpscQueueTest, PopFromEmpty) {
    SpscQueue<int, 8> q;
    int val = 0;
    EXPECT_FALSE(q.TryPop(val));
    EXPECT_TRUE(q.IsEmpty());
}

TEST(SpscQueueTest, FullThenPopThenPush) {
    SpscQueue<int, 4> q;
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(q.TryPush(i));
    }
    EXPECT_TRUE(q.IsFull());

    int val;
    EXPECT_TRUE(q.TryPop(val));
    EXPECT_EQ(val, 0);
    EXPECT_FALSE(q.IsFull());
    EXPECT_EQ(q.Size(), 3);

    EXPECT_TRUE(q.TryPush(100));
    EXPECT_TRUE(q.IsFull());
    EXPECT_EQ(q.Size(), 4);
}

// ============================================================================
// Size 跟踪
// ============================================================================

TEST(SpscQueueTest, SizeTracking) {
    SpscQueue<int, 16> q;
    EXPECT_EQ(q.Size(), 0);

    q.TryPush(1);
    EXPECT_EQ(q.Size(), 1);

    q.TryPush(2);
    EXPECT_EQ(q.Size(), 2);

    int val;
    q.TryPop(val);
    EXPECT_EQ(q.Size(), 1);

    q.TryPop(val);
    EXPECT_EQ(q.Size(), 0);
}

// ============================================================================
// Reset
// ============================================================================

TEST(SpscQueueTest, Reset) {
    SpscQueue<int, 8> q;
    q.TryPush(1);
    q.TryPush(2);
    q.TryPush(3);
    EXPECT_EQ(q.Size(), 3);

    q.Reset();
    EXPECT_TRUE(q.IsEmpty());
    EXPECT_EQ(q.Size(), 0);

    // After reset, should be able to push again
    EXPECT_TRUE(q.TryPush(99));
    int val = 0;
    EXPECT_TRUE(q.TryPop(val));
    EXPECT_EQ(val, 99);
}

// ============================================================================
// 容量验证
// ============================================================================

TEST(SpscQueueTest, CapacityBoundary) {
    SpscQueue<int, 1> q;
    EXPECT_TRUE(q.IsEmpty());
    EXPECT_FALSE(q.IsFull());

    EXPECT_TRUE(q.TryPush(1));
    EXPECT_TRUE(q.IsFull());
    EXPECT_FALSE(q.IsEmpty());

    int val = 0;
    EXPECT_TRUE(q.TryPop(val));
    EXPECT_EQ(val, 1);
    EXPECT_TRUE(q.IsEmpty());
}

TEST(SpscQueueTest, LargeCapacity) {
    constexpr size_t kCapacity = 1024;
    SpscQueue<int, kCapacity> q;
    EXPECT_FALSE(q.IsFull());

    for (size_t i = 0; i < kCapacity; ++i) {
        EXPECT_TRUE(q.TryPush(static_cast<int>(i)));
    }
    EXPECT_TRUE(q.IsFull());

    for (size_t i = 0; i < kCapacity; ++i) {
        int val = -1;
        EXPECT_TRUE(q.TryPop(val));
        EXPECT_EQ(val, static_cast<int>(i));
    }
    EXPECT_TRUE(q.IsEmpty());
}

// ============================================================================
// AudioCommand 类型验证
// ============================================================================

TEST(SpscQueueTest, AudioCommandPushPop) {
    SpscQueue<AudioCommand, 32> q;

    AudioCommand cmd;
    cmd.type     = AudioCommand::NodeCreate;
    cmd.nodeId   = 1;
    cmd.floatValue = 0.5f;

    EXPECT_TRUE(q.TryPush(cmd));

    AudioCommand popped{};
    EXPECT_TRUE(q.TryPop(popped));
    EXPECT_EQ(popped.type, AudioCommand::NodeCreate);
    EXPECT_EQ(popped.nodeId, 1);
    EXPECT_FLOAT_EQ(popped.floatValue, 0.5f);
}

TEST(SpscQueueTest, AudioCommandAllTypes) {
    SpscQueue<AudioCommand, 16> q;

    auto testType = [&](AudioCommand::Type type, uint64_t nodeId, float val) {
        AudioCommand cmd;
        cmd.type = type;
        cmd.nodeId = nodeId;
        cmd.floatValue = val;
        EXPECT_TRUE(q.TryPush(cmd));

        AudioCommand popped{};
        EXPECT_TRUE(q.TryPop(popped));
        EXPECT_EQ(popped.type, type);
        EXPECT_EQ(popped.nodeId, nodeId);
        EXPECT_FLOAT_EQ(popped.floatValue, val);
    };

    testType(AudioCommand::NodeCreate,    1, 0.0f);
    testType(AudioCommand::NodeRemove,    2, 0.0f);
    testType(AudioCommand::NodeConnect,   3, 0.0f);
    testType(AudioCommand::NodeDisconnect, 4, 0.0f);
    testType(AudioCommand::ParamChange,   5, 0.75f);
    testType(AudioCommand::PlayClip,      6, 0.0f);
    testType(AudioCommand::StopClip,      7, 0.0f);
    testType(AudioCommand::SetVolume,     8, -6.0f);
    testType(AudioCommand::SetPan,        9, 0.5f);
    testType(AudioCommand::SetPitch,     10, 1.0f);
}

// ============================================================================
// 线程安全基础验证（单生产者→单消费者）
// ============================================================================

TEST(SpscQueueTest, SingleProducerSingleConsumer) {
    SpscQueue<int, 128> q;
    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};
    constexpr size_t kTotal = 10000;

    std::thread producer([&]() {
        for (size_t i = 0; i < kTotal; ++i) {
            while (!q.TryPush(static_cast<int>(i))) {
                std::this_thread::yield();
            }
            produced++;
        }
    });

    std::thread consumer([&]() {
        size_t count = 0;
        int expected = 0;
        while (count < kTotal) {
            int val;
            if (q.TryPop(val)) {
                EXPECT_EQ(val, expected);
                expected++;
                count++;
                consumed++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(produced.load(), kTotal);
    EXPECT_EQ(consumed.load(), kTotal);
    EXPECT_TRUE(q.IsEmpty());
}

// ============================================================================
// IsFull / IsEmpty 循环验证
// ============================================================================

TEST(SpscQueueTest, IsFullIsEmptyCycle) {
    SpscQueue<int, 4> q;

    EXPECT_TRUE(q.IsEmpty());
    EXPECT_FALSE(q.IsFull());

    q.TryPush(1);
    EXPECT_FALSE(q.IsEmpty());
    EXPECT_FALSE(q.IsFull());

    q.TryPush(2);
    q.TryPush(3);
    q.TryPush(4);
    EXPECT_FALSE(q.IsEmpty());
    EXPECT_TRUE(q.IsFull());

    int val;
    q.TryPop(val);
    EXPECT_FALSE(q.IsFull());

    q.TryPop(val);
    q.TryPop(val);
    EXPECT_FALSE(q.IsFull());
    EXPECT_FALSE(q.IsEmpty());

    q.TryPop(val);
    EXPECT_TRUE(q.IsEmpty());
    EXPECT_FALSE(q.IsFull());
}

// ============================================================================
// 队列复用（多次 fill-drain 循环）
// ============================================================================

TEST(SpscQueueTest, MultipleFillDrainCycles) {
    SpscQueue<int, 8> q;
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Fill
        for (int i = 0; i < 8; ++i) {
            EXPECT_TRUE(q.TryPush(cycle * 100 + i));
        }
        EXPECT_TRUE(q.IsFull());

        // Drain
        for (int i = 0; i < 8; ++i) {
            int val = -1;
            EXPECT_TRUE(q.TryPop(val));
            EXPECT_EQ(val, cycle * 100 + i);
        }
        EXPECT_TRUE(q.IsEmpty());
    }
}
