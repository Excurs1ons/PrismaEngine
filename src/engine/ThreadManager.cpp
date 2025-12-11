#include "ThreadManager.h"
#include "Logger.h"
#include <map>
#include <mutex>
#include <thread>

namespace Engine {

bool ThreadManager::Initialize() {
    LOG_INFO("Thread", "线程管理器初始化开始");
    
    // 初始化线程管理器
    // 例如：设置线程池、初始化同步原语等
    
    LOG_INFO("Thread", "线程管理器初始化完成");
    return true;
}

void ThreadManager::Shutdown() {
    LOG_INFO("Thread", "线程管理器开始关闭");
    
    // 等待所有线程完成
    for (auto& pair : m_threads) {
        if (pair.second.joinable()) {
            pair.second.join();
        }
    }
    m_threads.clear();
    
    LOG_INFO("Thread", "线程管理器关闭完成");
}

std::thread ThreadManager::CreateThread(const std::string& name, std::function<void()> function) {
    LOG_INFO("Thread", "创建线程: {0}", name);
    
    std::thread thread([this, name, function]() {
        // 设置线程名称
        SetThreadName(std::this_thread::get_id(), name);
        
        // 执行线程函数
        function();
    });
    
    // 存储线程
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threads[thread.get_id()] = std::move(thread);
    
    // 返回一个空的线程对象，因为线程已经被存储
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

void ThreadManager::SetThreadAffinity(std::thread::id id, uint32_t coreMask) {
    // 设置线程亲和性
    // 这是一个平台相关的操作，需要根据不同平台实现
}

void ThreadManager::SetThreadPriority(std::thread::id id, int priority) {
    // 设置线程优先级
    // 这是一个平台相关的操作，需要根据不同平台实现
}

}  // namespace Engine
