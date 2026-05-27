#pragma once

#include "math/MathTypes.h"
#include <cstdint>
#include <string>
#include <array>
#include <vector>

namespace Prisma::Terrain {

/**
 * @brief 地形配置 - 控制地形生成、LOD 和纹理的全局参数
 *
 * 对应 Minecraft 的地形渲染配置概念，但更通用
 */
struct TerrainConfig {
    // ========== 基本参数 ==========

    uint32_t chunkSize = 64;            // 地形块大小（世界单位）
    uint32_t heightmapResolution = 1024; // 高度图分辨率（像素）
    float heightScale = 1.0f;           // 高度缩放因子
    float heightOffset = 0.0f;          // 高度偏移

    // ========== LOD 参数 ==========

    // LOD 等级的距离阈值（从近到远）
    // LOD 0 = 最近（最高细节），LOD N = 最远（最低细节）
    std::array<float, 6> lodDistances = {
        0.0f,     // 总是显示
        60.0f,    // LOD 1 开始距离
        150.0f,   // LOD 2 开始距离
        350.0f,   // LOD 3 开始距离
        750.0f,   // LOD 4 开始距离
        1500.0f   // LOD 5 开始距离
    };

    // 每个 LOD 等级的顶点网格大小（每边的顶点数）
    // 更大的数字 = 更高的顶点密度
    std::array<uint32_t, 6> lodVertexCounts = {
        65,   // LOD 0: 65x65 = 4225 顶点
        33,   // LOD 1: 33x33 = 1089 顶点
        17,   // LOD 2: 17x17 = 289 顶点
        9,    // LOD 3:  9x9  = 81 顶点
        5,    // LOD 4:  5x5  = 25 顶点
        5     // LOD 5:  5x5  = 25 顶点
    };

    // 每个 LOD 等级覆盖的世界范围（半径）
    std::array<float, 6> lodRadii = {
        32.0f,    // LOD 0 半径
        80.0f,    // LOD 1 半径
        180.0f,   // LOD 2 半径
        400.0f,   // LOD 3 半径
        800.0f,   // LOD 4 半径
        1600.0f   // LOD 5 半径
    };

    // ========== 纹理参数 ==========

    // 纹理层定义
    struct TextureLayerDef {
        std::string name;           // 纹理名称（对应 TextureAtlas 中的名称）
        std::string texturePath;    // 纹理文件路径
        float blendHeight = 0.0f;   // 混合中心高度
        float blendWidth = 20.0f;   // 混合过渡宽度
        float slopeFactor = 0.0f;   // 坡度影响系数 (0-1)
        Vector2 tiling = Vector2(32.0f); // UV 平铺
    };

    std::vector<TextureLayerDef> textureLayers;

    // UV 平铺缩放
    float tileScale = 32.0f;

    // ========== 碰撞参数 ==========

    float collisionMeshResolution = 1.0f; // 碰撞网格分辨率（世界单位/顶点）

    // ========== 默认构造函数 ==========
    TerrainConfig() {
        // 填充默认纹理层
        textureLayers = {
            { "grass",   "textures/terrain/grass.png",   10.0f,  15.0f, 0.0f, Vector2(32.0f) },
            { "rock",    "textures/terrain/rock.png",    50.0f,  20.0f, 0.7f, Vector2(32.0f) },
            { "sand",    "textures/terrain/sand.png",    5.0f,   10.0f, 0.0f, Vector2(64.0f) },
            { "snow",    "textures/terrain/snow.png",    100.0f, 15.0f, 0.3f, Vector2(64.0f) }
        };
    }
};

} // namespace Prisma::Terrain
