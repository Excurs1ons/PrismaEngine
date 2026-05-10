#include "Pellet.h"

namespace PacMan {

// ========== Pellet 实现 ==========

Pellet::Pellet()
    : m_position(0, 0)
    , m_gridPosition(0, 0)
    , m_score(PELLET_SCORE)
    , m_eaten(false)
    , m_visible(true)
{
}

void Pellet::Initialize(const glm::ivec2& gridPosition, int score) {
    m_gridPosition = gridPosition;
    m_score = score;
    m_eaten = false;
    m_visible = true;

    // 计算像素位置
    m_position = glm::vec2(
        static_cast<float>(gridPosition.x * TILE_SIZE) + TILE_SIZE / 2.0f,
        static_cast<float>(gridPosition.y * TILE_SIZE) + TILE_SIZE / 2.0f
    );

    // 设置精灵渲染器
    m_spriteRenderer.SetPosition(m_position);
    m_spriteRenderer.SetSize(TILE_SIZE * 0.6f, TILE_SIZE * 0.6f);
}

} // namespace PacMan
