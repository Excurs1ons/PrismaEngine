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
    LOG_INFO("Thread", "线程管理器初始化开始");
    return 0;
}

void ThreadManager::Shutdown() {
    LOG_INFO("Thread", "线程管理器开始关闭");
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_threads) {
        if (pair.second.joinable()) {
            pair.second.join();
        }
    }
    m_threads.clear();
}

std::thread ThreadManager::CreateThread(const std::string& name, std::function<void()> function) {
    std::thread thread([this, name, function]() {
        SetThreadName(std::this_thread::get_id(), name);
        function();
    });
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads[thread.get_id()] = std::move(thread);
    return std::thread();
}

std::string ThreadManager::GetThreadName(std::thread::id id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_threadNames.find(id);
    if (it != m_threadNames.end()) {
        return it->second;
    }
    return "Unknown";
}

void ThreadManager::SetThreadName(std::thread::id id, const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadNames[id] = name;
}

void ThreadManager::SetThreadAffinity(std::thread::id /*id*/, uint32_t /*coreMask*/) {}
void ThreadManager::SetThreadPriority(std::thread::id /*id*/, int /*priority*/) {}

void ThreadManager::Update(Timestep /*ts*/) {}

}  // namespace Prisma
