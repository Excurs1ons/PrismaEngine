#include "PacManGame.h"
#include "game/GameController.h"
#include "core/Timestep.h"
#include "Application.h"
#include "EngineLauncher.h"
#include "core/Event.h"
#include <SDL3/SDL_scancode.h>
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
        Prisma::Application::OnEvent(e);

        Prisma::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Prisma::WindowResizeEvent>([this](Prisma::WindowResizeEvent& ev) {
            m_game->OnWindowResize(static_cast<int>(ev.GetWidth()), static_cast<int>(ev.GetHeight()));
            return false;
        });

        dispatcher.Dispatch<Prisma::KeyPressedEvent>([this](Prisma::KeyPressedEvent& ev) {
            if (ev.IsRepeat()) return false;
            if (ev.GetKeyCode() == SDL_SCANCODE_ESCAPE) {
                Close();
                return true;
            }
            m_game->OnKeyPress(ev.GetKeyCode());
            return false;
        });

        dispatcher.Dispatch<Prisma::KeyReleasedEvent>([this](Prisma::KeyReleasedEvent& ev) {
            m_game->OnKeyRelease(ev.GetKeyCode());
            return false;
        });

        dispatcher.Dispatch<Prisma::MouseButtonPressedEvent>([this](Prisma::MouseButtonPressedEvent& ev) {
            m_game->OnMousePress(ev.GetMouseButton(), 0, 0);
            return false;
        });

        dispatcher.Dispatch<Prisma::MouseButtonReleasedEvent>([this](Prisma::MouseButtonReleasedEvent& ev) {
            m_game->OnMouseRelease(ev.GetMouseButton(), 0, 0);
            return false;
        });

        dispatcher.Dispatch<Prisma::MouseMovedEvent>([this](Prisma::MouseMovedEvent& ev) {
            m_game->OnMouseMove(static_cast<int>(ev.GetX()), static_cast<int>(ev.GetY()));
            return false;
        });
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
    Prisma::EngineSpecification spec;
    spec.Name = "PacManGame";
    spec.Headless = false;
    spec.MaxFPS = 144;
    spec.MinLogLevel = Prisma::LogLevel::Info;

    m_lastRunResult = Prisma::RunApplication(std::move(m_application), spec);
    if (m_lastRunResult != 0) {
        std::cerr << "Pac-Man exited with engine error: " << m_lastRunResult << std::endl;
    }
    m_running = false;
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
