#pragma once
#include "ManagerBase.h"
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace PrismaEngine {

class ENGINE_API ThreadManager : public ManagerBase<ThreadManager> {
public:
    static std::shared_ptr<ThreadManager> GetInstance();

    static constexpr const char* GetStaticName() { return "ThreadManager"; }
    int Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;

    std::thread CreateThread(const std::string& name, std::function<void()> function);
    std::string GetThreadName(std::thread::id id) const;
    void SetThreadName(std::thread::id id, const std::string& name);
    void SetThreadAffinity(std::thread::id id, uint32_t coreMask);
    void SetThreadPriority(std::thread::id id, int priority);

    ThreadManager();
    ~ThreadManager() override;

private:
    std::map<std::thread::id, std::thread> m_threads;
    std::map<std::thread::id, std::string> m_threadNames;
    mutable std::mutex m_mutex;
};
}  // namespace PrismaEngine
