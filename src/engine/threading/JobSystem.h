#pragma once
#include "ISubSystem.h"
#include "Singleton.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace Prisma {

struct PoolConfig {
    std::string name;
    uint32_t threadCount;  // 0 = auto
    int priority;
};

class JobSystem : public ISubSystem {
    friend class Singleton<JobSystem>;

public:
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "JobSystem"; }
    using Job = std::function<void()>;

    // 提交作业到指定线程池
    void SubmitJob(Job job, uint32_t threadPoolIndex = 0);
    // 等待所有作业完成
    void WaitForAllJobs();
    // 等待指定线程池的所有作业完成
    void WaitForPool(uint32_t poolIndex);

    // 线程池配置
    void SetPoolConfig(const std::vector<PoolConfig>& configs);
    uint32_t RegisterPool(const std::string& name, uint32_t threadCount = 0);

    // 查询
    size_t GetThreadPoolCount() const { return m_threadPools.size(); }
    size_t GetPendingJobCount(uint32_t poolIndex = 0) const;
    size_t GetTotalPendingJobCount() const;

private:
    struct ThreadPool {
        std::vector<std::thread> threads;
        std::queue<Job> jobQueue;
        mutable std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> running{false};
        std::atomic<uint32_t> activeJobs{0};
        std::string name;
        uint32_t poolIndex;

        void WorkerThread(uint32_t poolIndex, class JobSystem* jobSystem);
    };

    std::mutex m_configMutex;
    std::vector<PoolConfig> m_pendingPoolConfigs;
    bool m_initialized{false};
    size_t GetTotalThreadCount() const;
    std::vector<std::unique_ptr<ThreadPool>> m_threadPools;
    std::atomic<uint32_t> m_jobCounter{0};
};

// 宏定义简化作业提交
#define SUBMIT_JOB(job) ::Prisma::Engine::Get().GetJobSystem()->SubmitJob(job)
#define SUBMIT_JOB_TO_POOL(job, poolIndex) ::Prisma::Engine::Get().GetJobSystem()->SubmitJob(job, poolIndex)
}  // namespace Prisma
