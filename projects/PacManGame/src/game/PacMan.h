#pragma once

#include "../core/GameConstants.h"
#include "graphic/SpriteRenderer.h"
#include "graphic/SpriteAnimation.h"
#include "physics/CollisionSystem.h"
#include <glm/glm.hpp>

namespace PacMan {

class GameBoard;

/**
 * @brief 吃豆人角色
 */
class PacMan {
public:
    PacMan();
    ~PacMan() = default;

    // ========== 初始化 ==========

    /**
     * @brief 初始化吃豆人
     */
    void Initialize(const glm::ivec2& spawnPosition, GameBoard* board);

    /**
     * @brief 重置吃豆人
     */
    void Reset();

    // ========== 移动控制 ==========

    /**
     * @brief 设置方向
     * @param direction 移动方向
     */
    void SetDirection(Direction direction);

    /**
     * @brief 设置下一个方向（缓存）
     * @param direction 下一个移动方向
     */
    void SetNextDirection(Direction direction);

    /**
     * @brief 获取当前方向
     */
    Direction GetCurrentDirection() const { return m_currentDirection; }

    /**
     * @brief 获取下一个方向
     */
    Direction GetNextDirection() const { return m_nextDirection; }

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

    // ========== 速度 ==========

    /**
     * @brief 设置速度
     */
    void SetSpeed(float speed) { m_speed = speed; }

    /**
     * @brief 获取速度
     */
    float GetSpeed() const { return m_speed; }

    // ========== 渲染 ==========

    /**
     * @brief 获取精灵渲染器
     */
    Prisma::Graphic::SpriteRenderer& GetSpriteRenderer() { return m_spriteRenderer; }

    /**
     * @brief 获取动画组件
     */
    Prisma::Graphic::SpriteAnimationComponent& GetAnimation() { return m_animation; }

    // ========== 更新 ==========

    /**
     * @brief 更新吃豆人（每帧调用）
     */
    void Update(Prisma::Timestep ts);

    /**
     * @brief 渲染吃豆人
     */
    void Render();

    // ========== 状态 ==========

    /**
     * @brief 是否正在移动
     */
    bool IsMoving() const { return m_currentDirection != Direction::None; }

    /**
     * @brief 是否在格子中心
     */
    bool IsAtTileCenter() const;

    /**
     * @brief 是否可以转向
     */
    bool CanTurn() const;

    // ========== 动画状态 ==========

    /**
     * @brief 设置嘴巴动画参数
     */
    void SetMouthAnimation(float minAngle, float maxAngle, float speed);

    /**
     * @brief 获取当前嘴巴角度
     */
    float GetMouthAngle() const { return m_mouthAngle; }

    // ========== 碰撞包围盒 ==========

    /**
     * @brief 获取碰撞包围盒
     */
    Prisma::Physics::AABB GetAABB() const;

    /**
     * @brief 设置包围盒大小
     */
    void SetAABBSize(float width, float height);

private:
    // 位置和移动
    glm::vec2 m_position{0, 0};
    float m_speed = PACMAN_SPEED;
    Direction m_currentDirection = Direction::None;
    Direction m_nextDirection = Direction::None;

    // 渲染
    Prisma::Graphic::SpriteRenderer m_spriteRenderer;
    Prisma::Graphic::SpriteAnimationComponent m_animation;

    // 动画状态
    float m_mouthAngle = 0.0f;
    float m_mouthMinAngle = 0.0f;
    float m_mouthMaxAngle = 45.0f;
    float m_mouthAnimationSpeed = 360.0f;
    float m_mouthAnimationTime = 0.0f;
    bool m_mouthOpening = true;

    // 碰撞
    float m_aabbWidth = TILE_SIZE * 0.7f;
    float m_aabbHeight = TILE_SIZE * 0.7f;

    // 游戏板引用
    GameBoard* m_board = nullptr;

    // 辅助方法
    void TryTurn();
    void UpdateMouthAnimation(Prisma::Timestep ts);
};

} // namespace PacMan
