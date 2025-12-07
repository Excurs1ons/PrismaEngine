#include "Application.h"
#include "Logger.h"
#include "Engine.h"
#include "RenderSystem.h"
#include "SceneManager.h"
#include "PhysicsSystem.h"
#include "ResourceManager.h"
#include "ThreadManager.h"
#include "PlatformWindows.h"
#include "Engine.h"
#include <iostream>
#include "Export.h"
namespace Engine {
/// <summary>
/// 应用程序初始化，需要完成平台层和引擎的初始化
/// </summary>
/// <returns></returns>
bool Application::Initialize() {
    LOG_INFO("Application", "应用程序初始化开始");
    
    // 创建引擎实例
    m_engine = std::make_unique<EngineCore>();
    
    // 初始化引擎
    if (!m_engine->Initialize()) {
        LOG_ERROR("Application", "引擎初始化失败");
        return false;
    }
    
    // 标记为运行状态
    m_Running = true;
    
    LOG_INFO("Application", "应用程序初始化完成");
    return true;
}

int Application::Run() {
    if (!m_engine) {
        LOG_ERROR("Application", "引擎实例无效，无法运行");
        return -1;
    }
    LOG_INFO("Application", "应用程序开始运行");
    int result = m_engine->MainLoop();
    LOG_INFO("Application", "应用程序运行结束");
    return result;
}

void Application::Shutdown() {
    LOG_INFO("Application", "应用程序开始关闭");
    
    // 关闭引擎
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
    
    m_Running = false;
    
    LOG_INFO("Application", "应用程序关闭完成");
}
extern "C" {

// 导出其他函数供动态加载使用
ENGINE_API bool Initialize() {
    return Application::GetInstance().Initialize();
}

ENGINE_API int Run() {
    return Application::GetInstance().Run();
}

ENGINE_API void Shutdown() {
    Application::GetInstance().Shutdown();
}

}

}  // namespace Engine