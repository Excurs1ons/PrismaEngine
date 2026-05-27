#pragma once

#include "Export.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <vector>
#include <cstdint>
#include <cmath>
#include <random>
#include <limits>

namespace Prisma {
namespace Navigation {

/**
 * @brief 导航网格区域类型
 */
enum class AreaType : uint8_t {
    Walkable   = 0,
    Water      = 1,
    Lava       = 2,
    Door       = 3,
    Jump       = 4,
    Obstacle   = 255
};

/**
 * @brief 导航多边形（三角形）
 *
 * 对应 NavMesh 中的一个三角形面片。
 * 每个三角形有 3 个顶点和 3 个邻居索引（-1 = 无邻居/边界边）。
 */
struct ENGINE_API NavPolygon {
    glm::dvec3 vertices[3];       // 三角形顶点（右手坐标系，已排序）
    int32_t    neighbors[3];      // 邻居多边形索引（-1 表示边界）
    AreaType   areaType;          // 区域类型
    uint32_t   id;                // 多边形 ID

    NavPolygon() : areaType(AreaType::Walkable), id(0) {
        neighbors[0] = neighbors[1] = neighbors[2] = -1;
    }

    /** 计算多边形中心 */
    glm::dvec3 GetCenter() const {
        return (vertices[0] + vertices[1] + vertices[2]) / 3.0;
    }

    /** 检查点是否在多边形内（重心坐标法） */
    bool ContainsPoint(const glm::dvec3& point) const {
        // 使用重心坐标判断点是否在三角形内
        const auto& a = vertices[0];
        const auto& b = vertices[1];
        const auto& c = vertices[2];

        glm::dvec3 v0 = c - a;
        glm::dvec3 v1 = b - a;
        glm::dvec3 v2 = point - a;

        double dot00 = glm::dot(v0, v0);
        double dot01 = glm::dot(v0, v1);
        double dot02 = glm::dot(v0, v2);
        double dot11 = glm::dot(v1, v1);
        double dot12 = glm::dot(v1, v2);

        double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
        double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

        const double eps = 1e-6;
        return (u >= -eps) && (v >= -eps) && (u + v <= 1.0 + eps);
    }

    /** 获取点到多边形平面的投影点 */
    glm::dvec3 ClosestPointOnPolygon(const glm::dvec3& point) const {
        const auto& a = vertices[0];
        const auto& b = vertices[1];
        const auto& c = vertices[2];

        glm::dvec3 ab = b - a;
        glm::dvec3 ac = c - a;
        glm::dvec3 normal = glm::cross(ab, ac);
        double len = glm::length(normal);
        if (len < 1e-10) return a;
        normal /= len;

        glm::dvec3 ap = point - a;
        double dist = glm::dot(ap, normal);
        return point - normal * dist;
    }

    /** 获取多边形法线 */
    glm::dvec3 GetNormal() const {
        glm::dvec3 ab = vertices[1] - vertices[0];
        glm::dvec3 ac = vertices[2] - vertices[0];
        glm::dvec3 n = glm::cross(ab, ac);
        double len = glm::length(n);
        if (len < 1e-10) return glm::dvec3(0, 1, 0);
        return n / len;
    }

    /** 获取三角形面积 */
    double GetArea() const {
        glm::dvec3 ab = vertices[1] - vertices[0];
        glm::dvec3 ac = vertices[2] - vertices[0];
        return 0.5 * glm::length(glm::cross(ab, ac));
    }
};

/**
 * @brief 导航网格
 *
 * 存储为三角形多边形列表，支持点查询和随机点采样。
 * 使用 Prisma::Navigation 命名空间。
 */
class ENGINE_API NavMesh {
public:
    NavMesh() = default;
    explicit NavMesh(std::vector<NavPolygon> polygons)
        : m_polygons(std::move(polygons)) {}

    /** 获取多边形数量 */
    size_t GetPolygonCount() const { return m_polygons.size(); }

    /** 获取多边形引用 */
    const NavPolygon& GetPolygon(size_t index) const { return m_polygons[index]; }
    NavPolygon& GetPolygon(size_t index) { return m_polygons[index]; }

    /** 获取多边形列表 */
    const std::vector<NavPolygon>& GetPolygons() const { return m_polygons; }
    std::vector<NavPolygon>& GetPolygons() { return m_polygons; }

    /**
     * @brief 查找包含点的多边形
     * @param point 查询点（世界坐标）
     * @return 多边形索引，-1 表示未找到
     */
    int32_t FindPolygonContainingPoint(const glm::dvec3& point) const {
        for (size_t i = 0; i < m_polygons.size(); ++i) {
            if (m_polygons[i].ContainsPoint(point)) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    /**
     * @brief 查找最近的多边形（当点不在任何多边形内时使用）
     * @param point 查询点
     * @param outPolyIdx 输出：最近的多边形索引
     * @param outClosestPoint 输出：多边形上最近的点
     * @return 是否找到
     */
    bool FindClosestPolygon(const glm::dvec3& point,
                            int32_t& outPolyIdx,
                            glm::dvec3& outClosestPoint) const {
        if (m_polygons.empty()) return false;

        double minDist = std::numeric_limits<double>::max();
        outPolyIdx = -1;

        for (size_t i = 0; i < m_polygons.size(); ++i) {
            glm::dvec3 cp = m_polygons[i].ClosestPointOnPolygon(point);
            double dist = glm::distance2(point, cp);
            if (dist < minDist) {
                minDist = dist;
                outPolyIdx = static_cast<int32_t>(i);
                outClosestPoint = cp;
            }
        }
        return outPolyIdx >= 0;
    }

    /**
     * @brief 在多边形内获取随机点
     * @param polyIdx 多边形索引
     * @param rng 随机数引擎
     * @return 多边形内的随机点
     */
    glm::dvec3 GetRandomPointInPolygon(int32_t polyIdx,
                                       std::mt19937& rng) const {
        if (polyIdx < 0 || polyIdx >= static_cast<int32_t>(m_polygons.size())) {
            return glm::dvec3(0);
        }

        const auto& poly = m_polygons[polyIdx];
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        // 在三角形内均匀采样：使用重心坐标
        double r1 = dist(rng);
        double r2 = dist(rng);

        if (r1 + r2 > 1.0) {
            r1 = 1.0 - r1;
            r2 = 1.0 - r2;
        }

        return poly.vertices[0]
             + r1 * (poly.vertices[1] - poly.vertices[0])
             + r2 * (poly.vertices[2] - poly.vertices[0]);
    }

    /**
     * @brief 在导航网格上获取随机点
     * @param rng 随机数引擎
     * @return 网格上的随机可走点
     */
    glm::dvec3 GetRandomPoint(std::mt19937& rng) const {
        if (m_polygons.empty()) return glm::dvec3(0);

        std::uniform_int_distribution<size_t> dist(0, m_polygons.size() - 1);
        return GetRandomPointInPolygon(static_cast<int32_t>(dist(rng)), rng);
    }

    /** 获取两个多边形之间的共享边中点 */
    bool GetEdgeMidpoint(int32_t polyA, int32_t polyB,
                         glm::dvec3& outMidpoint) const {
        if (polyA < 0 || polyA >= static_cast<int32_t>(m_polygons.size()) ||
            polyB < 0 || polyB >= static_cast<int32_t>(m_polygons.size())) {
            return false;
        }

        const auto& a = m_polygons[polyA];
        const auto& b = m_polygons[polyB];

        // 找到两个多边形之间的共享边
        for (int ei = 0; ei < 3; ++ei) {
            if (a.neighbors[ei] == polyB) {
                int vi0 = ei;
                int vi1 = (ei + 1) % 3;
                outMidpoint = (a.vertices[vi0] + a.vertices[vi1]) * 0.5;
                return true;
            }
        }
        return false;
    }

    /** 清空网格 */
    void Clear() { m_polygons.clear(); }

    /** 设置多边形列表 */
    void SetPolygons(std::vector<NavPolygon> polygons) {
        m_polygons = std::move(polygons);
    }

    /** 是否为空 */
    bool IsEmpty() const { return m_polygons.empty(); }

    /** 获取边界范围 */
    void GetBounds(glm::dvec3& outMin, glm::dvec3& outMax) const {
        if (m_polygons.empty()) {
            outMin = outMax = glm::dvec3(0);
            return;
        }
        outMin = glm::dvec3(std::numeric_limits<double>::max());
        outMax = glm::dvec3(-std::numeric_limits<double>::max());

        for (const auto& poly : m_polygons) {
            for (int i = 0; i < 3; ++i) {
                outMin = glm::min(outMin, poly.vertices[i]);
                outMax = glm::max(outMax, poly.vertices[i]);
            }
        }
    }

private:
    std::vector<NavPolygon> m_polygons;
};

} // namespace Navigation
} // namespace Prisma
