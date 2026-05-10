#pragma once

#include <glm/glm.hpp>

namespace PacMan {

// ========== 游戏常量 ==========

constexpr int BOARD_WIDTH = 28;
constexpr int BOARD_HEIGHT = 31;
constexpr int TILE_SIZE = 32;  // 每个方块的像素大小

// ========== 方块类型 ==========

enum class TileType : int {
    Empty = 0,
    Wall = 1,
    Pellet = 2,
    PowerPellet = 3,
    GhostHouse = 4,
    PacmanSpawn = 5
};

// ========== 方向 ==========

enum class Direction : int {
    None = 0,
    Up = 1,
    Down = 2,
    Left = 3,
    Right = 4
};

// 方向向量
constexpr glm::ivec2 DirectionToVector(Direction dir) {
    switch (dir) {
        case Direction::Up:    return {0, -1};
        case Direction::Down:  return {0, 1};
        case Direction::Left:  return {-1, 0};
        case Direction::Right: return {1, 0};
        default:              return {0, 0};
    }
}

// 反方向
constexpr Direction ReverseDirection(Direction dir) {
    switch (dir) {
        case Direction::Up:    return Direction::Down;
        case Direction::Down:  return Direction::Up;
        case Direction::Left:  return Direction::Right;
        case Direction::Right: return Direction::Left;
        default:              return Direction::None;
    }
}

// ========== 幽灵类型 ==========

enum class GhostType : int {
    Blinky = 0,  // 红色 - 直接追逐
    Pinky = 1,   // 粉色 - 提前预判
    Inky = 2,     // 青色 - 混合行为
    Clyde = 3,     // 橙色 - 随机巡逻
    Scared = 4     // 被吓状态
};

// ========== 幽灵状态 ==========

enum class GhostState : int {
    Scatter = 0,  // 散开模式
    Chase = 1,     // 追逐模式
    Frightened = 2, // 惊吓模式
    Eaten = 3      // 被吃模式
};

// ========== 游戏状态 ==========

enum class GameState : int {
    Menu = 0,
    Playing = 1,
    Paused = 2,
    GameOver = 3,
    Victory = 4
};

// ========== 游戏配置 ==========

// 像素/秒（32px 一个格子）
constexpr float PACMAN_SPEED = 120.0f;
constexpr float GHOST_SPEED = 95.0f;
constexpr float GHOST_SCARED_SPEED = 65.0f;

constexpr int PELLET_SCORE = 10;
constexpr int POWER_PELLET_SCORE = 50;
constexpr int GHOST_EAT_SCORE = 200;

constexpr int POWER_MODE_DURATION = 8000;  // 毫秒

// ========== 颜色 ==========

constexpr glm::vec4 COLOR_WALL = {0.0f, 0.0f, 0.39f, 1.0f};
constexpr glm::vec4 COLOR_PELLET = {1.0f, 0.8f, 0.0f, 1.0f};
constexpr glm::vec4 COLOR_POWER_PELLET = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr glm::vec4 COLOR_PACMAN = {1.0f, 1.0f, 0.0f, 1.0f};

// 幽灵颜色
constexpr glm::vec4 COLOR_GHOST_BLINKY = {1.0f, 0.0f, 0.0f, 1.0f};  // 红色
constexpr glm::vec4 COLOR_GHOST_PINKY = {1.0f, 0.7f, 0.7f, 1.0f};  // 粉色
constexpr glm::vec4 COLOR_GHOST_INKY = {0.0f, 1.0f, 1.0f, 1.0f};   // 青色
constexpr glm::vec4 COLOR_GHOST_CLYDE = {1.0f, 0.65f, 0.0f, 1.0f}; // 橙色
constexpr glm::vec4 COLOR_GHOST_SCARED = {0.0f, 0.0f, 0.4f, 1.0f};  // 惊吓色

} // namespace PacMan
