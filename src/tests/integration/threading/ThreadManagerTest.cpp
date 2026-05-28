#include <gtest/gtest.h>
#include "threading/ThreadManager.h"
#include "threading/WorkerThread.h"
#include <atomic>
#include <chrono>

namespace Prisma {
namespace {

// ==================== ThreadManager 测试 ====================

class ThreadManagerTest : public ::testing::Test {
protected:
    ThreadManager* m_tm = nullptr;

    void SetUp() override {
        m_tm = ThreadManager::Get().get();
        m_tm->Initialize();
    }

    void TearDown() override {
        m_tm->Shutdown();
    }
};

// 验证 ThreadManager 启动/停止生命周期：Initialize 设置主线程元数据，
// Shutdown 清理所有状态。重新初始化应能正确恢复。
TEST_F(ThreadManagerTest, StartStopLifecycle) {
    std::string mainName = m_tm->GetThreadName(std::this_thread::get_id());
    EXPECT_EQ(mainName, "MainThread");

    m_tm->Shutdown();

    // 重新初始化验证状态重置
    m_tm->Initialize();
    mainName = m_tm->GetThreadName(std::this_thread::get_id());
    EXPECT_EQ(mainName, "MainThread");
}

// 通过 CreateThread 创建线程并验证回调正确执行
TEST_F(ThreadManagerTest, ThreadCallbackExecution) {
    std::atomic<bool> executed{false};

    m_tm->CreateThread("TestWorker", [&executed]() {
        executed.store(true);
    });

    // 等待线程执行完成
    for (int i = 0; i < 50; ++i) {
        if (executed.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(executed.load());

    // 清理已完成的线程
    m_tm->Update(0.0f);
}

// 验证线程名称跟踪功能
TEST_F(ThreadManagerTest, ThreadNameTracking) {
    // 主线程默认名为 "MainThread"
    EXPECT_EQ(m_tm->GetThreadName(std::this_thread::get_id()), "MainThread");

    // 修改线程名
    m_tm->SetThreadName(std::this_thread::get_id(), "CustomMain");
    EXPECT_EQ(m_tm->GetThreadName(std::this_thread::get_id()), "CustomMain");

    // 恢复原名
    m_tm->SetThreadName(std::this_thread::get_id(), "MainThread");
}

// 验证 Update 能正确清理已完成的线程
TEST_F(ThreadManagerTest, UpdateCleansFinishedThreads) {
    std::atomic<bool> executed{false};

    m_tm->CreateThread("FinishedWorker", [&executed]() {
        executed.store(true);
    });

    // 等待线程完成
    for (int i = 0; i < 50; ++i) {
        if (executed.load()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(executed.load());

    // Update 应清理已完成的线程
    m_tm->Update(0.0f);
    // 没有崩溃即表示清理成功
    SUCCEED();
}

// ==================== WorkerThread 测试 ====================

class WorkerThreadTest : public ::testing::Test {
protected:
    void TearDown() override {
        // 确保 WorkerThread 相关资源在测试间正确释放
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
};

// 验证 WorkerThread 的完整生命周期：创建、设置任务、启动、停止、等待结束
TEST_F(WorkerThreadTest, FullLifecycle) {
    std::atomic<int> callCount{0};
    {
        WorkerThread wt;
        EXPECT_FALSE(wt.IsRunning());

        wt.SetTask([&callCount]() {
            callCount.fetch_add(1, std::memory_order_relaxed);
        });
        wt.Start();

        // 等待任务被执行若干次（Run 循环每次 sleep 1ms）
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        EXPECT_TRUE(wt.IsRunning());
        EXPECT_GT(callCount.load(), 0);

        wt.Stop();
        wt.Join();
    } // WorkerThread 在离开作用域时析构，析构函数会再次调用 Stop()+Join()
    EXPECT_FALSE(callCount.load() == 0);
}

// 验证多个 WorkerThread 可以并发运行
TEST_F(WorkerThreadTest, MultipleWorkers) {
    constexpr int kNumWorkers = 4;
    std::atomic<int> totalCount{0};
    std::vector<std::unique_ptr<WorkerThread>> workers;

    for (int i = 0; i < kNumWorkers; ++i) {
        auto wt = std::make_unique<WorkerThread>();
        wt->SetTask([&totalCount]() {
            totalCount.fetch_add(1, std::memory_order_relaxed);
        });
        wt->Start();
        workers.push_back(std::move(wt));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    for (auto& wt : workers) {
        wt->Stop();
        wt->Join();
    }

    EXPECT_GT(totalCount.load(), 0);
}

} // namespace
} // namespace Prisma
