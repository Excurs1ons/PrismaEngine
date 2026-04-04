#include "PacMan.h"
#include "GameBoard.h"
#include "graphic/Renderer2D.h"
#include <cmath>

namespace PacMan {

// ========== PacMan 实现 ==========

PacMan::PacMan()
    : m_position(0, 0)
    , m_speed(PACMAN_SPEED)
    , m_currentDirection(Direction::None)
    , m_nextDirection(Direction::None)
    , m_mouthAngle(0.0f)
    , m_mouthMinAngle(0.0f)
    , m_mouthMaxAngle(45.0f)
    , m_mouthAnimationSpeed(360.0f)
    , m_mouthAnimationTime(0.0f)
    , m_mouthOpening(true)
    , m_aabbWidth(TILE_SIZE * 0.7f)
    , m_aabbHeight(TILE_SIZE * 0.7f)
    , m_board(nullptr)
{
}

void PacMan::Initialize(const glm::ivec2& spawnPosition, GameBoard* board) {
    m_board = board;
    m_position = board->GridToPixel(spawnPosition.x, spawnPosition.y);
    m_currentDirection = Direction::None;
    m_nextDirection = Direction::None;
    m_spriteRenderer.SetPosition(m_position);
    m_spriteRenderer.SetSize(TILE_SIZE * 0.8f, TILE_SIZE * 0.8f);
    m_spriteRenderer.SetColor(Prisma::Color(COLOR_PACMAN.r, COLOR_PACMAN.g, COLOR_PACMAN.b, COLOR_PACMAN.a));
}

void PacMan::Reset() {
    if (m_board) {
        m_position = m_board->GridToPixel(m_board->GetPacmanSpawnPoint().x, m_board->GetPacmanSpawnPoint().y);
    }
    m_currentDirection = Direction::None;
    m_nextDirection = Direction::None;
    m_spriteRenderer.SetPosition(m_position);
}

void PacMan::SetDirection(Direction direction) {
    m_nextDirection = direction;
}

void PacMan::SetNextDirection(Direction direction) {
    m_nextDirection = direction;
}

void PacMan::SetPosition(const glm::vec2& position) {
    m_position = position;
}

glm::ivec2 PacMan::GetGridPosition() const {
    if (m_board) {
        return m_board->PixelToGrid(m_position);
    }
    return glm::ivec2(0, 0);
}

void PacMan::SetMouthAnimation(float minAngle, float maxAngle, float speed) {
    m_mouthMinAngle = minAngle;
    m_mouthMaxAngle = maxAngle;
    m_mouthAnimationSpeed = speed;
}

void PacMan::Update(Prisma::Timestep ts) {
    // 尝试转向
    TryTurn();

    // 更新移动
    if (m_currentDirection != Direction::None && m_board) {
        glm::ivec2 dir = DirectionToVector(m_currentDirection);
        glm::vec2 velocity(dir.x * m_speed, dir.y * m_speed);

        // 计算新位置
        glm::vec2 newPosition = m_position + velocity * static_cast<float>(ts);

        // 检查穿墙
        glm::vec2 tunnelPosition;
        if (m_board->CheckTunnel(newPosition, tunnelPosition)) {
            m_position = tunnelPosition;
        } else {
            // 检查碰撞
            glm::ivec2 gridPos = m_board->PixelToGrid(newPosition);
            if (m_board->IsWalkable(gridPos.x, gridPos.y)) {
                m_position = newPosition;
            } else {
                // 如果不能移动，尝试转向
                TryTurn();
            }
        }

        // 更新精灵渲染器位置
        m_spriteRenderer.SetPosition(m_position);
    }

    // 更新嘴巴动画
    UpdateMouthAnimation(ts);
}

void PacMan::Render() {
    if (auto texture = m_spriteRenderer.GetTexture()) {
        Prisma::Graphic::Renderer2D::DrawQuad(m_position, m_spriteRenderer.GetSize(), texture, m_spriteRenderer.GetColor());
        return;
    }

    Prisma::Graphic::Renderer2D::DrawQuad(m_position, m_spriteRenderer.GetSize(), m_spriteRenderer.GetColor());
}

bool PacMan::IsAtTileCenter() const {
    if (!m_board) {
        return false;
    }

    glm::ivec2 gridPos = m_board->PixelToGrid(m_position);
    glm::vec2 tileCenter = m_board->GridToPixel(gridPos.x, gridPos.y);

    float distance = glm::length(m_position - tileCenter);
    return distance < 2.0f;
}

bool PacMan::CanTurn() const {
    return IsAtTileCenter();
}

void PacMan::TryTurn() {
    if (m_nextDirection == Direction::None || m_nextDirection == m_currentDirection) {
        return;
    }

    if (!m_board) {
        return;
    }

    // 检查是否在格子中心附近
    if (IsAtTileCenter()) {
        glm::ivec2 gridPos = GetGridPosition();
        glm::ivec2 nextDir = DirectionToVector(m_nextDirection);
        glm::ivec2 nextPos = gridPos + nextDir;

        // 检查目标格子是否可通行
        if (m_board->IsWalkable(nextPos.x, nextPos.y)) {
            m_currentDirection = m_nextDirection;
        }
    }
}

void PacMan::UpdateMouthAnimation(Prisma::Timestep ts) {
    // 嘴巴开合动画
    if (m_currentDirection != Direction::None) {
        m_mouthAnimationTime += static_cast<float>(ts) * m_mouthAnimationSpeed;

        // 正弦波动画
        float t = (std::sin(m_mouthAnimationTime * 0.001f) + 1.0f) * 0.5f;
        m_mouthAngle = m_mouthMinAngle + t * (m_mouthMaxAngle - m_mouthMinAngle);
    } else {
        m_mouthAngle = 0.0f;  // 停止时嘴巴闭合
    }
}

void PacMan::SetAABBSize(float width, float height) {
    m_aabbWidth = width;
    m_aabbHeight = height;
}

Prisma::Physics::AABB PacMan::GetAABB() const {
    return Prisma::Physics::AABB(
        m_position.x - m_aabbWidth / 2.0f,
        m_position.y - m_aabbHeight / 2.0f,
        0.0f,
        m_position.x + m_aabbWidth / 2.0f,
        m_position.y + m_aabbHeight / 2.0f,
        0.0f
    );
}

} // namespace PacMan
