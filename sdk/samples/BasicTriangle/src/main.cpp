#include <PrismaEngine/PrismaEngine.h>
#include <iostream>

using namespace PrismaEngine;

class BasicTriangleApp : public IApplication<BasicTriangleApp> {
public:
    bool Initialize() override {
        LOG_INFO("BasicTriangle", "应用程序初始化");

        // 初始化平台
        if (!Platform::Initialize()) {
            LOG_ERROR("BasicTriangle", "平台初始化失败");
            return false;
        }

        // 创建窗口
        WindowProps props("Basic Triangle - PrismaEngine", 1280, 720);
        m_window = Platform::CreateWindow(props);

        if (!m_window) {
            LOG_ERROR("BasicTriangle", "窗口创建失败");
            return false;
        }

        LOG_INFO("BasicTriangle", "初始化完成");
        return true;
    }

    int Run() override {
        LOG_INFO("BasicTriangle", "开始运行");

        auto renderSystem = Graphic::RenderSystem::GetInstance();

        if (!renderSystem->Initialize()) {
            LOG_ERROR("BasicTriangle", "渲染系统初始化失败");
            return 1;
        }

        bool running = true;
        SetRunning(true);

        while (IsRunning()) {
            Platform::PumpEvents();

            if (Platform::ShouldClose(m_window)) {
                SetRunning(false);
                break;
            }

            // 渲染一帧
            renderSystem->BeginFrame();

            // TODO: 在这里添加渲染代码

            renderSystem->EndFrame();
            renderSystem->Present();
        }

        LOG_INFO("BasicTriangle", "运行结束");
        return 0;
    }

    void Shutdown() override {
        LOG_INFO("BasicTriangle", "应用程序关闭");

        if (m_window) {
            Platform::DestroyWindow(m_window);
        }

        Platform::Shutdown();
    }

private:
    WindowHandle m_window = nullptr;
};

int main(int argc, char* argv[]) {
    // 初始化日志
    Logger::GetInstance().Initialize();

    LOG_INFO("Main", "BasicTriangle 示例程序");
    LOG_INFO("Main", "PrismaEngine 版本: {}",
             CommandLineParser::GetVersion());

    // 创建并运行应用
    BasicTriangleApp app;
    if (!app.Initialize()) {
        LOG_FATAL("Main", "应用初始化失败");
        return 1;
    }

    int exitCode = app.Run();
    app.Shutdown();

    return exitCode;
}
