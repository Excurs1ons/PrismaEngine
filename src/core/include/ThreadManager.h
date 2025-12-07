#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <thread>
#include <string>
#include <functional>
#include <mutex>
#include "ManagerBase.h"

namespace Engine {
class ThreadManager : public ManagerBase<ThreadManager> {
public:
    friend class ManagerBase<ThreadManager>;
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override {}
    static constexpr const std::string GetName() { return R"(ThreadManager)"; }
    
    // 创建专用线程
    std::thread CreateThread(const std::string& name, std::function<void()> function);

    // 获取线程信息
    std::string GetThreadName(std::thread::id id) const;
    void SetThreadName(std::thread::id id, const std::string& name);

    // 线程亲缘性设置（平台相关）
    void SetThreadAffinity(std::thread::id id, uint32_t coreMask);

    // 线程优先级设置
    void SetThreadPriority(std::thread::id id, int priority);

private:
    // 存储所有线程
    std::unordered_map<std::thread::id, std::thread> m_threads;
    
    // 存储线程名称
    std::unordered_map<std::thread::id, std::string> m_threadNames;
    
    // 互斥锁，保护线程映射
    mutable std::mutex m_mutex;
};

}  // namespace Engine