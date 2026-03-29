#include "PowerPellet.h"

namespace PacMan {

// ========== PowerPellet 实现 ==========

PowerPellet::PowerPellet()
    : m_position(0, 0)
    , m_gridPosition(0, 0)
    , m_score(POWER_PELLET_SCORE)
    , m_eaten(false)
    , m_visible(true)
    , m_blinkSpeed(3.0f)
    , m_blinkTimer(0.0f)
{
}

void PowerPellet::Initialize(const glm::ivec2& gridPosition, int score) {
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
    m_spriteRenderer.SetSize(TILE_SIZE * 0.8f, TILE_SIZE * 0.8f);
}

void PowerPellet::Update(Prisma::Timestep ts) {
    // 闪烁动画
    m_blinkTimer += static_cast<float>(ts);

    // 使用正弦波创建闪烁效果
    float blinkValue = (std::sin(m_blinkTimer * m_blinkSpeed) + 1.0f) * 0.5f;

    // 应用闪烁到透明度
    m_spriteRenderer.SetOpacity(0.5f + blinkValue * 0.5f);
}

} // namespace PacMan
