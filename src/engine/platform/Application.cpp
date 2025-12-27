#include "Application.h"
#include "Engine.h"
#include "Export.h"
#include "Logger.h"
#include "Platform.h"

namespace Engine {
/// <summary>
/// 应用程序初始化，需要完成平台层和引擎的初始化
/// </summary>
/// <returns></returns>
bool Application::Initialize() {
    LOG_INFO("Application", "应用程序初始化开始");
    
    // 创建引擎实例
    engine = std::make_unique<EngineCore>();
    
    // 初始化引擎
    if (!engine->Initialize()) {
        LOG_ERROR("Application", "引擎初始化失败");
        return false;
    }
    
    LOG_INFO("Application", "应用程序初始化完成");
    return true;
}

int Application::Run() {
    if (!engine) {
        LOG_ERROR("Application", "引擎实例无效，无法运行");
        return -1;
    }
    SetRunning(true);
    LOG_INFO("Application", "应用程序开始运行");
    int result = engine->MainLoop();
    LOG_INFO("Application", "应用程序运行结束");
    return result;
}

void Application::Shutdown() {
    LOG_INFO("Application", "应用程序开始关闭");
    
    // 关闭引擎
    if (engine) {
        engine->Shutdown();
        engine.reset();
    }
    
    SetRunning(false);
    LOG_INFO("Application", "应用程序关闭完成");
}
}  // namespace Engine
