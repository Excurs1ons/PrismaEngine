#pragma once

#include "../core/GameConstants.h"
#include "graphic/SpriteRenderer.h"
#include "graphic/SpriteAnimation.h"
#include "physics/CollisionSystem.h"
#include <glm/glm.hpp>

namespace PacMan {

class GameBoard;
class PacMan;

/**
 * @brief 幽灵角色
 */
class Ghost {
public:
    Ghost();
    ~Ghost() = default;

    // ========== 初始化 ==========

    /**
     * @brief 初始化幽灵
     */
    void Initialize(GhostType type, const glm::ivec2& spawnPosition, GameBoard* board, PacMan* pacman);

    /**
     * @brief 重置幽灵
     */
    void Reset();

    // ========== 类型和状态 ==========

    /**
     * @brief 获取幽灵类型
     */
    GhostType GetType() const { return m_type; }

    /**
     * @brief 设置幽灵类型
     */
    void SetType(GhostType type);

    /**
     * @brief 获取幽灵状态
     */
    GhostState GetState() const { return m_state; }

    /**
     * @brief 设置幽灵状态
     */
    void SetState(GhostState state);

    // ========== 移动和AI ==========

    /**
     * @brief 获取当前方向
     */
    Direction GetCurrentDirection() const { return m_currentDirection; }

    /**
     * @brief 设置方向
     */
    void SetDirection(Direction direction) { m_currentDirection = direction; }

    /**
     * @brief 设置目标位置（AI导航目标）
     */
    void SetTargetPosition(const glm::ivec2& position);

    /**
     * @brief 获取目标位置
     */
    const glm::ivec2& GetTargetPosition() const { return m_targetPosition; }

    // ========== 位置 ==========

    /**
     * @brief 获取位置
     */
    const glm::vec2& GetPosition() const { return m_position; }

    /**
     * @brief 设置位置
     */
    void SetPosition(const glm::vec2& position);

    /**
     * @brief 获取格子坐标
     */
    glm::ivec2 GetGridPosition() const;

    /**
     * @brief 设置格子坐标
     */
    void SetGridPosition(const glm::ivec2& position);

    // ========== 速度 ==========

    /**
     * @brief 设置正常速度
     */
    void SetNormalSpeed(float speed) { m_normalSpeed = speed; }

    /**
     * @brief 设置惊吓速度
     */
    void SetScaredSpeed(float speed) { m_scaredSpeed = speed; }

    /**
     * @brief 设置被吃后速度
     */
    void SetEatenSpeed(float speed) { m_eatenSpeed = speed; }

    /**
     * @brief 获取当前速度
     */
    float GetCurrentSpeed() const;

    // ========== 渲染 ==========

    /**
     * @brief 获取精灵渲染器
     */
    Prisma::Graphic::SpriteRenderer& GetSpriteRenderer() { return m_spriteRenderer; }

    /**
     * @brief 获取动画组件
     */
    Prisma::Graphic::SpriteAnimationComponent& GetAnimation() { return m_animation; }

    /**
     * @brief 获取颜色
     */
    const glm::vec4& GetColor() const { return m_color; }

    /**
     * @brief 设置颜色
     */
    void SetColor(const glm::vec4& color);

    // ========== 更新 ==========

    /**
     * @brief 更新幽灵（每帧调用）
     */
    void Update(Prisma::Timestep ts);

    /**
     * @brief 渲染幽灵
     */
    void Render();

    // ========== AI ==========

    /**
     * @brief 更新 AI（在格子交点调用）
     */
    void UpdateAI();

    /**
     * @brief 计算目标位置（基于当前状态和幽灵类型）
     */
    glm::ivec2 CalculateTargetPosition();

    // ========== 碰撞包围盒 ==========

    /**
     * @brief 获取碰撞包围盒
     */
    Prisma::Physics::AABB GetAABB() const;

    /**
     * @brief 设置包围盒大小
     */
    void SetAABBSize(float width, float height);

    // ========== 模式切换 ==========

    /**
     * @brief 切换到追逐模式
     */
    void ChaseMode();

    /**
     * @brief 切换到散开模式
     */
    void ScatterMode();

    /**
     * @brief 切换到惊吓模式
     */
    void FrightenMode();

    /**
     * @brief 被吃掉
     */
    void GetEaten();

    // ========== 复活 ==========

    /**
     * @brief 复活幽灵
     */
    void Revive();

private:
    // 幽灵属性
    GhostType m_type = GhostType::Blinky;
    GhostState m_state = GhostState::Scatter;
    glm::vec4 m_color = COLOR_GHOST_BLINKY;

    // 位置和移动
    glm::vec2 m_position{0, 0};
    Direction m_currentDirection = Direction::Left;
    Direction m_previousDirection = Direction::None;
    float m_normalSpeed = GHOST_SPEED;
    float m_scaredSpeed = GHOST_SCARED_SPEED;
    float m_eatenSpeed = 4.0f;

    // AI 目标
    glm::ivec2 m_targetPosition{0, 0};

    // 渲染
    Prisma::Graphic::SpriteRenderer m_spriteRenderer;
    Prisma::Graphic::SpriteAnimationComponent m_animation;

    // 碰撞
    float m_aabbWidth = TILE_SIZE * 0.7f;
    float m_aabbHeight = TILE_SIZE * 0.7f;

    // 游戏引用
    GameBoard* m_board = nullptr;
    PacMan* m_pacman = nullptr;

    // 生成点
    glm::ivec2 m_spawnPosition{0, 0};

    // AI 计时器
    float m_aiUpdateTimer = 0.0f;

    // 惊吓计时器
    float m_frightenedTimer = 0.0f;
    float m_frightenedWarningTime = 2000.0f;  // 惊吓结束前2秒闪烁

    // 辅助方法
    void UpdateMovement(Prisma::Timestep ts);
    void ChooseNextDirection();
    Direction GetBestDirection(const glm::ivec2& target);
    glm::ivec2 GetChaseTarget();
    glm::ivec2 GetScatterTarget();
};

} // namespace PacMan
