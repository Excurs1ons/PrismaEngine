#pragma once

#include "math/MathTypes.h"

namespace Prisma::Graphic {

/* 2D 阴影投射体 — 表示一个能投射阴影的 AABB 物体 */
struct ShadowCaster2D {
    Vector2 position;  // 中心位置
    Vector2 size;      // 宽高
    float softness = 0.5f;  // 保留给未来软阴影使用

    Vector2 GetMin() const { return position - size * 0.5f; }
    Vector2 GetMax() const { return position + size * 0.5f; }
};

} // namespace Prisma::Graphic
