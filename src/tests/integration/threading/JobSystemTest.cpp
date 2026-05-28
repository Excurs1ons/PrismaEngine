#include <gtest/gtest.h>
#include "threading/JobSystem.h"
#include <atomic>
#include <chrono>

namespace Prisma {
namespace {

class JobSystemTest : public ::testing::Test {
protected:
    JobSystem* m_jobSystem = nullptr;

    void SetUp() override {
        m_jobSystem = &Singleton<JobSystem>::Get();
        // 用 4 个线程创建通用线程池，确保并行性
        m_jobSystem->SetPoolConfig({{"TestPool", 4, 0}});
        m_jobSystem->Initialize();
    }

    void TearDown() override {
        m_jobSystem->Shutdown();
    }

    // 带超时的等待工具，防止死锁导致测试卡死
    bool WaitForJobsWithTimeout(JobSystem* js, int timeoutMs = 5000) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (js->GetTotalPendingJobCount() > 0) {
            js->WaitForAllJobs();
            if (js->GetTotalPendingJobCount() == 0) break;
            if (std::chrono::steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }
};

// 提交单个作业并等待完成
TEST_F(JobSystemTest, SubmitSingleJob) {
    std::atomic<int> counter{0};
    m_jobSystem->SubmitJob([&counter]() { counter.fetch_add(1); });
    EXPECT_TRUE(WaitForJobsWithTimeout(m_jobSystem));
    EXPECT_EQ(counter.load(), 1);
}

// 提交多个作业，验证所有作业都正确执行
TEST_F(JobSystemTest, SubmitMultipleJobs) {
    constexpr int kNumJobs = 100;
    std::atomic<int> counter{0};
    for (int i = 0; i < kNumJobs; ++i) {
        m_jobSystem->SubmitJob([&counter]() { counter.fetch_add(1); });
    }
    EXPECT_TRUE(WaitForJobsWithTimeout(m_jobSystem));
    EXPECT_EQ(counter.load(), kNumJobs);
}

// 验证线程池配置正确
TEST_F(JobSystemTest, PoolConfiguration) {
    EXPECT_EQ(m_jobSystem->GetThreadPoolCount(), 1u);
    // 提交作业前 GetPendingJobCount 应为 0
    EXPECT_EQ(m_jobSystem->GetPendingJobCount(0), 0u);
    EXPECT_EQ(m_jobSystem->GetTotalPendingJobCount(), 0u);
}

// 空作业队列应能立即返回，不会挂起
TEST_F(JobSystemTest, EmptyJobQueue) {
    m_jobSystem->WaitForAllJobs();
    EXPECT_EQ(m_jobSystem->GetTotalPendingJobCount(), 0u);
}

// 作业依赖：两个作业通过原子变量协调执行顺序
TEST_F(JobSystemTest, JobDependencyViaAtomic) {
    std::atomic<bool> jobADone{false};
    std::atomic<int> jobBCount{0};

    // 作业 A：设置标志
    m_jobSystem->SubmitJob([&jobADone]() {
        jobADone.store(true, std::memory_order_release);
    });

    // 作业 B：等待作业 A 完成后再执行
    m_jobSystem->SubmitJob([&jobADone, &jobBCount]() {
        while (!jobADone.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        jobBCount.fetch_add(1, std::memory_order_release);
    });

    EXPECT_TRUE(WaitForJobsWithTimeout(m_jobSystem));
    EXPECT_TRUE(jobADone.load());
    EXPECT_EQ(jobBCount.load(), 1);
}

// 运行时注册新线程池并提交作业
TEST_F(JobSystemTest, RegisterPoolAfterInit) {
    uint32_t poolIndex = m_jobSystem->RegisterPool("CustomPool", 2);
    EXPECT_EQ(poolIndex, 1u);
    EXPECT_EQ(m_jobSystem->GetThreadPoolCount(), 2u);

    std::atomic<int> counter{0};
    m_jobSystem->SubmitJob([&counter]() { counter.fetch_add(1); }, poolIndex);
    m_jobSystem->WaitForPool(poolIndex);
    EXPECT_EQ(counter.load(), 1);
}

// 提交大量短作业，验证线程池负载均衡
TEST_F(JobSystemTest, ConcurrentJobBurst) {
    constexpr int kNumJobs = 500;
    std::atomic<int> counter{0};

    for (int i = 0; i < kNumJobs; ++i) {
        m_jobSystem->SubmitJob([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    EXPECT_TRUE(WaitForJobsWithTimeout(m_jobSystem, 10000));
    EXPECT_EQ(counter.load(), kNumJobs);
}

} // namespace
} // namespace Prisma
