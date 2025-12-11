#pragma once
#include "Singleton.h"

namespace Engine {

template<typename T>
class IApplication :public Singleton<T> {
public:
    friend class Singleton<T>;
    ~IApplication() override = default;

    /// <summary>
    /// 应用程序初始化，应该包括完成平台层和渲染器的初始化
    /// </summary>
    /// <returns></returns>
    virtual bool Initialize() = 0;
    virtual int Run() = 0;
    virtual void Shutdown() = 0;
    // 提供获取运行状态的公共方法
    virtual bool IsRunning() const {
        return isRunning;
    }

protected:
    // 提供设置运行状态的保护方法
    virtual void SetRunning(bool running) {
        isRunning = running;
    }

private:
    bool isRunning = false;
    bool isInitialized = false;
};

}
