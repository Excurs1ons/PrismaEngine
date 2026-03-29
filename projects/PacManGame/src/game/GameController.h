#pragma once

#include "../core/GameConstants.h"
#include "PacMan.h"
#include "Ghost.h"
#include "GameBoard.h"
#include "graphic/OrthographicCamera.h"
#include <vector>
#include <memory>

namespace PacMan {

class GameController {
public:
    GameController();
    ~GameController() = default;

    // ========== 初始化 ==========

    /**
     * @brief 初始化游戏
     */
    void Initialize();

    /**
     * @brief 关闭游戏
     */
    void Shutdown();

    // ========== 游戏循环 ==========

    /**
     * @brief 更新游戏（每帧调用）
     */
    void Update(Prisma::Timestep ts);

    /**
     * @brief 渲染游戏
     */
    void Render();

    // ========== 游戏状态 ==========

    /**
     * @brief 获取游戏状态
     */
    GameState GetGameState() const { return m_gameState; }

    /**
     * @brief 设置游戏状态
     */
    void SetGameState(GameState state);

    /**
     * @brief 开始游戏
     */
    void StartGame();

    /**
     * @brief 暂停游戏
     */
    void PauseGame();

    /**
     * @brief 继续游戏
     */
    void ResumeGame();

    /**
     * @brief 重置游戏
     */
    void ResetGame();

    // ========== 得分系统 ==========

    /**
     * @brief 获取得分
     */
    int GetScore() const { return m_score; }

    /**
     * @brief 添加得分
     */
    void AddScore(int score);

    /**
     * @brief 获取最高分
     */
    int GetHighScore() const { return m_highScore; }

    /**
     * @brief 设置最高分
     */
    void SetHighScore(int score);

    // ========== 生命系统 ==========

    /**
     * @brief 获取剩余生命
     */
    int GetLives() const { return m_lives; }

    /**
     * @brief 设置生命
     */
    void SetLives(int lives);

    /**
     * @brief 减少生命
     */
    void LoseLife();

    /**
     * @brief 吃豆人死亡
     */
    void OnPacmanDeath();

    // ========== 能量模式 ==========

    /**
     * @brief 激活能量模式
     */
    void ActivatePowerMode();

    /**
     * @brief 是否在能量模式
     */
    bool IsPowerMode() const { return m_powerModeActive; }

    // ========== 游戏对象访问 ==========

    /**
     * @brief 获取吃豆人
     */
    PacMan& GetPacman() { return m_pacman; }

    /**
     * @brief 获取幽灵列表
     */
    std::vector<Ghost>& GetGhosts() { return m_ghosts; }

    /**
     * @brief 获取幽灵（按索引）
     */
    Ghost& GetGhost(int index) { return m_ghosts[index]; }

    /**
     * @brief 获取游戏板
     */
    GameBoard& GetGameBoard() { return m_board; }

    /**
     * @brief 获取相机
     */
    Prisma::Graphic::OrthographicCamera& GetCamera() { return m_camera; }

    // ========== 关卡系统 ==========

    /**
     * @brief 获取当前关卡
     */
    int GetCurrentLevel() const { return m_currentLevel; }

    /**
     * @brief 设置关卡
     */
    void SetLevel(int level);

    /**
     * @brief 下一关
     */
    void NextLevel();

    // ========== 输入处理 ==========

    /**
     * @brief 处理键盘输入
     */
    void OnKeyPress(int keyCode);
    void OnKeyRelease(int keyCode);

    // ========== 碰撞检测 ==========

    /**
     * @brief 检查吃豆人与豆子碰撞
     */
    void CheckPelletCollision();

    /**
     * @brief 检查吃豆人与幽灵碰撞
     */
    void CheckGhostCollision();

private:
    // 游戏状态
    GameState m_gameState = GameState::Menu;
    int m_score = 0;
    int m_highScore = 0;
    int m_lives = 3;
    int m_currentLevel = 1;

    // 能量模式
    bool m_powerModeActive = false;
    float m_powerModeTimer = 0.0f;
    float m_powerModeDuration = POWER_MODE_DURATION / 1000.0f;

    // 游戏对象
    GameBoard m_board;
    PacMan m_pacman;
    std::vector<Ghost> m_ghosts;

    // 相机
    Prisma::Graphic::OrthographicCamera m_camera;

    // 输入缓存
    Direction m_inputDirection = Direction::None;

    // 辅助方法
    void InitializeGhosts();
    void CheckLevelComplete();
    void UpdatePowerMode(Prisma::Timestep ts);
    void UpdateUI();
    void RenderGame();
    void RenderUI();
};

} // namespace PacMan
