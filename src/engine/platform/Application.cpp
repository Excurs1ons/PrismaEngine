#include "Application.h"
#include "Engine.h"
#include "Export.h"
#include "Logger.h"
#include "Platform.h"

namespace Prisma {
/// <summary>
/// 应用程序初始化，需要完成平台层和引擎的初始化
/// </summary>
/// <returns></returns>
int Application::Initialize() {
    LOG_INFO("Application", "应用程序初始化开始");

    // 获取引擎单例
    engine = Engine::Get();

    if (!engine) {
        LOG_ERROR("Application", "无法获取引擎实例");
        return false;
    }
    int result = engine->Initialize();
    // 初始化引擎

    if (result != 0) {
        LOG_ERROR("Application", "引擎初始化失败");
        return result;
    }

    LOG_INFO("Application", "应用程序初始化完成");
    return 0;
}

int Application::Run() {
    if (!engine) {
        LOG_ERROR("Application", "引擎实例无效，无法运行");
        return -1;
    }
    SetRunning(true);
    LOG_INFO("Application", "应用程序开始运行");
    int result = engine->Run();
    LOG_INFO("Application", "应用程序运行结束");
    return result;
}

void Application::Shutdown() {
    LOG_INFO("Application", "应用程序开始关闭");

    // 关闭引擎
    if (engine) {
        engine->Shutdown();
        engine = nullptr;
    }    
    SetRunning(false);
    LOG_INFO("Application", "应用程序关闭完成");
}
}  // namespace Engine
