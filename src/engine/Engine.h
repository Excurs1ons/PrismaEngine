#pragma once

#include "ISubSystem.h"
#include "Logger.h"
#include "Singleton.h"
#include "Export.h"
#include <chrono>
#include <memory>
#include <vector>
#include "ManagerBase.h"

namespace PrismaEngine {

/// @brief 引擎核心类
class ENGINE_API EngineCore : public ManagerBase<EngineCore> {
public:
    static std::shared_ptr<EngineCore> GetInstance();

    EngineCore();
    virtual ~EngineCore() = default;

    /// @brief 初始化引擎所有子系统
    int Initialize();

    /// @brief 启动引擎主循环
    int Run();

    /// @brief 关闭引擎
    void Shutdown();

    /// @brief 每一帧的更新逻辑
    void Update();

    /// @brief 检查引擎是否已初始化
    bool IsInitialized() const { return m_initialized; }

    /// @brief 检查引擎是否正在运行
    bool IsRunning() const { return isRunning_; }

private:
    /// @brief 注册并初始化子系统
    bool RegisterSystem(ISubSystem* system);

    std::vector<ISubSystem*> m_systems;
    bool m_initialized = false;
    bool isRunning_ = false;
};

} // namespace PrismaEngine