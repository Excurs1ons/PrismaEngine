#pragma once
#include "ManagerBase.h"
#include <functional>
#include <map>
#include <mutex>
#include <unordered_map>
#include <string>
#include <thread>
#include <vector>

namespace Prisma {

class ENGINE_API ThreadManager : public ManagerBase<ThreadManager> {
public:
    PRISMA_SUBSET_NAME("ThreadManager")
    static std::shared_ptr<ThreadManager> Get();

    static constexpr const char* GetStaticName() { return "ThreadManager"; }
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    std::thread CreateThread(const std::string& name, std::function<void()> function);
    std::string GetThreadName(std::thread::id id) const;
    void SetThreadName(std::thread::id id, const std::string& name);
    void SetThreadAffinity(std::thread::id id, uint32_t coreMask);
    void SetThreadPriority(std::thread::id id, int priority);

    ThreadManager();
    ~ThreadManager() override;

private:
    struct ThreadMetadata {
        std::string name;
        uint32_t affinityMask = 0;
        int priority = 0;
        bool finished = false;
    };

    std::map<std::thread::id, std::thread> m_threads;
    std::unordered_map<std::thread::id, ThreadMetadata> m_threadMetadata;
    mutable std::mutex m_mutex;
};
}  // namespace Prisma
