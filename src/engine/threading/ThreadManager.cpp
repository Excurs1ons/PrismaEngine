#include "ThreadManager.h"
#include "Logger.h"
#include <map>
#include <mutex>
#include <thread>

namespace Prisma {

std::shared_ptr<ThreadManager> ThreadManager::Get() {
    static std::shared_ptr<ThreadManager> instance = std::shared_ptr<ThreadManager>(new ThreadManager());
    return instance;
}

ThreadManager::ThreadManager() {
}

ThreadManager::~ThreadManager() {
    Shutdown();
}

int ThreadManager::Initialize() {
    LOG_DEBUG("Thread", "线程管理器初始化开始");
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadMetadata[std::this_thread::get_id()] = {"MainThread", 0, 0, false};
    return 0;
}

void ThreadManager::Shutdown() {
    LOG_DEBUG("Thread", "线程管理器开始关闭");
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_threads) {
        if (pair.second.joinable()) {
            pair.second.join();
        }
    }
    m_threads.clear();
    m_threadMetadata.clear();
}

std::thread ThreadManager::CreateThread(const std::string& name, std::function<void()> function) {
    std::thread thread([this, name, function = std::move(function)]() {
        const std::thread::id threadId = std::this_thread::get_id();
        SetThreadName(threadId, name);
        function();

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_threadMetadata.find(threadId);
        if (it != m_threadMetadata.end()) {
            it->second.finished = true;
        }
    });

    std::lock_guard<std::mutex> lock(m_mutex);
    const std::thread::id threadId = thread.get_id();
    m_threadMetadata[threadId] = {name, 0, 0, false};
    m_threads[threadId] = std::move(thread);
    return std::thread();
}

std::string ThreadManager::GetThreadName(std::thread::id id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_threadMetadata.find(id);
    if (it != m_threadMetadata.end()) {
        return it->second.name;
    }
    return "Unknown";
}

void ThreadManager::SetThreadName(std::thread::id id, const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadMetadata[id].name = name;
}

void ThreadManager::SetThreadAffinity(std::thread::id id, uint32_t coreMask) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadMetadata[id].affinityMask = coreMask;
}

void ThreadManager::SetThreadPriority(std::thread::id id, int priority) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadMetadata[id].priority = priority;
}

void ThreadManager::Update(Timestep ts) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_threads.begin(); it != m_threads.end(); ) {
        auto metadataIt = m_threadMetadata.find(it->first);
        if (metadataIt != m_threadMetadata.end() && metadataIt->second.finished) {
            if (it->second.joinable()) {
                it->second.join();
            }
            metadataIt->second.finished = true;
            it = m_threads.erase(it);
            continue;
        }
        ++it;
    }

    if (ts > 0.0f && !m_threadMetadata.empty()) {
        LOG_DEBUG("Thread", "线程管理器更新：{} 个已跟踪线程", m_threadMetadata.size());
    }
}

}  // namespace Prisma
