#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Prisma::Physics2D {

/**
 * @brief 2D Axis-Aligned Bounding Box
 *
 * 纯 2D AABB，仅包含 X/Y 轴（无 Z 轴开销）。
 * 使用 float 精度，适合像素级碰撞检测。
 */
struct AABB2D {
    float minX, minY, maxX, maxY;

    AABB2D() : minX(0), minY(0), maxX(0), maxY(0) {}
    AABB2D(float x1, float y1, float x2, float y2)
        : minX(x1), minY(y1), maxX(x2), maxY(y2) {}

    float GetWidth() const { return maxX - minX; }
    float GetHeight() const { return maxY - minY; }
    glm::vec2 GetCenter() const { return {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f}; }
    glm::vec2 GetSize() const { return {GetWidth(), GetHeight()}; }

    bool Intersects(const AABB2D& other) const {
        return minX < other.maxX && maxX > other.minX &&
               minY < other.maxY && maxY > other.minY;
    }

    bool Contains(float px, float py) const {
        return px >= minX && px <= maxX && py >= minY && py <= maxY;
    }

    static AABB2D FromCenterSize(float cx, float cy, float w, float h) {
        return AABB2D(cx - w * 0.5f, cy - h * 0.5f,
                      cx + w * 0.5f, cy + h * 0.5f);
    }

    AABB2D Translated(float dx, float dy) const {
        return AABB2D(minX + dx, minY + dy, maxX + dx, maxY + dy);
    }
};

} // namespace Prisma::Physics2D
