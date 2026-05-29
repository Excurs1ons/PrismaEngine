#include "JobSystem.h"
#include "Logger.h"
#include "ThreadManager.h"

namespace Prisma {

int JobSystem::Initialize() {
    LOG_DEBUG("JobSystem", "初始化开始");

    std::vector<PoolConfig> configs;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        if (!m_pendingPoolConfigs.empty()) {
            configs = std::move(m_pendingPoolConfigs);
            m_pendingPoolConfigs.clear();
        }
    }

    if (configs.empty()) {
        uint32_t totalThreads = std::max(1u, std::thread::hardware_concurrency() - 2u);
        configs.push_back({"General", totalThreads, 0});
    }

    for (size_t i = 0; i < configs.size(); ++i) {
        auto& cfg = configs[i];
        auto pool = std::make_unique<ThreadPool>();
        pool->name = cfg.name;
        pool->poolIndex = static_cast<uint32_t>(i);
        pool->running.store(true);

        uint32_t threadCount = cfg.threadCount;
        if (threadCount == 0) {
            if (i == 0) {
                threadCount = std::max(1u, std::thread::hardware_concurrency() - 2u);
            } else {
                threadCount = 1;
            }
        }

        // 先插入向量，确保 WorkerThread 能访问到有效的 m_threadPools[poolIndex]
        uint32_t poolIndex = static_cast<uint32_t>(i);
        m_threadPools.push_back(std::move(pool));

        for (uint32_t t = 0; t < threadCount; ++t) {
            m_threadPools.back()->threads.emplace_back([this, poolIndex]() {
                std::string threadName = "JobPool" + std::to_string(poolIndex) + "_Worker";
                ThreadManager::Get()->SetThreadName(std::this_thread::get_id(), threadName);
                m_threadPools[poolIndex]->WorkerThread(poolIndex, this);
            });
        }

        LOG_INFO("JobSystem", "线程池 {0} '{1}' 创建: {2} 个线程", i, cfg.name, threadCount);
    }

    m_initialized.store(true, std::memory_order_release);
    LOG_INFO("JobSystem", "初始化完成: {0} 个池, {1} 个总线程",
             m_threadPools.size(), GetTotalThreadCount());
    return 0;
}

void JobSystem::Shutdown() {
    LOG_DEBUG("JobSystem", "关闭开始");
    m_initialized.store(false, std::memory_order_release);

    for (auto& pool : m_threadPools) {
        pool->running.store(false, std::memory_order_release);
        pool->condition.notify_all();
    }

    for (auto& pool : m_threadPools) {
        for (auto& thread : pool->threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        {
            std::lock_guard<std::mutex> lock(pool->queueMutex);
            while (!pool->jobQueue.empty()) {
                pool->jobQueue.pop();
            }
        }
    }

    // 清零计数器（Worker 可能在退出前来不及递减）
    for (auto& pool : m_threadPools) {
        pool->activeJobs.store(0, std::memory_order_release);
    }
    m_jobCounter.store(0, std::memory_order_release);

    m_threadPools.clear();
    LOG_INFO("JobSystem", "关闭完成");
}

void JobSystem::Update(Timestep ts) {
    (void)ts;
}

void JobSystem::SubmitJob(Job job, uint32_t threadPoolIndex) {
    if (!job) {
        LOG_WARNING("JobSystem", "提交了一个空的作业");
        return;
    }

    if (!m_initialized.load(std::memory_order_acquire) || threadPoolIndex >= m_threadPools.size()) {
        job();
        return;
    }

    auto& pool = m_threadPools[threadPoolIndex];
    {
        std::lock_guard<std::mutex> lock(pool->queueMutex);
        pool->jobQueue.push(std::move(job));
        pool->activeJobs.fetch_add(1, std::memory_order_release);
        m_jobCounter.fetch_add(1, std::memory_order_release);
    }
    pool->condition.notify_one();
}

void JobSystem::WaitForAllJobs() {
    if (m_threadPools.empty()) return;

    while (m_jobCounter.load(std::memory_order_acquire) > 0) {
        bool foundActive = false;
        for (auto& pool : m_threadPools) {
            if (pool->activeJobs.load(std::memory_order_acquire) > 0) {
                foundActive = true;
                std::unique_lock<std::mutex> lock(pool->queueMutex);
                pool->condition.wait_for(lock, std::chrono::milliseconds(10), [this, &pool]() {
                    return pool->activeJobs.load(std::memory_order_acquire) == 0
                        || !m_initialized.load(std::memory_order_acquire);
                });
            }
        }
        if (!foundActive && m_jobCounter.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
    }
}

void JobSystem::WaitForPool(uint32_t poolIndex) {
    if (poolIndex >= m_threadPools.size()) return;

    auto& pool = m_threadPools[poolIndex];
    while (pool->activeJobs.load(std::memory_order_acquire) > 0) {
        std::unique_lock<std::mutex> lock(pool->queueMutex);
        pool->condition.wait_for(lock, std::chrono::milliseconds(10), [&pool]() {
            return pool->activeJobs.load(std::memory_order_acquire) == 0;
        });
    }
}

void JobSystem::SetPoolConfig(const std::vector<PoolConfig>& configs) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_pendingPoolConfigs = configs;
}

uint32_t JobSystem::RegisterPool(const std::string& name, uint32_t threadCount) {
    std::lock_guard<std::mutex> lock(m_configMutex);

    if (!m_initialized.load(std::memory_order_acquire)) {
        m_pendingPoolConfigs.push_back({name, threadCount, 0});
        return static_cast<uint32_t>(m_pendingPoolConfigs.size() - 1);
    }

    uint32_t poolIndex = static_cast<uint32_t>(m_threadPools.size());
    auto pool = std::make_unique<ThreadPool>();
    pool->name = name;
    pool->poolIndex = poolIndex;
    pool->running.store(true);

    uint32_t count = (threadCount > 0) ? threadCount : 1;

    // 先插入向量，确保 WorkerThread 能访问到有效的 m_threadPools[poolIndex]
    m_threadPools.push_back(std::move(pool));

    for (uint32_t t = 0; t < count; ++t) {
        m_threadPools.back()->threads.emplace_back([this, poolIndex]() {
            std::string threadName = "JobPool" + std::to_string(poolIndex) + "_Worker";
            ThreadManager::Get()->SetThreadName(std::this_thread::get_id(), threadName);
            m_threadPools[poolIndex]->WorkerThread(poolIndex, this);
        });
    }

    LOG_INFO("JobSystem", "运行时注册新池 {0} '{1}': {2} 个线程", poolIndex, name, count);
    return poolIndex;
}

size_t JobSystem::GetPendingJobCount(uint32_t poolIndex) const {
    if (poolIndex >= m_threadPools.size()) return 0;
    std::lock_guard<std::mutex> lock(m_threadPools[poolIndex]->queueMutex);
    return m_threadPools[poolIndex]->jobQueue.size();
}

size_t JobSystem::GetTotalPendingJobCount() const {
    return m_jobCounter.load(std::memory_order_acquire);
}

size_t JobSystem::GetTotalThreadCount() const {
    size_t total = 0;
    for (auto& pool : m_threadPools) {
        total += pool->threads.size();
    }
    return total;
}

void JobSystem::ThreadPool::WorkerThread(uint32_t poolIdx, JobSystem* jobSystem) {
    while (running.load(std::memory_order_acquire)) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]() {
                return !jobQueue.empty() || !running.load(std::memory_order_acquire);
            });

            if (!running.load(std::memory_order_acquire)) break;
            if (jobQueue.empty()) continue;

            job = std::move(jobQueue.front());
            jobQueue.pop();
        }

        if (job) {
            try {
                job();
            } catch (const std::exception& e) {
                LOG_ERROR("JobSystem", "池 {0} Worker 任务异常: {1}", poolIdx, e.what());
            } catch (...) {
                LOG_ERROR("JobSystem", "池 {0} Worker 未知异常", poolIdx);
            }
        }

        activeJobs.fetch_sub(1, std::memory_order_release);
        if (jobSystem) {
            jobSystem->m_jobCounter.fetch_sub(1, std::memory_order_release);
        }
        condition.notify_all();
    }
}

}  // namespace Prisma
