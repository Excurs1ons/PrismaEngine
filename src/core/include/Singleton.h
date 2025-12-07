#pragma once
#include <memory>

/// @brief 单例模板类
template<typename T>
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static T& GetInstance() {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

template <typename T>
class SharedSingleton {
public:
    SharedSingleton(const SharedSingleton&)      = delete;
    SharedSingleton& operator=(const SharedSingleton&) = delete;

    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> instance = std::make_shared<T>();
        return instance;
    }
protected:
    SharedSingleton()    = default;
    virtual ~SharedSingleton() = default;
};

// 使用方式
class MyManager : public Singleton<MyManager> {
    friend class Singleton<MyManager>;  // 允许Singleton访问私有构造函数

public:
    void Initialize() { /* ... */ }
    void Shutdown() { /* ... */ }

private:
    MyManager() = default;
    ~MyManager() = default;
};
