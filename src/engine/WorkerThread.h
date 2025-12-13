#pragma once

class WorkerThread
{
public:
    virtual ~WorkerThread() = default;

protected:
    // 虚函数，提供默认实现
    virtual void Run() {}
    virtual void OnStart() {}
    virtual void OnStop() {}
};

