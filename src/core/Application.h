#pragma once
#include "Engine.h"
#include "IApplication.h"
#include "Singleton.h"
#include <memory>
namespace Engine {

class Application : public IApplication<Application> {
public:
    friend class IApplication<Application>;
    /// <summary>
    /// 应用程序初始化，应该包括完成平台层和渲染器的初始化
    /// </summary>
    /// <returns></returns>
    bool Initialize() override;
    int Run() override;
    void Shutdown() override;

protected:
    bool m_Running = false;
    std::unique_ptr<EngineCore> m_engine;
};
}  // namespace Engine