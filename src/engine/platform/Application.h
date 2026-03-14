#pragma once
#include "Engine.h"
#include "Application.h"
#include <memory>
namespace Prisma {
class Application : public Application<Application> {
public:
    friend class Application<Application>;
    /// <summary>
    /// 应用程序初始化，应该包括完成平台层和渲染器的初始化
    /// </summary>
    /// <returns>返回初始化结果，0表示成功，非0表示失败</returns>
    int Initialize() override;
    int Run() override;
    void Shutdown() override;

    Application() = default;
    virtual ~Application() = default;

private:
    std::shared_ptr<Engine> engine = nullptr;
};

}  // namespace Engine