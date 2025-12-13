#include "RenderThread.h"
#include "Logger.h"

RenderThread::RenderThread()
{
}

RenderThread::~RenderThread()
{
    Stop();
    Join();
}

void RenderThread::Start()
{
    if (m_running.load()) {
        LOG_WARNING("RenderThread", "渲染线程已经在运行");
        return;
    }

    m_shouldStop.store(false);
    m_thread = std::thread(&RenderThread::Run, this);
    LOG_INFO("RenderThread", "渲染线程已启动");
}

void RenderThread::Stop()
{
    if (!m_running.load()) {
        return;
    }

    m_shouldStop.store(true);
    LOG_INFO("RenderThread", "渲染线程停止信号已发送");
}

void RenderThread::Join()
{
    if (m_thread.joinable()) {
        m_thread.join();
        LOG_INFO("RenderThread", "渲染线程已结束");
    }
}

void RenderThread::SetRenderFunction(std::function<void()> renderFunc)
{
    m_renderFunction = std::move(renderFunc);
}

void RenderThread::Run()
{
    OnStart();

    while (!m_shouldStop.load() && m_running.load()) {
        if (m_renderFunction) {
            try {
                m_renderFunction();
            } catch (const std::exception& e) {
                LOG_ERROR("RenderThread", "渲染函数执行异常: {0}", e.what());
            }
        }

        // 避免CPU占用过高，可以添加短暂休眠
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    OnStop();
}

void RenderThread::OnStart()
{
    m_running.store(true);
    LOG_INFO("RenderThread", "渲染线程开始运行");
}

void RenderThread::OnStop()
{
    m_running.store(false);
    LOG_INFO("RenderThread", "渲染线程已停止");
}