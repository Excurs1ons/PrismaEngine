#pragma once

#include "Types.h"
#include <vector>
#include <memory>

namespace PrismaEngine {

// ============================================================================
// 前向声明
// ============================================================================

class Tileset;
struct TileLayer;

// ============================================================================
// 动画帧
// ============================================================================

struct Frame {
    int tileId = 0;      // 局部瓦片 ID
    int duration = 0;    // 持续时间 (毫秒)
};

// ============================================================================
// 碰撞形状
// ============================================================================

enum class CollisionShapeType {
    Rectangle,
    Ellipse,
    Polygon,
    Polyline
};

struct CollisionShape {
    CollisionShapeType type;
    // 形状数据
    std::vector<std::pair<float, float>> points;  // 相对坐标 (浮点数)
};

// ============================================================================
// 瓦片定义 (用于 Tileset 中的特殊瓦片)
// ============================================================================

struct Tile {
    int id = -1;                       // 局部瓦片 ID
    std::string type;                  // 瓦片类型
    std::vector<Frame> animation;      // 动画帧
    std::vector<CollisionShape> collisionShapes;  // 碰撞形状
    PropertyMap properties;            // 自定义属性
    float probability = 1.0f;          // 概率权重 (用于自动绘制)
    std::string imagePath;             // 替代图像 (Image Collection)

    // 地形信息 (已弃用但保留兼容性)
    int terrainTopLeft = -1;
    int terrainTopRight = -1;
    int terrainBottomLeft = -1;
    int terrainBottomRight = -1;

    // 图层
    std::vector<std::shared_ptr<TileLayer>> objectGroup;  // 对象组 (作为图层)

    // 帧
    std::vector<Frame> frames;

    // 检查是否有动画
    bool HasAnimation() const { return !animation.empty() || !frames.empty(); }

    // 获取动画总时长
    int GetAnimationDuration() const {
        const auto& animFrames = animation.empty() ? frames : animation;
        int total = 0;
        for (const auto& frame : animFrames) {
            total += frame.duration;
        }
        return total;
    }
};

// ============================================================================
// 瓦片图层中的瓦片 (运行时)
// ============================================================================

struct TileInstance {
    uint32_t gid = 0;          // 全局瓦片 ID (包含翻转标志)
    bool flippedHorz = false;  // 水平翻转
    bool flippedVert = false;  // 垂直翻转
    bool flippedDiag = false;  // 对角翻转

    // 从 GID 解析
    void FromGid(uint32_t gidValue) {
        gid = gidValue;
        flippedHorz = GIDHelper::IsHorizontallyFlipped(gidValue);
        flippedVert = GIDHelper::IsVerticallyFlipped(gidValue);
        flippedDiag = GIDHelper::IsDiagonallyFlipped(gidValue);
    }

    // 获取纯 GID
    uint32_t GetPureGid() const {
        return GIDHelper::GetPureGid(gid);
    }

    // 检查是否为空瓦片 (0 或 0xFFFFFFFF)
    bool IsEmpty() const {
        return GetPureGid() == 0 || GetPureGid() == 0xFFFFFFFF;
    }
};

} // namespace PrismaEngine
