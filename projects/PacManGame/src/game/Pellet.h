#pragma once

#include "../core/GameConstants.h"
#include "graphic/SpriteRenderer.h"
#include <glm/glm.hpp>

namespace PacMan {

/**
 * @brief 普通豆子
 */
class Pellet {
public:
    Pellet();
    ~Pellet() = default;

    /**
     * @brief 初始化豆子
     */
    void Initialize(const glm::ivec2& gridPosition, int score = PELLET_SCORE);

    /**
     * @brief 获取位置
     */
    const glm::vec2& GetPosition() const { return m_position; }

    /**
     * @brief 获取格子坐标
     */
    const glm::ivec2& GetGridPosition() const { return m_gridPosition; }

    /**
     * @brief 获取得分
     */
    int GetScore() const { return m_score; }

    /**
     * @brief 是否被吃掉
     */
    bool IsEaten() const { return m_eaten; }

    /**
     * @brief 吃掉豆子
     */
    void Eat() { m_eaten = true; }

    /**
     * @brief 重置豆子
     */
    void Reset() { m_eaten = false; }

    /**
     * @brief 获取渲染器
     */
    Prisma::Graphic::SpriteRenderer& GetSpriteRenderer() { return m_spriteRenderer; }

    /**
     * @brief 设置是否可见
     */
    void SetVisible(bool visible) { m_visible = visible; }

    /**
     * @brief 是否可见
     */
    bool IsVisible() const { return m_visible && !m_eaten; }

private:
    glm::vec2 m_position{0, 0};
    glm::ivec2 m_gridPosition{0, 0};
    int m_score = PELLET_SCORE;
    bool m_eaten = false;
    bool m_visible = true;

    Prisma::Graphic::SpriteRenderer m_spriteRenderer;
};

} // namespace PacMan
