#include "Ghost.h"
#include "GameBoard.h"
#include "PacMan.h"
#include "graphic/Renderer2D.h"
#include "../core/GameConfig.h"
#include <cmath>
#include <algorithm>

namespace PacMan {

// ========== Ghost 实现 ==========

Ghost::Ghost()
    : m_type(GhostType::Blinky)
    , m_state(GhostState::Scatter)
    , m_color(COLOR_GHOST_BLINKY)
    , m_position(0, 0)
    , m_currentDirection(Direction::Left)
    , m_previousDirection(Direction::None)
    , m_normalSpeed(GHOST_SPEED)
    , m_scaredSpeed(GHOST_SCARED_SPEED)
    , m_eatenSpeed(4.0f)
    , m_targetPosition(0, 0)
    , m_aabbWidth(TILE_SIZE * 0.7f)
    , m_aabbHeight(TILE_SIZE * 0.7f)
    , m_board(nullptr)
    , m_pacman(nullptr)
    , m_spawnPosition(0, 0)
    , m_aiUpdateTimer(0.0f)
    , m_frightenedTimer(0.0f)
    , m_frightenedWarningTime(2000.0f)
{
}

void Ghost::Initialize(GhostType type, const glm::ivec2& spawnPosition, GameBoard* board, PacMan* pacman) {
    m_type = type;
    m_spawnPosition = spawnPosition;
    m_board = board;
    m_pacman = pacman;

    // 根据类型设置颜色
    switch (type) {
        case GhostType::Blinky:
            m_color = COLOR_GHOST_BLINKY;
            break;
        case GhostType::Pinky:
            m_color = COLOR_GHOST_PINKY;
            break;
        case GhostType::Inky:
            m_color = COLOR_GHOST_INKY;
            break;
        case GhostType::Clyde:
            m_color = COLOR_GHOST_CLYDE;
            break;
        default:
            m_color = COLOR_GHOST_BLINKY;
            break;
    }

    // 初始位置
    if (board) {
        m_position = board->GridToPixel(spawnPosition.x, spawnPosition.y);
    }

    // 初始状态
    m_state = GhostState::Scatter;
    m_currentDirection = Direction::Left;
}

void Ghost::Reset() {
    if (m_board) {
        m_position = m_board->GridToPixel(m_spawnPosition.x, m_spawnPosition.y);
    }
    m_state = GhostState::Scatter;
    m_currentDirection = Direction::Left;
    m_frightenedTimer = 0.0f;
}

void Ghost::SetType(GhostType type) {
    m_type = type;

    // 更新颜色
    switch (type) {
        case GhostType::Blinky:
            m_color = COLOR_GHOST_BLINKY;
            break;
        case GhostType::Pinky:
            m_color = COLOR_GHOST_PINKY;
            break;
        case GhostType::Inky:
            m_color = COLOR_GHOST_INKY;
            break;
        case GhostType::Clyde:
            m_color = COLOR_GHOST_CLYDE;
            break;
        default:
            m_color = COLOR_GHOST_BLINKY;
            break;
    }
}

void Ghost::SetState(GhostState state) {
    m_state = state;
}

void Ghost::SetPosition(const glm::vec2& position) {
    m_position = position;
}

glm::ivec2 Ghost::GetGridPosition() const {
    if (m_board) {
        return m_board->PixelToGrid(m_position);
    }
    return glm::ivec2(0, 0);
}

void Ghost::SetGridPosition(const glm::ivec2& position) {
    if (m_board) {
        m_position = m_board->GridToPixel(position.x, position.y);
    }
}

float Ghost::GetCurrentSpeed() const {
    switch (m_state) {
        case GhostState::Frightened:
            return m_scaredSpeed;
        case GhostState::Eaten:
            return m_eatenSpeed;
        default:
            return m_normalSpeed;
    }
}

void Ghost::SetColor(const glm::vec4& color) {
    m_color = color;
}

void Ghost::Update(Prisma::Timestep ts) {
    // 更新惊吓计时器
    if (m_state == GhostState::Frightened) {
        m_frightenedTimer += static_cast<float>(ts) * 1000.0f;

        // 检查是否惊吓结束
        if (m_frightenedTimer >= g_GameConfig.powerModeDuration) {
            ChaseMode();  // 返回追逐模式
        }
    }

    // 更新移动
    UpdateMovement(ts);

    // 更新 AI（在格子交点）
    m_aiUpdateTimer += static_cast<float>(ts);
    if (m_aiUpdateTimer >= 0.1f) {  // 每 0.1 秒更新一次
        m_aiUpdateTimer = 0.0f;
        UpdateAI();
    }
}

void Ghost::Render() {
    Prisma::Graphic::Renderer2D::DrawQuad(m_position, m_spriteRenderer.GetSize(), m_spriteRenderer.GetTexture(), m_spriteRenderer.GetColor());
}

void Ghost::UpdateMovement(Prisma::Timestep ts) {
    if (!m_board) {
        return;
    }

    float speed = GetCurrentSpeed();
    glm::ivec2 dir = DirectionToVector(m_currentDirection);

    // 计算新位置
    glm::vec2 newPosition = m_position + glm::vec2(dir.x * speed, dir.y * speed) * static_cast<float>(ts);

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
            // 碰撞，选择新方向
            ChooseNextDirection();
        }
    }

    // 更新精灵渲染器位置
    m_spriteRenderer.SetPosition(m_position);
}

void Ghost::UpdateAI() {
    // 在格子中心时更新目标
    glm::ivec2 gridPos = GetGridPosition();
    glm::vec2 tileCenter;

    if (m_board) {
        tileCenter = m_board->GridToPixel(gridPos.x, gridPos.y);
    }

    float distance = glm::length(m_position - tileCenter);
    if (distance < 2.0f) {
        // 在格子中心，计算目标位置
        m_targetPosition = CalculateTargetPosition();

        // 选择最佳方向
        if (m_state != GhostState::Eaten) {
            m_currentDirection = GetBestDirection(m_targetPosition);
        } else {
            // 被吃模式，回到生成点
            if (gridPos == m_spawnPosition) {
                Revive();
            } else {
                m_currentDirection = GetBestDirection(m_spawnPosition);
            }
        }
    }
}

void Ghost::ChooseNextDirection() {
    // 获取当前格子
    glm::ivec2 gridPos = GetGridPosition();

    // 尝试所有可能的方向（除了反方向）
    std::vector<Direction> possibleDirections;

    for (int i = 1; i <= 4; i++) {
        Direction dir = static_cast<Direction>(i);
        if (dir == ReverseDirection(m_currentDirection)) {
            continue;  // 不能反方向
        }

        glm::ivec2 nextPos = gridPos + DirectionToVector(dir);
        if (m_board && m_board->IsWalkable(nextPos.x, nextPos.y)) {
            possibleDirections.push_back(dir);
        }
    }

    // 如果没有可行方向，可以反方向
    if (possibleDirections.empty()) {
        Direction reverseDir = ReverseDirection(m_currentDirection);
        glm::ivec2 nextPos = gridPos + DirectionToVector(reverseDir);
        if (m_board && m_board->IsWalkable(nextPos.x, nextPos.y)) {
            possibleDirections.push_back(reverseDir);
        }
    }

    // 随机选择一个方向
    if (!possibleDirections.empty()) {
        int randomIndex = std::rand() % possibleDirections.size();
        m_currentDirection = possibleDirections[randomIndex];
    }
}

Direction Ghost::GetBestDirection(const glm::ivec2& target) {
    if (!m_board) {
        return m_currentDirection;
    }

    glm::ivec2 gridPos = GetGridPosition();
    Direction bestDirection = m_currentDirection;
    float bestDistance = std::numeric_limits<float>::max();

    // 尝试所有可能的方向（除了反方向）
    for (int i = 1; i <= 4; i++) {
        Direction dir = static_cast<Direction>(i);
        if (dir == ReverseDirection(m_currentDirection)) {
            continue;
        }

        glm::ivec2 nextPos = gridPos + DirectionToVector(dir);
        if (m_board->IsWalkable(nextPos.x, nextPos.y)) {
            // 计算距离目标的距离
            float distance = glm::length(glm::vec2(target - nextPos));
            if (distance < bestDistance) {
                bestDistance = distance;
                bestDirection = dir;
            }
        }
    }

    return bestDirection;
}

glm::ivec2 Ghost::CalculateTargetPosition() {
    switch (m_state) {
        case GhostState::Scatter:
            return GetScatterTarget();
        case GhostState::Chase:
            return GetChaseTarget();
        case GhostState::Frightened:
        case GhostState::Eaten:
            return GetGridPosition();  // 惊吓时不计算目标
        default:
            return GetGridPosition();
    }
}

glm::ivec2 Ghost::GetChaseTarget() {
    if (!m_pacman) {
        return GetGridPosition();
    }

    glm::ivec2 pacmanPos = m_pacman->GetGridPosition();
    Direction pacmanDir = m_pacman->GetCurrentDirection();
    glm::ivec2 pacmanDirVector = DirectionToVector(pacmanDir);

    // 根据幽灵类型使用不同的追逐策略
    switch (m_type) {
        case GhostType::Blinky:
            // Blinky: 直接追逐吃豆人
            return pacmanPos;

        case GhostType::Pinky:
            // Pinky: 预判吃豆人前方4格
            return pacmanPos + pacmanDirVector * 4;

        case GhostType::Inky:
            // Inky: 混合 Blinky 和吃豆人位置
            {
                glm::ivec2 inkyTarget = pacmanPos * 2 - GetGridPosition();
                return inkyTarget;
            }

        case GhostType::Clyde:
            // Clyde: 如果距离近，随机散开；否则追逐
            {
                float distance = glm::length(glm::vec2(pacmanPos - GetGridPosition()));
                if (distance < 8.0f) {
                    return GetScatterTarget();  // 靠近时散开
                } else {
                    return pacmanPos;  // 远时追逐
                }
            }

        default:
            return pacmanPos;
    }
}

glm::ivec2 Ghost::GetScatterTarget() {
    // 每个幽灵有不同的散开目标
    switch (m_type) {
        case GhostType::Blinky:
            return glm::ivec2(BOARD_WIDTH - 2, BOARD_HEIGHT - 2);  // 右下角
        case GhostType::Pinky:
            return glm::ivec2(2, BOARD_HEIGHT - 2);  // 左下角
        case GhostType::Inky:
            return glm::ivec2(BOARD_WIDTH - 2, 2);  // 右上角
        case GhostType::Clyde:
            return glm::ivec2(2, 2);  // 左上角
        default:
            return glm::ivec2(BOARD_WIDTH / 2, BOARD_HEIGHT / 2);
    }
}

void Ghost::SetAABBSize(float width, float height) {
    m_aabbWidth = width;
    m_aabbHeight = height;
}

Prisma::Physics::AABB Ghost::GetAABB() const {
    return Prisma::Physics::AABB(
        m_position.x - m_aabbWidth / 2.0f,
        m_position.y - m_aabbHeight / 2.0f,
        0.0f,
        m_position.x + m_aabbWidth / 2.0f,
        m_position.y + m_aabbHeight / 2.0f,
        0.0f
    );
}

void Ghost::ChaseMode() {
    m_state = GhostState::Chase;
}

void Ghost::ScatterMode() {
    m_state = GhostState::Scatter;
}

void Ghost::FrightenMode() {
    m_state = GhostState::Frightened;
    m_frightenedTimer = 0.0f;
}

void Ghost::GetEaten() {
    m_state = GhostState::Eaten;
}

void Ghost::Revive() {
    if (m_board) {
        m_position = m_board->GridToPixel(m_spawnPosition.x, m_spawnPosition.y);
    }
    m_state = GhostState::Scatter;
}

} // namespace PacMan
