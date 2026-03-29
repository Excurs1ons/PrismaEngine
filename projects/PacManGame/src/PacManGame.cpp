#include "PacManGame.h"
#include "game/GameController.h"
#include "core/Timestep.h"
#include "Application.h"
#include <iostream>

namespace PacMan {

/**
 * @brief Pac-Man 具体的应用程序类
 */
class PacManApplication : public Prisma::Application {
public:
    PacManApplication(PacManGame* game)
        : Prisma::Application({"Pac-Man", 28 * 32, 31 * 32 + 100}) // 宽度, 高度(带UI)
        , m_game(game)
    {
    }

    virtual int OnInitialize() override {
        std::cout << "Pac-Man Application Initialized." << std::endl;
        return 0;
    }

    virtual void OnUpdate(Prisma::Timestep ts) override {
        m_game->OnUpdate(ts);
    }

    virtual void OnRender() override {
        m_game->OnRender();
    }

    virtual void OnEvent(Prisma::Event& e) override {
        // TODO: 处理事件 (键盘、鼠标、窗口大小等)
    }

private:
    PacManGame* m_game;
};

// ========== PacManGame 实现 ==========

PacManGame::PacManGame() {
}

PacManGame::~PacManGame() {
    Shutdown();
}

void PacManGame::Initialize() {
    if (m_initialized) return;

    // 创建游戏控制器
    m_gameController = std::make_unique<GameController>();
    m_gameController->Initialize();

    // 创建 Prisma 应用程序实例
    m_application = std::make_unique<PacManApplication>(this);
    m_application->OnInitialize();

    m_initialized = true;
    std::cout << "Pac-Man Game Initialized." << std::endl;
}

void PacManGame::Shutdown() {
    if (!m_initialized) return;

    if (m_gameController) {
        m_gameController->Shutdown();
    }

    m_initialized = false;
    std::cout << "Pac-Man Game Shutdown." << std::endl;
}

void PacManGame::Run() {
    if (!m_initialized) return;

    m_running = true;
    m_gameController->StartGame();

    // 运行主循环 (这里假设 Application 已经管理了循环)
    // 或者我们手动调用 m_application->Run() 如果它有的话
    // 在 EntryPoint.h 中，是 engine.Run(std::unique_ptr<Prisma::Application>(app));
    // 但在 PacManGame::Run 中，我们需要实现循环直到关闭
    
    // 注意：由于我们已经在 main.cpp 中手动创建了 PacManGame，
    // 我们需要一个循环来保持运行，除非 Prisma::Application 负责。
    
    // 简单实现一个循环，如果 Application 没有自带循环
    // 通常引擎会有自己的循环
}

void PacManGame::OnUpdate(Prisma::Timestep ts) {
    if (m_gameController) {
        m_gameController->Update(ts);
    }
}

void PacManGame::OnRender() {
    if (m_gameController) {
        m_gameController->Render();
    }
}

void PacManGame::OnWindowResize(int width, int height) {
    if (m_gameController) {
        m_gameController->GetCamera().SetViewportSize(width, height);
    }
}

void PacManGame::OnKeyPress(int keyCode) {
    if (m_gameController) {
        m_gameController->OnKeyPress(keyCode);
    }
}

void PacManGame::OnKeyRelease(int keyCode) {
    if (m_gameController) {
        m_gameController->OnKeyRelease(keyCode);
    }
}

void PacManGame::OnMousePress(int button, int x, int y) {
}

void PacManGame::OnMouseRelease(int button, int x, int y) {
}

void PacManGame::OnMouseMove(int x, int y) {
}

} // namespace PacMan
