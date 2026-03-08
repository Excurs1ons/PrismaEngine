#pragma once
#include "Singleton.h"
#include "Export.h"

namespace PrismaEngine {

/// <summary>
/// 应用程序基类（非模板，用于多态）
/// </summary>
class ENGINE_API IApplicationBase {
public:
    IApplicationBase();
    virtual ~IApplicationBase();

    virtual int Initialize() = 0;
    virtual int Run() = 0;
    virtual void Shutdown() = 0;

    virtual bool IsRunning() const;

protected:
    virtual void SetRunning(bool running);

private:
    bool isRunning = false;
    bool isInitialized = false;
};

/// <summary>
/// 应用程序模板基类
/// </summary>
template<typename T>
class IApplication : public IApplicationBase, public Singleton<T> {
public:
    friend class Singleton<T>;
    ~IApplication() override = default;

    /// <summary>
    /// 获取单例实例
    /// </summary>
    /// <returns></returns>
    static T& GetInstance() {
        return Singleton<T>::GetInstance();
    }

    bool IsInitialized() const {
        return isInitialized;
    }

    bool IsRunning() const override {
        return IApplicationBase::IsRunning();
    }

private:
    bool isRunning = false;
    bool isInitialized = false;
};

} // namespace PrismaEngine
