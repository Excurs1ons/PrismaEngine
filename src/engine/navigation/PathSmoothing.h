#pragma once

#include "Export.h"
#include "NavMesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>

namespace Prisma {
namespace Navigation {

/**
 * @brief 路径平滑器
 *
 * 使用漏斗算法（Funnel Algorithm / String Pulling）对 A* 输出的多边形路径
 * 进行直线化处理，去除冗余的边中点航点，得到尽可能直的路径。
 *
 * 算法原理：
 *   沿着多边形路径维护一个漏斗（左边界、右边界），
 *   逐步收紧漏斗，当遇到无法通过的顶点时生成一个航点。
 */
class ENGINE_API PathSmoother {
public:
    struct Config {
        bool enableSmoothing   = true;   // 是否启用平滑
        bool validateWithRaycast = false; // 是否用射线检测验证（需要碰撞数据）
        double raycastStep     = 0.5;    // 射线检测步长
    };

    PathSmoother() = default;
    explicit PathSmoother(Config config) : m_config(config) {}

    /**
     * @brief 执行路径平滑（漏斗算法）
     * @param path A* 寻路结果
     * @param navMesh 导航网格
     * @return 平滑后的路径
     */
    PathResult Smooth(const PathResult& path, const NavMesh& navMesh) const {
        if (!path.found || path.polygonPath.size() < 2) {
            return path;
        }

        if (!m_config.enableSmoothing) {
            return path;
        }

        PathResult result;
        result.found = true;
        result.polygonPath = path.polygonPath;

        // 使用多边形顶点序列执行漏斗算法
        // 将多边形路径转换为边顶点序列（左右边界交替）
        std::vector<glm::dvec3> portalPoints = BuildPortalPoints(path.polygonPath, navMesh);

        if (portalPoints.empty()) {
            // 回退到原路径
            return path;
        }

        // 漏斗算法
        std::vector<glm::dvec3> smoothed = FunnelAlgorithm(portalPoints);

        // 添加起点和终点
        result.waypoints.clear();
        result.waypoints.push_back(path.waypoints.front());

        for (size_t i = 1; i < smoothed.size() - 1; ++i) {
            result.waypoints.push_back(smoothed[i]);
        }

        result.waypoints.push_back(path.waypoints.back());

        // 去重（移除距离过近的航点）
        DeduplicateWaypoints(result.waypoints);

        // 可选：射线检测验证
        if (m_config.validateWithRaycast && result.waypoints.size() > 2) {
            ValidatePathWithRaycast(result.waypoints, navMesh);
        }

        // 重新计算成本
        result.cost = 0.0;
        for (size_t i = 1; i < result.waypoints.size(); ++i) {
            result.cost += glm::distance(result.waypoints[i - 1], result.waypoints[i]);
        }

        return result;
    }

    /**
     * @brief 检查两个点之间是否有直线可见性
     * @param from 起点
     * @param to 终点
     * @param navMesh 导航网格
     * @return 是否可见（直线路径完全在导航网格内）
     */
    static bool IsLineOfSight(const glm::dvec3& from, const glm::dvec3& to,
                              const NavMesh& navMesh) {
        glm::dvec3 dir = to - from;
        double dist = glm::length(dir);
        if (dist < 1e-6) return true;
        dir /= dist;

        // 沿直线采样检测点
        int numSamples = std::max(2, static_cast<int>(dist / 0.5));
        for (int i = 0; i <= numSamples; ++i) {
            double t = static_cast<double>(i) / numSamples;
            glm::dvec3 samplePoint = from + dir * t * dist;

            if (navMesh.FindPolygonContainingPoint(samplePoint) < 0) {
                return false;
            }
        }
        return true;
    }

    void SetConfig(const Config& config) { m_config = config; }
    const Config& GetConfig() const { return m_config; }

private:
    /**
     * @brief 构建端口点序列（左右边界顶点）
     *
     * 对多边形路径中每一对相邻多边形，提取共享边的两个顶点。
     * 左顶点 = 从观察方向看靠左的顶点
     */
    std::vector<glm::dvec3> BuildPortalPoints(
        const std::vector<int32_t>& polyPath,
        const NavMesh& navMesh) const {

        std::vector<glm::dvec3> portals;

        for (size_t i = 0; i + 1 < polyPath.size(); ++i) {
            int32_t polyA = polyPath[i];
            int32_t polyB = polyPath[i + 1];

            const auto& a = navMesh.GetPolygon(polyA);
            const auto& b = navMesh.GetPolygon(polyB);

            // 找到共享边
            glm::dvec3 edgeV0, edgeV1;
            bool found = false;

            for (int ei = 0; ei < 3; ++ei) {
                if (a.neighbors[ei] == polyB) {
                    edgeV0 = a.vertices[ei];
                    edgeV1 = a.vertices[(ei + 1) % 3];
                    found = true;
                    break;
                }
            }

            if (!found) continue;

            // 确定左右顺序：从当前多边形中心到下一个多边形中心的方向
            glm::dvec3 centerA = a.GetCenter();
            glm::dvec3 centerB = b.GetCenter();
            glm::dvec3 forward = glm::normalize(centerB - centerA);
            glm::dvec3 up = a.GetNormal();

            if (glm::length(up) < 1e-10) up = glm::dvec3(0, 1, 0);

            glm::dvec3 edgeDir = edgeV1 - edgeV0;
            glm::dvec3 edgeCross = glm::cross(edgeDir, up);

            // 判断左右：edgeCross 指向 forward 方向的端点为左
            if (glm::dot(edgeCross, forward) > 0) {
                portals.push_back(edgeV0); // 左
                portals.push_back(edgeV1); // 右
            } else {
                portals.push_back(edgeV1); // 左
                portals.push_back(edgeV0); // 右
            }
        }

        return portals;
    }

    /**
     * @brief 漏斗算法核心实现
     * @param portals 端口点序列（L0, R0, L1, R1, ...）
     * @return 直化后的路径点
     */
    std::vector<glm::dvec3> FunnelAlgorithm(
        const std::vector<glm::dvec3>& portals) const {

        if (portals.size() < 4) {
            // 不足以构成漏斗
            std::vector<glm::dvec3> result;
            if (portals.size() >= 2) {
                result.push_back((portals[0] + portals[1]) * 0.5);
            }
            if (portals.size() >= 4) {
                result.push_back((portals[2] + portals[3]) * 0.5);
            }
            return result;
        }

        std::vector<glm::dvec3> result;

        // 初始航点为第一个端口的中心
        glm::dvec3 apex = (portals[0] + portals[1]) * 0.5;
        result.push_back(apex);

        glm::dvec3 portalLeft  = portals[0];
        glm::dvec3 portalRight = portals[1];

        size_t leftIdx  = 0;
        size_t rightIdx = 1;

        for (size_t i = 2; i + 1 < portals.size(); i += 2) {
            glm::dvec3 newLeft  = portals[i];
            glm::dvec3 newRight = portals[i + 1];

            // 检查新左顶点是否在漏斗右侧
            glm::dvec3 rightDir = portalRight - apex;
            glm::dvec3 leftDir  = newLeft - apex;

            // 计算左边界角度
            double crossRight = Cross2D(rightDir, leftDir);
            if (crossRight > 0) {
                // 新左顶点在右侧之外 - 收缩右边
                if (Cross2D(portalRight - apex, LeftPerp(newRight - apex)) < 0) {
                    // 使用新右顶点作为下一个航点
                    apex = portalRight;
                    result.push_back(apex);

                    // 重置漏斗
                    portalLeft  = apex;
                    portalRight = apex;
                    leftIdx  = i;
                    rightIdx = i + 1;

                    // 回退重新处理
                    i = leftIdx;
                    continue;
                }

                portalRight = newLeft;
                rightIdx = i;
            }

            // 检查新右顶点是否在漏斗左侧
            glm::dvec3 leftDir2  = portalLeft - apex;
            glm::dvec3 rightDir2 = newRight - apex;

            double crossLeft = Cross2D(leftDir2, rightDir2);
            if (crossLeft < 0) {
                // 新右顶点在左侧之外 - 收缩左边
                if (Cross2D(LeftPerp(portalLeft - apex), newRight - apex) > 0) {
                    // 使用新左顶点作为下一个航点
                    apex = portalLeft;
                    result.push_back(apex);

                    // 重置漏斗
                    portalLeft  = apex;
                    portalRight = apex;
                    leftIdx  = i;
                    rightIdx = i + 1;

                    // 回退重新处理
                    i = leftIdx;
                    continue;
                }

                portalLeft = newRight;
                leftIdx = i + 1;
            }
        }

        // 最后一个端口的中心作为终点
        glm::dvec3 finalPortalCenter =
            (portals[portals.size() - 2] + portals[portals.size() - 1]) * 0.5;

        // 去重
        if (glm::distance(result.back(), finalPortalCenter) > 0.01) {
            result.push_back(finalPortalCenter);
        }

        return result;
    }

    /** 2D 叉积（在 XZ 平面上） */
    static double Cross2D(const glm::dvec3& a, const glm::dvec3& b) {
        return a.x * b.z - a.z * b.x;
    }

    /** 左垂直向量（在 XZ 平面上） */
    static glm::dvec3 LeftPerp(const glm::dvec3& v) {
        return glm::dvec3(-v.z, 0, v.x);
    }

    /** 去除距离过近的航点 */
    static void DeduplicateWaypoints(std::vector<glm::dvec3>& waypoints) {
        if (waypoints.size() < 2) return;

        std::vector<glm::dvec3> deduped;
        deduped.push_back(waypoints[0]);

        for (size_t i = 1; i < waypoints.size(); ++i) {
            if (glm::distance(waypoints[i], deduped.back()) > 0.01) {
                deduped.push_back(waypoints[i]);
            }
        }

        waypoints = std::move(deduped);
    }

    /** 射线检测验证路径（当碰撞数据可用时） */
    static void ValidatePathWithRaycast(
        std::vector<glm::dvec3>& waypoints,
        const NavMesh& navMesh) {
        if (waypoints.size() < 3) return;

        // 从前往后检查路径可见性，跳过被遮挡的中间点
        std::vector<glm::dvec3> validated;
        validated.push_back(waypoints[0]);

        size_t start = 0;
        for (size_t i = 2; i < waypoints.size(); ++i) {
            if (!IsLineOfSight(waypoints[start], waypoints[i], navMesh)) {
                validated.push_back(waypoints[i - 1]);
                start = i - 1;
            }
        }

        validated.push_back(waypoints.back());
        waypoints = std::move(validated);
    }

    Config m_config;
};

} // namespace Navigation
} // namespace Prisma
