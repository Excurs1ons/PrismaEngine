#include "GameController.h"
#include "graphic/Renderer2D.h"
#include "Engine.h"
#include <SDL3/SDL_scancode.h>
#include <iostream>
#include <algorithm>
#include <sstream>

namespace PacMan {

GameController::GameController() {
}

void GameController::Initialize() {
    m_board.LoadDefaultMap();
    m_pacman.Initialize(m_board.GetPacmanSpawnPoint(), &m_board);
    InitializeGhosts();
    
    // 初始化相机
    m_camera.SetPosition(glm::vec2(BOARD_WIDTH * TILE_SIZE / 2.0f, BOARD_HEIGHT * TILE_SIZE / 2.0f));
    m_camera.SetViewportSize(BOARD_WIDTH * TILE_SIZE, BOARD_HEIGHT * TILE_SIZE);
    
    m_gameState = GameState::Menu;
    m_score = 0;
    m_lives = 3;
    m_currentLevel = 1;
}

void GameController::Shutdown() {
    // 清理资源
}

void GameController::InitializeGhosts() {
    m_ghosts.clear();
    m_ghosts.resize(4);
    
    auto& spawnPoints = m_board.GetGhostSpawnPoints();
    
    // 如果地图中没有足够的出生点，使用默认位置
    glm::ivec2 defaultSpawn(14, 14);
    
    m_ghosts[0].Initialize(GhostType::Blinky, spawnPoints.size() > 0 ? spawnPoints[0] : defaultSpawn, &m_board, &m_pacman);
    m_ghosts[1].Initialize(GhostType::Pinky,  spawnPoints.size() > 1 ? spawnPoints[1] : defaultSpawn, &m_board, &m_pacman);
    m_ghosts[2].Initialize(GhostType::Inky,   spawnPoints.size() > 2 ? spawnPoints[2] : defaultSpawn, &m_board, &m_pacman);
    m_ghosts[3].Initialize(GhostType::Clyde,  spawnPoints.size() > 3 ? spawnPoints[3] : defaultSpawn, &m_board, &m_pacman);
}

void GameController::Update(Prisma::Timestep ts) {
    m_uiRefreshAccumulator += static_cast<float>(ts);
    if (m_uiRefreshAccumulator >= 0.15f) {
        m_uiRefreshAccumulator = 0.0f;
        UpdateUI();
    }

    if (m_gameState != GameState::Playing) {
        return;
    }

    // 更新吃豆人
    m_pacman.Update(ts);

    // 更新幽灵
    for (auto& ghost : m_ghosts) {
        ghost.Update(ts);
    }

    // 碰撞检测
    CheckPelletCollision();
    CheckGhostCollision();
    
    // 检查关卡完成
    CheckLevelComplete();

    // 更新能量模式
    if (m_powerModeActive) {
        UpdatePowerMode(ts);
    }
}

void GameController::Render() {
    Prisma::Graphic::Renderer2D::BeginScene(m_camera);

    // 渲染游戏板（背景和墙壁）
    m_board.Render();

    // 渲染幽灵
    for (auto& ghost : m_ghosts) {
        ghost.Render();
    }

    // 渲染吃豆人
    m_pacman.Render();

    Prisma::Graphic::Renderer2D::EndScene();

    // 渲染 UI
    RenderUI();
}

void GameController::SetGameState(GameState state) {
    m_gameState = state;
}

void GameController::StartGame() {
    if (m_gameState == GameState::Menu || m_gameState == GameState::GameOver || m_gameState == GameState::Victory) {
        ResetGame();
    }
    m_gameState = GameState::Playing;
}

void GameController::PauseGame() {
    if (m_gameState == GameState::Playing) {
        m_gameState = GameState::Paused;
    }
}

void GameController::ResumeGame() {
    if (m_gameState == GameState::Paused) {
        m_gameState = GameState::Playing;
    }
}

void GameController::ResetGame() {
    Initialize();
    m_gameState = GameState::Playing;
}

void GameController::AddScore(int score) {
    m_score += score;
    if (m_score > m_highScore) {
        m_highScore = m_score;
    }
}

void GameController::SetHighScore(int score) {
    m_highScore = score;
}

void GameController::SetLives(int lives) {
    m_lives = lives;
}

void GameController::LoseLife() {
    m_lives--;
    if (m_lives <= 0) {
        m_gameState = GameState::GameOver;
    } else {
        // 重置位置，但不重置得分和地图
        m_pacman.Reset();
        for (auto& ghost : m_ghosts) {
            ghost.Reset();
        }
    }
}

void GameController::OnPacmanDeath() {
    LoseLife();
}

void GameController::ActivatePowerMode() {
    m_powerModeActive = true;
    m_powerModeTimer = m_powerModeDuration;
    
    for (auto& ghost : m_ghosts) {
        if (ghost.GetState() != GhostState::Eaten) {
            ghost.SetState(GhostState::Frightened);
        }
    }
}

void GameController::UpdatePowerMode(Prisma::Timestep ts) {
    m_powerModeTimer -= static_cast<float>(ts);
    if (m_powerModeTimer <= 0) {
        m_powerModeActive = false;
        for (auto& ghost : m_ghosts) {
            if (ghost.GetState() == GhostState::Frightened) {
                ghost.SetState(GhostState::Chase); // 或者回到之前的状态
            }
        }
    }
}

void GameController::CheckPelletCollision() {
    glm::ivec2 gridPos = m_pacman.GetGridPosition();
    TileType tile = m_board.GetTile(gridPos.x, gridPos.y);

    if (tile == TileType::Pellet) {
        AddScore(m_board.EatPellet(gridPos.x, gridPos.y));
    } else if (tile == TileType::PowerPellet) {
        AddScore(m_board.EatPowerPellet(gridPos.x, gridPos.y));
        ActivatePowerMode();
    }
}

void GameController::CheckGhostCollision() {
    glm::vec2 pacmanPos = m_pacman.GetPosition();
    float pacmanRadius = TILE_SIZE * 0.4f;

    for (auto& ghost : m_ghosts) {
        glm::vec2 ghostPos = ghost.GetPosition();
        float distance = glm::distance(pacmanPos, ghostPos);

        if (distance < pacmanRadius + TILE_SIZE * 0.4f) {
            if (ghost.GetState() == GhostState::Frightened) {
                // 吃到幽灵
                ghost.SetState(GhostState::Eaten);
                AddScore(GHOST_EAT_SCORE);
            } else if (ghost.GetState() != GhostState::Eaten) {
                // 碰到正常幽灵，死亡
                OnPacmanDeath();
                break;
            }
        }
    }
}

void GameController::CheckLevelComplete() {
    if (m_board.GetRemainingPellets() == 0) {
        m_gameState = GameState::Victory;
    }
}

void GameController::SetLevel(int level) {
    m_currentLevel = level;
    // 增加难度：提高幽灵速度等
}

void GameController::NextLevel() {
    m_currentLevel++;
    m_board.LoadDefaultMap(); // 重新加载地图
    m_pacman.Reset();
    for (auto& ghost : m_ghosts) {
        ghost.Reset();
    }
}

void GameController::OnKeyPress(int keyCode) {
    if (keyCode == SDL_SCANCODE_RETURN || keyCode == SDL_SCANCODE_KP_ENTER) {
        if (m_gameState == GameState::Menu || m_gameState == GameState::GameOver || m_gameState == GameState::Victory) {
            StartGame();
        }
        return;
    }

    if (keyCode == SDL_SCANCODE_R) {
        ResetGame();
        return;
    }

    switch (keyCode) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            m_pacman.SetNextDirection(Direction::Up);
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            m_pacman.SetNextDirection(Direction::Down);
            break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
            m_pacman.SetNextDirection(Direction::Left);
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            m_pacman.SetNextDirection(Direction::Right);
            break;
        case SDL_SCANCODE_P:
        case SDL_SCANCODE_SPACE:
            if (m_gameState == GameState::Playing) PauseGame();
            else if (m_gameState == GameState::Paused) ResumeGame();
            break;
    }
}

void GameController::OnKeyRelease(int /*keyCode*/) {
}

void GameController::RenderUI() {
    // Render simple in-game HUD using quads so gameplay feedback is visible
    // even without text rendering support.
    Prisma::Graphic::Renderer2D::BeginScene(m_camera);

    const float panelHeight = 18.0f;
    const float panelWidth = BOARD_WIDTH * TILE_SIZE - 16.0f;
    const glm::vec2 panelPos(8.0f, 8.0f);
    Prisma::Graphic::Renderer2D::DrawQuad(panelPos, glm::vec2(panelWidth, panelHeight), Prisma::Color(0.05f, 0.05f, 0.08f, 0.8f));

    Prisma::Color stateColor(0.2f, 0.2f, 0.2f, 0.95f);
    if (m_gameState == GameState::Playing) stateColor = Prisma::Color(0.1f, 0.55f, 0.2f, 0.95f);
    else if (m_gameState == GameState::Paused) stateColor = Prisma::Color(0.8f, 0.65f, 0.1f, 0.95f);
    else if (m_gameState == GameState::GameOver) stateColor = Prisma::Color(0.7f, 0.12f, 0.12f, 0.95f);
    else if (m_gameState == GameState::Victory) stateColor = Prisma::Color(0.2f, 0.35f, 0.9f, 0.95f);
    Prisma::Graphic::Renderer2D::DrawQuad(glm::vec2(10.0f, 10.0f), glm::vec2(90.0f, 14.0f), stateColor);

    // Lives indicator
    for (int i = 0; i < std::max(0, m_lives); ++i) {
        Prisma::Graphic::Renderer2D::DrawQuad(glm::vec2(110.0f + i * 16.0f, 10.0f), glm::vec2(12.0f, 12.0f), Prisma::Color(1.0f, 0.95f, 0.15f, 1.0f));
    }

    // Pellet progress bar
    const int totalPellets = std::max(1, m_board.GetTotalPellets());
    const int eatenPellets = std::max(0, totalPellets - m_board.GetRemainingPellets());
    const float progress = static_cast<float>(eatenPellets) / static_cast<float>(totalPellets);
    const glm::vec2 barBgPos(200.0f, 10.0f);
    const glm::vec2 barSize(220.0f, 12.0f);
    Prisma::Graphic::Renderer2D::DrawQuad(barBgPos, barSize, Prisma::Color(0.15f, 0.15f, 0.2f, 0.95f));
    Prisma::Graphic::Renderer2D::DrawQuad(barBgPos, glm::vec2(barSize.x * progress, barSize.y), Prisma::Color(0.2f, 0.7f, 1.0f, 0.95f));

    // Power mode indicator
    if (m_powerModeActive) {
        Prisma::Graphic::Renderer2D::DrawQuad(glm::vec2(430.0f, 10.0f), glm::vec2(120.0f, 12.0f), Prisma::Color(0.05f, 0.05f, 0.2f, 0.95f));
        const float ratio = std::clamp(m_powerModeTimer / m_powerModeDuration, 0.0f, 1.0f);
        Prisma::Graphic::Renderer2D::DrawQuad(glm::vec2(430.0f, 10.0f), glm::vec2(120.0f * ratio, 12.0f), Prisma::Color(0.35f, 0.35f, 1.0f, 0.95f));
    }

    Prisma::Graphic::Renderer2D::EndScene();

    UpdateUI();
}

void GameController::UpdateUI() {
    if (!Prisma::Engine::Get().IsRunning()) return;

    const char* state = "MENU";
    switch (m_gameState) {
        case GameState::Playing: state = "PLAYING"; break;
        case GameState::Paused: state = "PAUSED"; break;
        case GameState::GameOver: state = "GAME OVER"; break;
        case GameState::Victory: state = "VICTORY"; break;
        case GameState::Menu: default: state = "MENU"; break;
    }

    std::ostringstream oss;
    oss << "Pac-Man | State: " << state
        << " | Score: " << m_score
        << " | High: " << m_highScore
        << " | Lives: " << m_lives
        << " | Level: " << m_currentLevel
        << " | Pellets: " << m_board.GetRemainingPellets() << "/" << m_board.GetTotalPellets()
        << " | Controls: WASD/Arrows Move, P Pause, Enter Start, R Reset, Esc Exit";

    Prisma::Engine::Get().GetWindow().SetTitle(oss.str());
}

} // namespace PacMan
