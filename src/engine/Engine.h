#pragma once
#include "ISubSystem.h"
#include "Logger.h"
#include "Singleton.h"
#include "Export.h"
#include <chrono>
#include <memory>
#include <vector>

namespace PrismaEngine {
class EngineCore {

public:
    EngineCore();
    bool Initialize();
    bool IsInitialized() const;
    int MainLoop();
    void Tick();
    void Shutdown();
    bool IsRunning() const;
private:

    template <typename T> bool RegisterSystem() {
        auto system = T::GetInstance();
        m_systems.push_back(system);
        // 如果需要初始化
        bool result = m_systems.back()->Initialize();
        if (!result) {
            LOG_ERROR("Engine", "子系统初始化失败: {}", typeid(T).name());
        }
        return result;
    }

    std::vector<std::shared_ptr<ISubSystem>> m_systems;

    // 状态
    bool isRunning_;
};

}  // namespace Engine