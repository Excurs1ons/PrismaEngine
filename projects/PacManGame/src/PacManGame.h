#pragma once

#include "game/GameController.h"
#include "graphic/OrthographicCamera.h"
#include <memory>

namespace Prisma {
    class Application;
}

namespace PacMan {

/**
 * @brief 吃豆人游戏主类
 * 负责初始化引擎和游戏循环
 */
class PacManGame {
public:
    PacManGame();
    ~PacManGame();

    // ========== 初始化 ==========

    /**
     * @brief 初始化游戏
     */
    void Initialize();

    /**
     * @brief 关闭游戏
     */
    void Shutdown();

    /**
     * @brief 运行游戏
     */
    void Run();

    // ========== 更新和渲染 ==========

    /**
     * @brief 每帧更新
     */
    void OnUpdate(Prisma::Timestep ts);

    /**
     * @brief 每帧渲染
     */
    void OnRender();

    // ========== 事件处理 ==========

    /**
     * @brief 窗口大小改变
     */
    void OnWindowResize(int width, int height);

    /**
     * @brief 键盘按下
     */
    void OnKeyPress(int keyCode);

    /**
     * @brief 键盘释放
     */
    void OnKeyRelease(int keyCode);

    /**
     * @brief 鼠标按下
     */
    void OnMousePress(int button, int x, int y);

    /**
     * @brief 鼠标释放
     */
    void OnMouseRelease(int button, int x, int y);

    /**
     * @brief 鼠标移动
     */
    void OnMouseMove(int x, int y);

private:
    std::unique_ptr<Prisma::Application> m_application;
    std::unique_ptr<GameController> m_gameController;
    bool RunConsoleFallback();
    void RenderConsoleFrame() const;
    void HandleConsoleInput(char command);

    bool m_initialized = false;
    bool m_running = false;
    int m_lastRunResult = 0;

public:
    int GetLastRunResult() const { return m_lastRunResult; }
};

extern PacManGame* g_PacManGame;

} // namespace PacMan
