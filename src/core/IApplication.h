#pragma once

namespace Engine {

template<typename T>
class IApplication :public Singleton<T> {
public:
    friend class Singleton<T>;
    virtual ~IApplication() = default;

    /// <summary>
    /// 应用程序初始化，应该包括完成平台层和渲染器的初始化
    /// </summary>
    /// <returns></returns>
    virtual bool Initialize() = 0;
    virtual int Run() = 0;
    virtual void Shutdown() = 0;

protected:
    bool m_isRunning = false;
    bool m_isInitialized = false;
};

}
