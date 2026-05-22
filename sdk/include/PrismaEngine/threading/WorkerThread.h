#pragma once
#include "Export.h"
#include <thread>
#include <atomic>
#include <functional>

class WorkerThread
{
public:
    ENGINE_API WorkerThread();
    ENGINE_API ~WorkerThread();

    // 启动线程
    ENGINE_API void Start();

    // 停止线程
    ENGINE_API void Stop();

    // 等待线程完成
    ENGINE_API void Join();

    // 设置任务函数
    ENGINE_API void SetTask(std::function<void()> task);

    // 检查是否运行中
    bool IsRunning() const { return m_running.load(); }

private:
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldStop{false};
    std::function<void()> m_task;

    void Run();
    void OnStart();
    void OnStop();
};

