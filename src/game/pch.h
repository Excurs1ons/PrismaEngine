#pragma once

// 游戏项目的预编译头文件
// 包含游戏常用的头文件和定义

// 引擎核心头文件（总是需要）
#include "../engine/pch.h"

// 注意：游戏特定的头文件（GameObject、Scene等）不在PCH中包含
// 它们会在各自的源文件中被包含

// C++标准库额外包含（游戏常用）
#include <random>
#include <fstream>
#include <filesystem>
#include <sstream>

// 游戏特定类型
using GameObjectID = uint32;
using ComponentID = uint32;
using EntityID = uint32;

// 游戏层定义
enum class GameLayer : uint32 {
    Default = 0,
    UI = 1,
    Background = 2,
    Foreground = 3,
    Player = 4,
    Enemy = 5,
    Projectile = 6,
    Pickup = 7,
    Environment = 8
};

// 游戏标签
using Tag = std::string;
static constexpr Tag TAG_PLAYER = "Player";
static constexpr Tag TAG_ENEMY = "Enemy";
static constexpr Tag TAG_UI = "UI";
static constexpr Tag TAG_CAMERA = "MainCamera";
static constexpr Tag TAG_LIGHT = "Light";

// 游戏常用宏
#define GAME_MAX_GAMEOBJECTS 10000
#define GAME_MAX_COMPONENTS_PER_OBJECT 64

// 游戏特定工具函数
inline bool IsPlayerLayer(GameLayer layer) {
    return layer == GameLayer::Player;
}

inline bool IsEnemyLayer(GameLayer layer) {
    return layer == GameLayer::Enemy;
}

inline bool IsProjectileLayer(GameLayer layer) {
    return layer == GameLayer::Projectile;
}

// 游戏配置
namespace GameConfig {
    constexpr int TARGET_FPS = 60;
    constexpr float FIXED_TIMESTEP = 1.0f / TARGET_FPS;
    constexpr int MAX_AUDIO_CHANNELS = 32;
    constexpr float GRAVITY = 9.81f;
    constexpr float MAX_SLOPE_ANGLE = 45.0f;
}