#pragma once
#include "WorkerThread.h"
#include <thread>
#include <atomic>
#include <functional>

class RenderThread : public WorkerThread
{
public:
    RenderThread();
    ~RenderThread() override;

    // 启动渲染线程
    void Start();

    // 停止渲染线程
    void Stop();

    // 等待渲染线程完成
    void Join();

    // 设置渲染函数
    void SetRenderFunction(std::function<void()> renderFunc);

    // 检查线程是否运行中
    bool IsRunning() const { return m_running.load(); }

protected:
    // WorkerThread 接口
    void Run() override;
    void OnStart() override;
    void OnStop() override;

private:
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldStop{false};
    std::function<void()> m_renderFunction;
};
