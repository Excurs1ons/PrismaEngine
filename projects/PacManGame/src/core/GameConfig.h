#pragma once

#include "GameConstants.h"
#include <string>

namespace PacMan {

// ========== 游戏配置结构 ==========

struct GameConfig {
    // 窗口设置
    int windowWidth = BOARD_WIDTH * TILE_SIZE;
    int windowHeight = BOARD_HEIGHT * TILE_SIZE;
    std::string windowTitle = "Pac-Man - PrismaEngine";

    // 游戏速度
    float pacmanSpeed = PACMAN_SPEED;
    float ghostSpeed = GHOST_SPEED;
    float ghostScaredSpeed = GHOST_SCARED_SPEED;

    // 得分设置
    int pelletScore = PELLET_SCORE;
    int powerPelletScore = POWER_PELLET_SCORE;
    int ghostEatScore = GHOST_EAT_SCORE;

    // 能量药丸持续时间（毫秒）
    int powerModeDuration = POWER_MODE_DURATION;

    // 生命值
    int initialLives = 3;

    // 是否开启 2D 光照
    bool enableLighting = true;

    // 光照设置
    float ambientLightIntensity = 0.3f;
    float pacmanLightRadius = 4.0f;
    float pacmanLightIntensity = 1.5f;

    // 幽灵光源设置
    float ghostLightRadius = 3.0f;
    float ghostLightIntensity = 0.8f;

    // 启用调试模式
    bool debugMode = false;

    // 启用粒子效果
    bool enableParticles = true;

    // 音量设置
    float masterVolume = 0.8f;
    float sfxVolume = 0.8f;
    float musicVolume = 0.5f;

    // 是否开启声音
    bool enableSound = true;
    bool enableMusic = true;
};

// ========== 全局配置实例 ==========

inline GameConfig g_GameConfig;

} // namespace PacMan
