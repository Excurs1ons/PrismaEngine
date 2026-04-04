#include "GameController.h"
#include "Platform.h"
#include "Logger.h"
#include "graphic/Renderer2D.h"
#include <SDL3/SDL_scancode.h>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace PacMan {

GameController::GameController() {}

void GameController::Initialize() {
    m_board.LoadDefaultMap();
    m_pacman.Initialize(m_board.GetPacmanSpawnPoint(), &m_board);
    InitializeGhosts();

    // 初始化相机：总高度增加 48 像素作为 HUD 边距
    // 迷宫尺寸为 448x496
    const float HUD_HEIGHT_MARGIN = 48.0f;
    const float BOARD_WIDTH_PX = BOARD_WIDTH * TILE_SIZE;
    const float BOARD_HEIGHT_PX = BOARD_HEIGHT * TILE_SIZE;
    
    // 相机中心下移，使顶部留出 HUD 空间
    // 逻辑坐标系：HUD 在 y = [-HUD_HEIGHT_MARGIN, 0], 迷宫在 y = [0, BOARD_HEIGHT_PX]
    m_camera.SetPosition(glm::vec2(BOARD_WIDTH_PX / 2.0f, (BOARD_HEIGHT_PX - HUD_HEIGHT_MARGIN) / 2.0f));
    m_camera.SetViewportSize(BOARD_WIDTH_PX, BOARD_HEIGHT_PX + HUD_HEIGHT_MARGIN);

    m_gameState    = GameState::Playing; 
    m_stateTimer   = 2.0f; // READY! 持续 2 秒
    m_score        = 0;
    m_lives        = 3;
    m_currentLevel = 1;

    // 初始化幽灵模式
    m_currentGhostMode = GhostState::Scatter;
    m_ghostModeTimer = 7.0f;
    m_modeCycleCount = 0;

    LOG_INFO("PacMan", "游戏控制器初始化完成，当前状态: PLAYING (READY phase)");
    UpdateUI();
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

    m_ghosts[0].Initialize(
        GhostType::Blinky, spawnPoints.size() > 0 ? spawnPoints[0] : defaultSpawn, &m_board, &m_pacman);
    m_ghosts[1].Initialize(
        GhostType::Pinky, spawnPoints.size() > 1 ? spawnPoints[1] : defaultSpawn, &m_board, &m_pacman);
    m_ghosts[2].Initialize(
        GhostType::Inky, spawnPoints.size() > 2 ? spawnPoints[2] : defaultSpawn, &m_board, &m_pacman);
    m_ghosts[3].Initialize(
        GhostType::Clyde, spawnPoints.size() > 3 ? spawnPoints[3] : defaultSpawn, &m_board, &m_pacman);
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

    // 处理 READY 阶段计时
    if (m_stateTimer > 0.0f) {
        m_stateTimer -= static_cast<float>(ts);
        return; // READY 期间不更新移动逻辑
    }

    // 更新幽灵模式循环
    if (!m_powerModeActive) {
        m_ghostModeTimer -= static_cast<float>(ts);
        if (m_ghostModeTimer <= 0.0f) {
            // 切换模式
            if (m_currentGhostMode == GhostState::Scatter) {
                m_currentGhostMode = GhostState::Chase;
                m_modeCycleCount++;
                // Chase 时长通常为 20s
                m_ghostModeTimer = 20.0f;
                LOG_INFO("PacMan", "幽灵进入追逐模式 (Chase)");
            } else {
                m_currentGhostMode = GhostState::Scatter;
                // Scatter 时长随循环次数减少 (7s, 7s, 5s, 5s)
                m_ghostModeTimer = (m_modeCycleCount < 2) ? 7.0f : 5.0f;
                if (m_modeCycleCount >= 4) m_ghostModeTimer = 999999.0f; // 永久 Chase (实际上由上一段逻辑控制)
                LOG_INFO("PacMan", "幽灵进入散开模式 (Scatter)");
            }

            // 通知所有非惊吓状态的幽灵切换模式
            for (auto& ghost : m_ghosts) {
                if (ghost.GetState() != GhostState::Frightened && ghost.GetState() != GhostState::Eaten) {
                    ghost.SetState(m_currentGhostMode);
                    // 经典 Pac-Man：模式切换时幽灵立即反向
                    ghost.SetDirection(ReverseDirection(ghost.GetCurrentDirection()));
                }
            }
        }
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

    // 渲染游戏 board (背景和墙壁)
    m_board.Render();

    // 渲染幽灵
    for (auto& ghost : m_ghosts) {
        ghost.Render();
    }

    // 渲染吃豆人
    m_pacman.Render();

    // 渲染 UI
    RenderUI();

    Prisma::Graphic::Renderer2D::EndScene();
}

void GameController::SetGameState(GameState state) {
    m_gameState = state;
}

void GameController::StartGame() {
    if (m_gameState == GameState::Menu || m_gameState == GameState::GameOver || m_gameState == GameState::Victory) {
        ResetGame();
    }
    m_gameState = GameState::Playing;
    m_stateTimer = 2.0f; 
    LOG_INFO("PacMan", "游戏开始");
}

void GameController::PauseGame() {
    if (m_gameState == GameState::Playing) {
        m_gameState = GameState::Paused;
        LOG_INFO("PacMan", "游戏暂停");
    }
}

void GameController::ResumeGame() {
    if (m_gameState == GameState::Paused) {
        m_gameState = GameState::Playing;
        LOG_INFO("PacMan", "游戏恢复");
    }
}

void GameController::ResetGame() {
    Initialize();
    m_gameState = GameState::Playing;
    m_stateTimer = 2.0f;
    LOG_INFO("PacMan", "游戏重置");
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
    LOG_INFO("PacMan", "失去一条生命，剩余: {}", m_lives);
    if (m_lives <= 0) {
        m_gameState = GameState::GameOver;
        LOG_INFO("PacMan", "游戏结束 (Game Over)");
    } else {
        // 重置位置，但不重置得分和地图
        m_pacman.Reset();
        for (auto& ghost : m_ghosts) {
            ghost.Reset();
        }
        m_stateTimer = 2.0f; // 重新开始时再次显示 READY!
    }
}

void GameController::OnPacmanDeath() {
    LoseLife();
}

void GameController::ActivatePowerMode() {
    m_powerModeActive = true;
    m_powerModeTimer  = m_powerModeDuration;
    LOG_INFO("PacMan", "进入能量模式！");

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
        LOG_INFO("PacMan", "能量模式结束");
        for (auto& ghost : m_ghosts) {
            if (ghost.GetState() == GhostState::Frightened) {
                ghost.SetState(GhostState::Chase);  // 或者回到之前的状态
            }
        }
    }
}

void GameController::CheckPelletCollision() {
    glm::ivec2 gridPos = m_pacman.GetGridPosition();
    TileType tile      = m_board.GetTile(gridPos.x, gridPos.y);

    if (tile == TileType::Pellet) {
        AddScore(m_board.EatPellet(gridPos.x, gridPos.y));
    } else if (tile == TileType::PowerPellet) {
        AddScore(m_board.EatPowerPellet(gridPos.x, gridPos.y));
        ActivatePowerMode();
    }
}

void GameController::CheckGhostCollision() {
    glm::vec2 pacmanPos = m_pacman.GetPosition();
    float pacmanRadius  = TILE_SIZE * 0.4f;

    for (auto& ghost : m_ghosts) {
        glm::vec2 ghostPos = ghost.GetPosition();
        float distance     = glm::distance(pacmanPos, ghostPos);

        if (distance < pacmanRadius + TILE_SIZE * 0.4f) {
            if (ghost.GetState() == GhostState::Frightened) {
                // 吃到幽灵
                ghost.SetState(GhostState::Eaten);
                AddScore(GHOST_EAT_SCORE);
                LOG_INFO("PacMan", "吃到幽灵！得分 +{}", GHOST_EAT_SCORE);
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
        LOG_INFO("PacMan", "胜利！关卡完成");
    }
}

void GameController::SetLevel(int level) {
    m_currentLevel = level;
    // 增加难度：提高幽灵速度等
}

void GameController::NextLevel() {
    m_currentLevel++;
    LOG_INFO("PacMan", "进入下一关: {}", m_currentLevel);
    m_board.LoadDefaultMap();  // 重新加载地图
    m_pacman.Reset();
    for (auto& ghost : m_ghosts) {
        ghost.Reset();
    }
    m_stateTimer = 2.0f;
}

void GameController::OnKeyPress(int keyCode) {
    LOG_DEBUG("PacMan", "按键按下: {}", keyCode);

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
            LOG_DEBUG("PacMan", "尝试向上移动");
            m_pacman.SetNextDirection(Direction::Up);
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            LOG_DEBUG("PacMan", "尝试向下移动");
            m_pacman.SetNextDirection(Direction::Down);
            break;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
            LOG_DEBUG("PacMan", "尝试向左移动");
            m_pacman.SetNextDirection(Direction::Left);
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            LOG_DEBUG("PacMan", "尝试向右移动");
            m_pacman.SetNextDirection(Direction::Right);
            break;
        case SDL_SCANCODE_P:
        case SDL_SCANCODE_SPACE:
            if (m_gameState == GameState::Playing)
                PauseGame();
            else if (m_gameState == GameState::Paused)
                ResumeGame();
            break;
    }
}

void GameController::OnKeyRelease(int /*keyCode*/) {}

void GameController::RenderUI() {
    // HUD 背景区域 (位于迷宫上方)
    const float HUD_Y = -40.0f;
    const float HUD_W = BOARD_WIDTH * TILE_SIZE;
    const float BOARD_H_PX = BOARD_HEIGHT * TILE_SIZE;
    Prisma::Graphic::Renderer2D::DrawQuad(glm::vec2(HUD_W / 2.0f, HUD_Y + 16.0f), glm::vec2(HUD_W, 40.0f), Prisma::Color(0.0f, 0.0f, 0.0f, 0.9f));

    // 使用 DrawString 渲染文字信息
    std::string scoreStr = "SCORE: " + std::to_string(m_score);
    std::string highStr  = "HIGH: " + std::to_string(m_highScore);
    std::string levelStr = "LEVEL: " + std::to_string(m_currentLevel);
    
    Prisma::Graphic::Renderer2D::DrawString(scoreStr, glm::vec2(10.0f, HUD_Y), 1.5f, Prisma::Color(1.0f, 1.0f, 1.0f, 1.0f));
    Prisma::Graphic::Renderer2D::DrawString(highStr,  glm::vec2(150.0f, HUD_Y), 1.5f, Prisma::Color(1.0f, 1.0f, 0.0f, 1.0f));
    Prisma::Graphic::Renderer2D::DrawString(levelStr, glm::vec2(320.0f, HUD_Y), 1.5f, Prisma::Color(0.0f, 1.0f, 1.0f, 1.0f));

    // 状态指示
    std::string stateText = "";
    Prisma::Color stateColor(1.0f, 1.0f, 1.0f, 1.0f);
    bool shouldShow = true;

    if (m_gameState == GameState::Playing && m_stateTimer > 0.0f) {
        stateText = "READY!";
        stateColor = Prisma::Color(1.0f, 1.0f, 0.0f, 1.0f);
    } else {
        switch (m_gameState) {
            case GameState::Paused:   stateText = "PAUSED";    stateColor = Prisma::Color(1.0f, 0.7f, 0.0f, 1.0f); break;
            case GameState::GameOver: stateText = "GAME OVER"; stateColor = Prisma::Color(1.0f, 0.0f, 0.0f, 1.0f); break;
            case GameState::Victory:  stateText = "VICTORY!";  stateColor = Prisma::Color(0.0f, 0.5f, 1.0f, 1.0f); break;
            default: shouldShow = false; break;
        }
    }
    
    if (shouldShow) {
        // 闪烁逻辑
        if (m_gameState != GameState::Playing || (int)(Prisma::Platform::GetTimeSeconds() * 2) % 2 == 0) {
            float scale = 2.5f;
            float textW = Prisma::Graphic::Renderer2D::GetStringWidth(stateText, scale);
            // 居中显示 (相对于棋盘中心)
            Prisma::Graphic::Renderer2D::DrawString(stateText, glm::vec2((HUD_W - textW) / 2.0f, BOARD_H_PX / 2.0f), scale, stateColor);
        }
    }

    // 生命周期指示 (小方块)
    for (int i = 0; i < std::max(0, m_lives); ++i) {
        Prisma::Graphic::Renderer2D::DrawQuad(
            glm::vec2(HUD_W - 20.0f - i * 16.0f, HUD_Y + 8.0f), glm::vec2(12.0f, 12.0f), Prisma::Color(1.0f, 0.9f, 0.0f, 1.0f));
    }
}

void GameController::UpdateUI() {
    auto window = Prisma::Platform::GetCurrentWindow();
    if (!window)
        return;

    const char* state = "MENU";
    switch (m_gameState) {
        case GameState::Playing:
            state = "PLAYING";
            break;
        case GameState::Paused:
            state = "PAUSED";
            break;
        case GameState::GameOver:
            state = "GAME OVER";
            break;
        case GameState::Victory:
            state = "VICTORY";
            break;
        case GameState::Menu:
        default:
            state = "MENU";
            break;
    }

    std::ostringstream oss;
    oss << "Pac-Man | State: " << state << " | Score: " << m_score << " | High: " << m_highScore
        << " | Lives: " << m_lives << " | Level: " << m_currentLevel << " | Pellets: " << m_board.GetRemainingPellets()
        << "/" << m_board.GetTotalPellets() << " | Controls: WASD/Arrows Move, P Pause, Enter Start, R Reset, Esc Exit";

    Prisma::Platform::SetWindowTitle(window, oss.str().c_str());
}

}  // namespace PacMan
