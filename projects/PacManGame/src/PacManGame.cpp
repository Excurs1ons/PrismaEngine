#include "PacManGame.h"
#include "game/GameController.h"
#include "core/GameResourceManager.h"
#include "audio/AudioAPI.h"
#include "core/Timestep.h"
#include "Application.h"
#include "EngineLauncher.h"
#include "Platform.h"
#include "core/Event.h"
#include <SDL3/SDL_scancode.h>
#include <iostream>
#include <string>
#include <vector>
#include <ctime>

namespace PacMan {

namespace {
bool HasGraphicsDeviceAvailable() {
    return Prisma::Platform::HasDisplaySupport();
}
}

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
        std::cout << "Pac-Man Application Initializing Assets..." << std::endl;
        
        // 1. 初始化音频设备 (确保引擎已启动)
        m_game->m_audioDevice = Prisma::Audio::AudioAPI::CreateBestDevice();
        if (m_game->m_audioDevice) {
            m_game->m_audioDevice->SetMasterVolume(0.2f);
            std::cout << "Audio device initialized and master volume set to 0.2." << std::endl;
        }
        GameResourceManager::Get().InitializeAudio(m_game->m_audioDevice.get());

        // 2. 加载资源 (此时 RenderDevice 已由 Engine 创建并传给 RenderResourceManager)
        auto& res = GameResourceManager::Get();
        
        // 纹理加载
        std::cout << "Loading textures..." << std::endl;
        res.LoadTexture("Pacman1", "assets/sprites/pacman-right/1.png");
        res.LoadTexture("Pacman2", "assets/sprites/pacman-right/2.png");
        res.LoadTexture("Pacman3", "assets/sprites/pacman-right/3.png");
        
        res.LoadTexture("GhostBlinky", "assets/sprites/ghosts/blinky.png");
        res.LoadTexture("GhostPinky",  "assets/sprites/ghosts/pinky.png");
        res.LoadTexture("GhostInky",   "assets/sprites/ghosts/inky.png");
        res.LoadTexture("GhostClyde",  "assets/sprites/ghosts/clyde.png");
        res.LoadTexture("GhostScared", "assets/sprites/ghosts/blue_ghost.png");

        // 音频加载
        std::cout << "Loading audios..." << std::endl;
        res.LoadAudio("Beginning", "assets/audios/pacman_beginning.wav");
        res.LoadAudio("Chomp",     "assets/audios/pacman_chomp.wav");
        res.LoadAudio("Death",     "assets/audios/pacman_death.wav");
        res.LoadAudio("EatGhost",  "assets/audios/pacman_eatghost.wav");

        // 3. 初始化游戏控制器 (地图等)
        m_game->m_gameController->Initialize();

        std::cout << "Pac-Man Application Initialized with Assets." << std::endl;
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

    // 优先切换工作目录
    const char* exePath = Prisma::Platform::GetExecutablePath();
    if (exePath && *exePath) {
        Prisma::Platform::SetCurrentDirectory(exePath);
        std::cout << "Engine switched working directory to: " << exePath << std::endl;
    }

    // 播种随机数
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 创建游戏控制器
    m_gameController = std::make_unique<GameController>();

    // 创建 Prisma 应用程序实例
    m_application = std::make_unique<PacManApplication>(this);

    m_initialized = true;
    std::cout << "Pac-Man Game Scaffolding Initialized." << std::endl;
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
    spec.RefreshAssetDatabaseOnStartup = false;
    spec.MaxFPS = 144;
    spec.MinLogLevel = Prisma::LogLevel::Debug;
    // 使用相对工程根目录的路径，或者绝对路径
    // 强制输出到工程根目录下的 logs/pacman.log
    // 注意：这里的路径逻辑取决于 Engine 内部如何处理 logFilePath
    // 如果是相对路径，通常是相对于 CWD。
    // 我们在这里尝试使用一个显眼的名称。
    // spec.LogFilePath = "logs/pacman.log"; // 如果 EngineSpecification 支持这个字段
    
    m_lastRunResult = Prisma::RunApplication(std::move(m_application), spec);
    if (m_lastRunResult != 0) {
        std::cerr << "Pac-Man exited with engine error: " << m_lastRunResult << std::endl;
        if (!HasGraphicsDeviceAvailable()) {
            std::cout << "No graphics device detected, switching to console view." << std::endl;
            if (RunConsoleFallback()) {
                m_lastRunResult = 0;
            }
        }
    }
    m_running = false;
}

void PacManGame::OnUpdate(Prisma::Timestep ts) {
    if (m_gameController) {
        m_gameController->Update(ts);
    }
    if (m_audioDevice) {
        m_audioDevice->Update(ts);
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

bool PacManGame::RunConsoleFallback() {
    if (!m_gameController) return false;

    m_gameController->ResetGame();
    m_running = true;

    std::cout << "\n=== Pac-Man Console View ===\n";
    std::cout << "Commands: w/a/s/d move, p pause, r restart, q quit\n";

    while (m_running) {
        RenderConsoleFrame();
        std::cout << "Input> ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            m_running = false;
            break;
        }

        char cmd = line.empty() ? ' ' : line[0];
        HandleConsoleInput(cmd);
        m_gameController->Update(0.12f);
    }

    return true;
}

void PacManGame::RenderConsoleFrame() const {
    const GameBoard& board = m_gameController->GetGameBoard();
    std::vector<std::string> rows(board.GetHeight(), std::string(board.GetWidth(), ' '));

    for (int y = 0; y < board.GetHeight(); ++y) {
        for (int x = 0; x < board.GetWidth(); ++x) {
            switch (board.GetTile(x, y)) {
                case TileType::Wall: rows[y][x] = '#'; break;
                case TileType::Pellet: rows[y][x] = '.'; break;
                case TileType::PowerPellet: rows[y][x] = 'o'; break;
                case TileType::GhostHouse: rows[y][x] = '='; break;
                case TileType::Empty:
                case TileType::PacmanSpawn:
                default: rows[y][x] = ' '; break;
            }
        }
    }

    const auto pacPos = m_gameController->GetPacman().GetGridPosition();
    if (pacPos.x >= 0 && pacPos.x < board.GetWidth() && pacPos.y >= 0 && pacPos.y < board.GetHeight()) {
        rows[pacPos.y][pacPos.x] = 'P';
    }

    const auto& ghosts = m_gameController->GetGhosts();
    for (size_t i = 0; i < ghosts.size(); ++i) {
        const auto gp = ghosts[i].GetGridPosition();
        if (gp.x >= 0 && gp.x < board.GetWidth() && gp.y >= 0 && gp.y < board.GetHeight()) {
            rows[gp.y][gp.x] = static_cast<char>('1' + (i % 9));
        }
    }

    std::cout << "\n";
    for (const auto& row : rows) {
        std::cout << row << '\n';
    }

    const char* state = "MENU";
    switch (m_gameController->GetGameState()) {
        case GameState::Playing: state = "PLAYING"; break;
        case GameState::Paused: state = "PAUSED"; break;
        case GameState::GameOver: state = "GAME OVER"; break;
        case GameState::Victory: state = "VICTORY"; break;
        case GameState::Menu: default: break;
    }

    std::cout << "State: " << state
              << " | Score: " << m_gameController->GetScore()
              << " | Lives: " << m_gameController->GetLives()
              << " | Pellets: " << board.GetRemainingPellets() << "/" << board.GetTotalPellets()
              << "\n";
}

void PacManGame::HandleConsoleInput(char command) {
    switch (command) {
        case 'w': case 'W': m_gameController->OnKeyPress(SDL_SCANCODE_W); break;
        case 'a': case 'A': m_gameController->OnKeyPress(SDL_SCANCODE_A); break;
        case 's': case 'S': m_gameController->OnKeyPress(SDL_SCANCODE_S); break;
        case 'd': case 'D': m_gameController->OnKeyPress(SDL_SCANCODE_D); break;
        case 'p': case 'P': m_gameController->OnKeyPress(SDL_SCANCODE_P); break;
        case 'r': case 'R': m_gameController->OnKeyPress(SDL_SCANCODE_R); break;
        case 'q': case 'Q': m_running = false; break;
        default: break;
    }
}

} // namespace PacMan
