#include "Engine.h"
#include "AssetManager.h"
#include "InputManager.h"
#include "Logger.h"
#include "PhysicsSystem.h"
#include "Platform.h"
#include "RenderSystemNew.h"
#include "SceneManager.h"
#include "ThreadManager.h"
#include "DebugOverlay.h"

#if PRISMA_ENABLE_IMGUI_DEBUG
#include "imgui.h"
#endif

namespace PrismaEngine {
    EngineCore::EngineCore() : isRunning_(false) {
        // 初始化日志系统
        if (!Logger::GetInstance().IsInitialized()) {
            Logger::GetInstance().Initialize();
        } else {
            LOG_INFO("Engine", "日志系统已初始化，无需重复初始化");
        }
    }
    bool EngineCore::Initialize() {
        LOG_INFO("Engine", "引擎初始化开始");

        // 核心子系统
        if (!RegisterSystem<ThreadManager>()) {
            return false;
        }    // 管理 JobSystem 和 独立线程

        if (!RegisterSystem<AssetManager>()) {
            return false;
        }   // 负责 IO 和资源缓存

        // SceneManager必须在RenderSystem之前初始化，因为RenderSystem依赖它
        if (!RegisterSystem<SceneManager>()) {
            return false;
        }     // 场景生命周期

        if (!RegisterSystem<PrismaEngine::Graphic::RenderSystem>()) {
            return false;
        }     // 渲染管线逻辑

        if (!RegisterSystem<PhysicsSystem>()) {
            return false;
        }     // 物理世界管理

#if PRISMA_ENABLE_IMGUI_DEBUG
        // 初始化调试覆盖层
        DebugOverlay::GetInstance().Initialize();
#endif

        LOG_INFO("Engine", "引擎初始化完成");
        return true;
    }

    int EngineCore::MainLoop() {

        isRunning_ = true;

#if defined(PRISMA_PLATFORM_WINDOWS) || defined(_WIN32)
        // 初始化平台
        Platform::Initialize();

        // 主循环
        while (IsRunning()) {
            LOG_TRACE("Engine","Ticking...");
            Tick();
            Platform::PumpEvents();

            // 检查窗口是否应该关闭
            if (Platform::ShouldClose(Platform::GetCurrentWindow())) {
                isRunning_ = false;
            }
        }

        // 关闭平台
        Platform::Shutdown();
#else
        // Android/其他平台的主循环由外部控制
        while (IsRunning()) {
            LOG_TRACE("Engine","Ticking...");
            Tick();
        }
#endif
        LOG_INFO("Engine", "引擎已停止运行，应用程序将关闭");
        return 0;
    }

    void EngineCore::Shutdown() {
        LOG_INFO("Engine", "引擎开始关闭");

#if PRISMA_ENABLE_IMGUI_DEBUG
        // 关闭调试覆盖层
        DebugOverlay::GetInstance().Shutdown();
#endif

        // 统一销毁
        // std::rbegin 反向迭代
        for (auto it = m_systems.rbegin(); it != m_systems.rend(); ++it) {
            (*it)->Shutdown();
        }

        // 反向析构
        // unique_ptr 会自动 delete T，触发 T 的析构函数
        m_systems.clear();

        isRunning_ = false;
        LOG_INFO("Engine", "引擎关闭完成");
    }

bool EngineCore::IsRunning() const {
    return isRunning_;
}

void EngineCore::Tick() const {
    // 计算时间差
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime     = std::chrono::high_resolution_clock::now();
    float deltaTime      = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime             = currentTime;

    // 更新Time类的静态变量
    Time::DeltaTime = deltaTime;
    Time::TotalTime += deltaTime;

    // 统一更新所有子系统
    for (auto& sys : m_systems) {
        if (sys) {
            sys->Update(deltaTime);
        }
    }

#if PRISMA_ENABLE_IMGUI_DEBUG
    // 更新和渲染调试覆盖层
    DebugOverlay::GetInstance().Update(deltaTime);

    // ImGui 新帧
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = deltaTime;

    // 渲染调试覆盖层
    DebugOverlay::GetInstance().Render();

    // ImGui 渲染
    ImGui::Render();
    // 注意：实际的 ImGui 绘制命令需要在渲染管线中执行
    // 这里只准备数据，由 RenderSystem 负责实际的绘制
#endif

    // 简单的退出条件：运行10秒后退出
    // 在实际应用中，这里应该检查窗口是否关闭、是否有退出请求等
    // if ((int)Time::TotalTime % 2 == 0) {
    //     LOG_INFO("Engine", "已运行{0}秒，DeltaTime = {1}", Time::TotalTime, Time::DeltaTime);
    // }
}

}  // namespace Engine
