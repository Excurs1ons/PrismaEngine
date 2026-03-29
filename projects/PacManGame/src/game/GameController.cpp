#include "GameController.h"
#include "graphic/Renderer2D.h"
#include <iostream>
#include <algorithm>

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
    StartGame();
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
        m_board.SetTile(gridPos.x, gridPos.y, TileType::Empty);
        AddScore(PELLET_SCORE);
    } else if (tile == TileType::PowerPellet) {
        m_board.SetTile(gridPos.x, gridPos.y, TileType::Empty);
        AddScore(POWER_PELLET_SCORE);
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
        NextLevel();
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
    // 映射键盘到方向
    // 假设这些是标准键码，或者由 PacManGame 传递
    // 这里暂时使用一个通用的映射逻辑
    switch (keyCode) {
        case 0: // Up
            m_pacman.SetNextDirection(Direction::Up);
            break;
        case 1: // Down
            m_pacman.SetNextDirection(Direction::Down);
            break;
        case 2: // Left
            m_pacman.SetNextDirection(Direction::Left);
            break;
        case 3: // Right
            m_pacman.SetNextDirection(Direction::Right);
            break;
        case 4: // Pause/P
            if (m_gameState == GameState::Playing) PauseGame();
            else if (m_gameState == GameState::Paused) ResumeGame();
            break;
    }
}

void GameController::OnKeyRelease(int /*keyCode*/) {
}

void GameController::RenderUI() {
    // TODO: 实现 UI 渲染（得分、生命等）
}

} // namespace PacMan
