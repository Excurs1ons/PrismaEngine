#pragma once

#include "CollisionSystem.h"
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

namespace Prisma {
namespace Physics {

/// 调试线段的顶点对
struct DebugLine {
    glm::dvec3 start;       ///< 起点（世界空间）
    glm::dvec3 end;         ///< 终点（世界空间）
    glm::vec4 color;        ///< RGBA 颜色

    DebugLine(const glm::dvec3& start, const glm::dvec3& end, const glm::vec4& color) noexcept
        : start(start), end(end), color(color) {}
};

/**
 * @brief 物理调试绘制器
 *
 * 收集物理系统的可视化线框数据，兼容现有的 Gizmo 渲染管线。
 * 使用 Clear() 清空后调用各类 Draw*() 方法，最后通过 getLines()
 * 获取所有线段供渲染。
 */
class PhysicsDebugDrawer {
public:
    PhysicsDebugDrawer() = default;
    ~PhysicsDebugDrawer() = default;

    /// 清空所有调试线段
    void Clear() { m_lines.clear(); }

    /// 获取所有收集的线段（只读）
    const std::vector<DebugLine>& getLines() const { return m_lines; }

    /// 获取所有收集的线段（可修改）
    std::vector<DebugLine>& getLines() { return m_lines; }

    // ========== 绘制接口 ==========

    /** @brief 绘制 AABB 线框 */
    void DrawAABB(const AABB& aabb, const glm::vec4& color) {
        // AABB 有 12 条边
        glm::dvec3 corners[8] = {
            { aabb.minX, aabb.minY, aabb.minZ },
            { aabb.maxX, aabb.minY, aabb.minZ },
            { aabb.maxX, aabb.minY, aabb.maxZ },
            { aabb.minX, aabb.minY, aabb.maxZ },
            { aabb.minX, aabb.maxY, aabb.minZ },
            { aabb.maxX, aabb.maxY, aabb.minZ },
            { aabb.maxX, aabb.maxY, aabb.maxZ },
            { aabb.minX, aabb.maxY, aabb.maxZ },
        };

        // 底面 4 条
        addLine(corners[0], corners[1], color);
        addLine(corners[1], corners[2], color);
        addLine(corners[2], corners[3], color);
        addLine(corners[3], corners[0], color);

        // 顶面 4 条
        addLine(corners[4], corners[5], color);
        addLine(corners[5], corners[6], color);
        addLine(corners[6], corners[7], color);
        addLine(corners[7], corners[4], color);

        // 垂直 4 条
        addLine(corners[0], corners[4], color);
        addLine(corners[1], corners[5], color);
        addLine(corners[2], corners[6], color);
        addLine(corners[3], corners[7], color);
    }

    /** @brief 绘制球体线框 */
    void DrawSphere(const glm::dvec3& center, double radius, const glm::vec4& color, int segments = 16) {
        // 3 个正交圆环
        drawRing(center, radius, glm::dvec3(1, 0, 0), color, segments); // X 轴环
        drawRing(center, radius, glm::dvec3(0, 1, 0), color, segments); // Y 轴环
        drawRing(center, radius, glm::dvec3(0, 0, 1), color, segments); // Z 轴环
    }

    /** @brief 绘制胶囊体线框 */
    void DrawCapsule(const glm::dvec3& p1, const glm::dvec3& p2, double radius,
                     const glm::vec4& color, int segments = 12) {
        glm::dvec3 axis = p2 - p1;
        double height = glm::length(axis);
        if (height < 1e-9) {
            DrawSphere(p1, radius, color, segments);
            return;
        }
        axis /= height;

        // 找到两个与轴垂直的正交向量
        glm::dvec3 up = std::abs(axis.y) < 0.99 ? glm::dvec3(0, 1, 0) : glm::dvec3(1, 0, 0);
        glm::dvec3 right = glm::normalize(glm::cross(axis, up));
        glm::dvec3 forward = glm::normalize(glm::cross(right, axis));

        // 中段（圆柱部分）：围绕轴的两个环之间的连线
        double angleStep = 2.0 * M_PI / segments;
        for (int i = 0; i < segments; ++i) {
            double a1 = i * angleStep;
            double a2 = (i + 1) % segments * angleStep;

            glm::dvec3 r1 = right * std::cos(a1) + forward * std::sin(a1);
            glm::dvec3 r2 = right * std::cos(a2) + forward * std::sin(a2);

            // 中段竖直线
            glm::dvec3 bottom1 = p1 + r1 * radius;
            glm::dvec3 top1 = p2 + r1 * radius;
            addLine(bottom1, top1, color);

            // 中段环线（底部环和顶部环）
            if (i < segments) {
                glm::dvec3 bottom2 = p1 + r2 * radius;
                glm::dvec3 top2 = p2 + r2 * radius;
                addLine(bottom1, bottom2, color);
                addLine(top1, top2, color);
            }
        }

        // 半球端盖：连接底部半球
        drawHemisphere(p1, axis, radius, color, segments, false);
        // 半球端盖：连接顶部半球
        drawHemisphere(p2, axis, radius, color, segments, true);
    }

    /** @brief 绘制接触点与法线 */
    void DrawContact(const glm::dvec3& point, const glm::dvec3& normal, const glm::vec4& color) {
        double normalLen = 0.3; // 法线显示长度
        glm::dvec3 normalEnd = point + glm::normalize(normal) * normalLen;

        // 碰撞点（用小十字表示）
        double crossSize = 0.05;
        addLine(point - glm::dvec3(crossSize, 0, 0), point + glm::dvec3(crossSize, 0, 0), color);
        addLine(point - glm::dvec3(0, crossSize, 0), point + glm::dvec3(0, crossSize, 0), color);
        addLine(point - glm::dvec3(0, 0, crossSize), point + glm::dvec3(0, 0, crossSize), color);

        // 法线方向
        addLine(point, normalEnd, glm::vec4(1, 1, 0, 1));
    }

    /** @brief 绘制射线 */
    void DrawRay(const glm::dvec3& origin, const glm::dvec3& direction, double length,
                 const glm::vec4& color) {
        glm::dvec3 end = origin + glm::normalize(direction) * length;
        addLine(origin, end, color);

        // 箭头（小锥体）
        double arrowSize = 0.1 * length;
        glm::dvec3 dir = glm::normalize(direction);
        glm::dvec3 perp = std::abs(dir.x) < 0.99 ? glm::dvec3(1, 0, 0) : glm::dvec3(0, 1, 0);
        glm::dvec3 up = glm::normalize(glm::cross(dir, perp));
        glm::dvec3 right = glm::normalize(glm::cross(dir, up));

        glm::dvec3 arrowBase = end - dir * arrowSize;
        addLine(arrowBase + up * arrowSize * 0.3, end, color);
        addLine(arrowBase - up * arrowSize * 0.3, end, color);
        addLine(arrowBase + right * arrowSize * 0.3, end, color);
        addLine(arrowBase - right * arrowSize * 0.3, end, color);
    }

    /** @brief 绘制关节（两个锚点之间的连线） */
    void DrawJoint(const glm::dvec3& anchorA, const glm::dvec3& anchorB, const glm::vec4& color) {
        // 锚点 A 用小方块标记
        drawSmallCross(anchorA, 0.05, glm::vec4(0, 1, 0, 1));
        // 锚点 B 用小方块标记
        drawSmallCross(anchorB, 0.05, glm::vec4(1, 0, 0, 1));
        // 连线
        addLine(anchorA, anchorB, color);
    }

private:
    std::vector<DebugLine> m_lines;

    void addLine(const glm::dvec3& start, const glm::dvec3& end, const glm::vec4& color) {
        m_lines.emplace_back(start, end, color);
    }

    /// 绘制 2D 圆环（在指定轴的法平面内）
    void drawRing(const glm::dvec3& center, double radius, const glm::dvec3& axis,
                  const glm::vec4& color, int segments) {
        glm::dvec3 up = std::abs(axis.y) < 0.99 ? glm::dvec3(0, 1, 0) : glm::dvec3(1, 0, 0);
        glm::dvec3 right = glm::normalize(glm::cross(axis, up));
        glm::dvec3 forward = glm::normalize(glm::cross(right, axis));

        double angleStep = 2.0 * M_PI / segments;
        for (int i = 0; i < segments; ++i) {
            double a1 = i * angleStep;
            double a2 = (i + 1) % segments * angleStep;

            glm::dvec3 p1 = center + (right * std::cos(a1) + forward * std::sin(a1)) * radius;
            glm::dvec3 p2 = center + (right * std::cos(a2) + forward * std::sin(a2)) * radius;
            addLine(p1, p2, color);
        }
    }

    /// 绘制半球网格
    void drawHemisphere(const glm::dvec3& center, const glm::dvec3& axis, double radius,
                        const glm::vec4& color, int segments, bool top) {
        glm::dvec3 up = std::abs(axis.y) < 0.99 ? glm::dvec3(0, 1, 0) : glm::dvec3(1, 0, 0);
        glm::dvec3 right = glm::normalize(glm::cross(axis, up));
        glm::dvec3 forward = glm::normalize(glm::cross(right, axis));

        int latSegments = segments / 2;
        double sign = top ? 1.0 : -1.0;
        double latStart = top ? 0.0 : -M_PI_2;
        double latEnd = top ? M_PI_2 : 0.0;

        for (int i = 0; i < latSegments; ++i) {
            double lat1 = latStart + (i) * (latEnd - latStart) / latSegments;
            double lat2 = latStart + (i + 1) * (latEnd - latStart) / latSegments;

            double r1 = std::cos(lat1) * radius;
            double r2 = std::cos(lat2) * radius;
            double y1 = std::sin(lat1) * radius;
            double y2 = std::sin(lat2) * radius;

            double angleStep = 2.0 * M_PI / segments;
            for (int j = 0; j < segments; ++j) {
                double a1 = j * angleStep;
                double a2 = (j + 1) % segments * angleStep;

                glm::dvec3 p1 = center + axis * y1 * sign
                    + (right * std::cos(a1) + forward * std::sin(a1)) * r1;
                glm::dvec3 p2 = center + axis * y2 * sign
                    + (right * std::cos(a2) + forward * std::sin(a2)) * r2;
                glm::dvec3 p3 = center + axis * y1 * sign
                    + (right * std::cos(a2) + forward * std::sin(a2)) * r1;
                glm::dvec3 p4 = center + axis * y2 * sign
                    + (right * std::cos(a1) + forward * std::sin(a1)) * r2;

                addLine(p1, p3, color);
                addLine(p1, p4, color);
            }
        }
    }

    /// 绘制小十字
    void drawSmallCross(const glm::dvec3& center, double size, const glm::vec4& color) {
        addLine(center - glm::dvec3(size, 0, 0), center + glm::dvec3(size, 0, 0), color);
        addLine(center - glm::dvec3(0, size, 0), center + glm::dvec3(0, size, 0), color);
        addLine(center - glm::dvec3(0, 0, size), center + glm::dvec3(0, 0, size), color);
    }
};

} // namespace Physics
} // namespace Prisma
