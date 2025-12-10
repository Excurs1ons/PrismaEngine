#include "Engine.h"
#include "Logger.h"
#include "PhysicsSystem.h"
#include "RenderSystem.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "ThreadManager.h"
#include "InputManager.h"

#include "PlatformWindows.h"

namespace Engine {
    EngineCore::EngineCore() : isRunning_(false) {
        // 初始化日志系统
        if (!Logger::GetInstance().IsInitialized()) {
            Logger::GetInstance().Initialize();
        } else {
            LOG_INFO("Engine", "日志系统已初始化，无需重复初始化");
        }
    }
    bool EngineCore::Initialize() {
        LOG_INFO("Engine", "引擎初始化开始");
        
        // 核心子系统
        if (!RegisterSystem<ThreadManager>()) { 
            return false; 
        }    // 管理 JobSystem 和 独立线程
        
        if (!RegisterSystem<ResourceManager>()) { 
            return false; 
        }   // 负责 IO 和资源缓存
        
        if (!RegisterSystem<RenderSystem>()) { 
            return false; 
        }     // 渲染管线逻辑
        
        if (!RegisterSystem<SceneManager>()) { 
            return false; 
        }     // 场景生命周期
        
        if (!RegisterSystem<PhysicsSystem>()) { 
            return false; 
        }     // 物理世界管理
        
        LOG_INFO("Engine", "引擎初始化完成");
        return true;
    }
    
    int EngineCore::MainLoop() {
        isRunning_ = true;
        auto platform = PlatformWindows::GetInstance();

        // 初始化输入管理器
        InputManager::GetInstance().SetPlatform(platform.get());

        // 主循环
        while (IsRunning()) {
            Tick();
            platform->PumpEvents();
            
            // 检查窗口是否应该关闭
            if (!platform->GetWindowHandle()) {
                Shutdown();
            }
        }
        LOG_INFO("Engine", "引擎已停止运行，应用程序将关闭");
        return 0;
    }

    void EngineCore::Shutdown() {
        LOG_INFO("Engine", "引擎开始关闭");
        
        // 统一销毁
        // std::rbegin 反向迭代
        for (auto it = m_systems.rbegin(); it != m_systems.rend(); ++it) {
            (*it)->Shutdown();
        }

        // 反向析构
        // unique_ptr 会自动 delete T，触发 T 的析构函数
        m_systems.clear(); 
        
        isRunning_ = false;
        LOG_INFO("Engine", "引擎关闭完成");
    }

bool EngineCore::IsRunning() const {
    return isRunning_;
}

void EngineCore::Tick() {
    // 计算时间差
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime     = std::chrono::high_resolution_clock::now();
    float deltaTime      = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime             = currentTime;

    // 更新Time类的静态变量
    Time::DeltaTime = deltaTime;
    Time::TotalTime += deltaTime;

    // 统一更新所有子系统
    for (auto& sys : m_systems) {
        sys->Update(deltaTime);
    }

    // 简单的退出条件：运行10秒后退出
    // 在实际应用中，这里应该检查窗口是否关闭、是否有退出请求等
    // if ((int)Time::TotalTime % 2 == 0) {
    //     LOG_INFO("Engine", "已运行{0}秒，DeltaTime = {1}", Time::TotalTime, Time::DeltaTime);
    // }
}

}  // namespace Engine

