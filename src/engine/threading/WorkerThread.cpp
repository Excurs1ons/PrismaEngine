#include "WorkerThread.h"
#include "Logger.h"
#include <chrono>

WorkerThread::WorkerThread()
{
}

WorkerThread::~WorkerThread()
{
    Stop();
    Join();
}

void WorkerThread::Start()
{
    if (m_running.load()) {
        LOG_WARNING("WorkerThread", "线程已经在运行");
        return;
    }

    m_shouldStop.store(false);
    m_thread = std::thread(&WorkerThread::Run, this);
    LOG_INFO("WorkerThread", "线程已启动");
}

void WorkerThread::Stop()
{
    if (!m_running.load()) {
        return;
    }

    m_shouldStop.store(true);
    LOG_INFO("WorkerThread", "线程停止信号已发送");
}

void WorkerThread::Join()
{
    if (m_thread.joinable()) {
        m_thread.join();
        LOG_INFO("WorkerThread", "线程已结束");
    }
}

void WorkerThread::SetTask(std::function<void()> task)
{
    m_task = std::move(task);
}

void WorkerThread::Run()
{
    OnStart();

    while (!m_shouldStop.load() && m_running.load()) {
        if (m_task) {
            try {
                m_task();
            } catch (const std::exception& e) {
                LOG_ERROR("WorkerThread", "任务执行异常: {0}", e.what());
            }
        }

        // 避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    OnStop();
}

void WorkerThread::OnStart()
{
    m_running.store(true);
    LOG_INFO("WorkerThread", "线程开始运行");
}

void WorkerThread::OnStop()
{
    m_running.store(false);
    LOG_INFO("WorkerThread", "线程已停止");
}
