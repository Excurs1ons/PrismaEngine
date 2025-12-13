#pragma once

// 游戏项目的预编译头文件
// 包含游戏常用的头文件和定义

// 引擎核心头文件（总是需要）
#include "../engine/pch.h"

// 游戏特定头文件（不经常改变的）
#include "core/GameObject.h"
#include "core/Scene.h"
#include "core/Component.h"
#include "core/Transform.h"

// 游戏系统
#include "systems/GameSystem.h"
#include "systems/RenderSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/AudioSystem.h"
#include "systems/InputSystem.h"
#include "systems/ScriptSystem.h"

// 游戏组件
#include "components/RenderComponent.h"
#include "components/PhysicsComponent.h"
#include "components/AudioComponent.h"
#include "components/ScriptComponent.h"
#include "components/CameraComponent.h"
#include "components/LightComponent.h"

// 游戏资源
#include "resources/Mesh.h"
#include "resources/Texture.h"
#include "resources/Material.h"
#include "resources/AudioClip.h"
#include "resources/Shader.h"
#include "resources/SceneAsset.h"

// 数学库
#include "math/Vector3.h"
#include "math/Vector2.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/MathUtils.h"

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