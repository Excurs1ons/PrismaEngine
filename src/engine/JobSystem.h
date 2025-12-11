#pragma once
#include "ISubSystem.h"
#include "Singleton.h"
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
namespace Engine {

class JobSystem : public ISubSystem {
    friend class Singleton<JobSystem>;

public:
    bool Initialize() override;
    void Shutdown() override;
    using Job = std::function<void()>;

    // 提交作业到指定线程池
    void SubmitJob(Job job, uint32_t threadPoolIndex = 0);
    // 等待所有作业完成
    void WaitForAllJobs();

    // 获取线程池数量
    size_t GetThreadPoolCount() const { return m_threadPools.size(); }
    struct ThreadPool {
        std::vector<std::thread> threads;
        std::queue<Job> jobQueue;
        std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> running{false};

        void WorkerThread();
    };
    std::vector<std::unique_ptr<ThreadPool>> m_threadPools;
    std::atomic<uint32_t> m_jobCounter{0};
};

// 宏定义简化作业提交
#define SUBMIT_JOB(job) JobSystem::GetInstance().SubmitJob(job)
#define SUBMIT_JOB_TO_POOL(job, poolIndex) JobSystem::GetInstance().SubmitJob(job, poolIndex)
}  // namespace Engine