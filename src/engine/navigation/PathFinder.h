#pragma once

#include "Export.h"
#include "NavMesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <functional>

namespace Prisma {
namespace Navigation {

/**
 * @brief A* 寻路结果
 */
struct ENGINE_API PathResult {
    std::vector<glm::dvec3> waypoints;   // 路径航点（世界坐标）
    std::vector<int32_t>     polygonPath; // 经过的多边形索引序列
    bool                     found;       // 是否找到路径
    double                   cost;        // 路径总成本

    PathResult() : found(false), cost(0.0) {}

    bool IsValid() const { return found && !waypoints.empty(); }
};

/**
 * @brief A* 寻路器
 *
 * 在 NavMesh 上使用 A* 算法搜索路径。
 * - Open set 使用二叉堆 (std::push_heap / std::pop_heap)
 * - Closed set 使用 unordered_set
 * - Heuristic 使用 3D 欧几里得距离
 * - 返回路径为多边形边中点的航点序列
 */
class ENGINE_API PathFinder {
public:
    /**
     * @brief 寻路配置
     */
    struct Config {
        double heuristicWeight = 1.0;    // 启发式权重
        double maxSearchNodes  = 10000;   // 最大搜索节点数
        bool   useEdgeMidpoints = true;   // 路径点使用边中点（否则用多边形中心）
    };

    PathFinder() = default;
    explicit PathFinder(Config config) : m_config(config) {}

    /**
     * @brief 在导航网格上寻找路径
     * @param startPos 起点（世界坐标）
     * @param endPos 终点（世界坐标）
     * @param navMesh 导航网格
     * @return 寻路结果
     */
    PathResult FindPath(const glm::dvec3& startPos,
                        const glm::dvec3& endPos,
                        const NavMesh& navMesh) const {
        PathResult result;

        // 找到起点和终点所在的多边形
        int32_t startPoly = navMesh.FindPolygonContainingPoint(startPos);
        int32_t endPoly   = navMesh.FindPolygonContainingPoint(endPos);

        // 如果找不到起点多边形，找最近的
        if (startPoly < 0) {
            glm::dvec3 closest;
            if (!navMesh.FindClosestPolygon(startPos, startPoly, closest)) {
                return result;
            }
        }

        // 如果找不到终点多边形，找最近的
        if (endPoly < 0) {
            glm::dvec3 closest;
            if (!navMesh.FindClosestPolygon(endPos, endPoly, closest)) {
                return result;
            }
        }

        // 起点=终点在同一多边形
        if (startPoly == endPoly) {
            result.found = true;
            result.waypoints.push_back(startPos);
            result.waypoints.push_back(endPos);
            result.polygonPath.push_back(startPoly);
            result.cost = glm::distance(startPos, endPos);
            return result;
        }

        // A* 算法
        size_t polyCount = navMesh.GetPolygonCount();

        // g 值（起点到该多边形的实际成本）
        std::vector<double> gScores(polyCount, std::numeric_limits<double>::max());
        // f 值（g + 启发式）
        std::vector<double> fScores(polyCount, std::numeric_limits<double>::max());
        // 父节点
        std::vector<int32_t> parents(polyCount, -1);

        gScores[startPoly] = 0.0;
        fScores[startPoly] = Heuristic(navMesh.GetPolygon(startPoly).GetCenter(),
                                       navMesh.GetPolygon(endPoly).GetCenter());

        // Open set 使用二叉堆（存储多边形索引）
        // 比较器：f 值小的优先
        auto cmp = [&](int32_t a, int32_t b) {
            return fScores[a] > fScores[b];
        };
        std::vector<int32_t> openSet;
        openSet.push_back(startPoly);
        std::push_heap(openSet.begin(), openSet.end(), cmp);

        // Closed set
        std::unordered_set<int32_t> closedSet;

        size_t nodesVisited = 0;
        bool found = false;

        while (!openSet.empty() && nodesVisited < m_config.maxSearchNodes) {
            // 弹出 f 值最小的节点
            std::pop_heap(openSet.begin(), openSet.end(), cmp);
            int32_t current = openSet.back();
            openSet.pop_back();

            if (current == endPoly) {
                found = true;
                break;
            }

            if (closedSet.count(current) > 0) continue;
            closedSet.insert(current);
            ++nodesVisited;

            const auto& currentPoly = navMesh.GetPolygon(current);
            glm::dvec3 currentCenter = currentPoly.GetCenter();

            // 遍历邻居
            for (int i = 0; i < 3; ++i) {
                int32_t neighbor = currentPoly.neighbors[i];
                if (neighbor < 0) continue;
                if (closedSet.count(neighbor) > 0) continue;

                const auto& neighborPoly = navMesh.GetPolygon(neighbor);
                glm::dvec3 neighborCenter = neighborPoly.GetCenter();

                // 计算从起点经过 current 到 neighbor 的成本
                double edgeCost = glm::distance(currentCenter, neighborCenter);
                double tentativeG = gScores[current] + edgeCost;

                if (tentativeG >= gScores[neighbor]) continue;

                // 更新路径
                parents[neighbor] = current;
                gScores[neighbor] = tentativeG;
                fScores[neighbor] = tentativeG
                    + Heuristic(neighborCenter,
                                navMesh.GetPolygon(endPoly).GetCenter())
                    * m_config.heuristicWeight;

                // 加入 open set
                openSet.push_back(neighbor);
                std::push_heap(openSet.begin(), openSet.end(), cmp);
            }
        }

        if (!found) return result;

        // 回溯路径
        result.found = true;

        // 多边形序列
        std::vector<int32_t> polyPath;
        int32_t current = endPoly;
        while (current >= 0) {
            polyPath.push_back(current);
            current = parents[current];
        }
        std::reverse(polyPath.begin(), polyPath.end());
        result.polygonPath = polyPath;

        // 生成航点
        if (m_config.useEdgeMidpoints && polyPath.size() > 1) {
            // 使用边中点
            result.waypoints.push_back(startPos); // 起点

            for (size_t i = 0; i < polyPath.size() - 1; ++i) {
                glm::dvec3 midpoint;
                if (navMesh.GetEdgeMidpoint(polyPath[i], polyPath[i + 1], midpoint)) {
                    result.waypoints.push_back(midpoint);
                } else {
                    // 回退到多边形中心
                    result.waypoints.push_back(
                        navMesh.GetPolygon(polyPath[i]).GetCenter());
                }
            }

            result.waypoints.push_back(endPos);   // 终点
        } else {
            // 使用多边形中心
            result.waypoints.push_back(startPos);
            for (size_t i = 1; i < polyPath.size() - 1; ++i) {
                result.waypoints.push_back(
                    navMesh.GetPolygon(polyPath[i]).GetCenter());
            }
            if (polyPath.size() > 1) {
                result.waypoints.push_back(endPos);
            }
        }

        // 计算总成本
        result.cost = gScores[endPoly];

        return result;
    }

    /** 设置寻路配置 */
    void SetConfig(const Config& config) { m_config = config; }
    const Config& GetConfig() const { return m_config; }

private:
    /**
     * @brief 3D 欧几里得距离启发式
     */
    static double Heuristic(const glm::dvec3& a, const glm::dvec3& b) {
        return glm::distance(a, b);
    }

    Config m_config;
};

} // namespace Navigation
} // namespace Prisma
