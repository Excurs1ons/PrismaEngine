#pragma once
#include "Singleton.h"
#include "Export.h"

namespace PrismaEngine {

/// <summary>
/// 应用程序基类（非模板，用于多态）
/// </summary>
class IApplicationBase {
public:
    virtual ~IApplicationBase() = default;
    virtual bool Initialize() = 0;
    virtual int Run() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsRunning() const = 0;
};

template<typename T>
class IApplication : public IApplicationBase, public Singleton<T> {
public:
    friend class Singleton<T>;
    ~IApplication() override = default;

    /// <summary>
    /// 应用程序初始化，应该包括完成平台层和渲染器的初始化
    /// </summary>
    /// <returns></returns>
    bool Initialize() override = 0;
    int Run() override = 0;
    void Shutdown() override = 0;

    // 提供获取运行状态的公共方法
    bool IsRunning() const override {
        return isRunning;
    }

protected:
    // 提供设置运行状态的保护方法
    ENGINE_API virtual void SetRunning(bool running) {
        isRunning = running;
    }

private:
    bool isRunning = false;
    bool isInitialized = false;
};

}
