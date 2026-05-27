#pragma once

#include "Export.h"
#include "NavMesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <sstream>

namespace Prisma {
namespace Navigation {

/**
 * @brief 导航网格构建器
 *
 * 支持从三角形网格（顶点+索引）构建 NavMesh。
 * 核心流程：
 *   1. 去重接收顶点和索引数据
 *   2. 构建每个三角形（NavPolygon），计算邻居关系
 *   3. 根据高度/坡度过滤可走区域
 *   4. 输出 NavMesh
 *
 * 当前实现专注于从预生成的地形/静态碰撞网格构建。
 * 未来可扩展为从高度场 → 体素化 → 可走区域 → 三角化的完整管线。
 */
class ENGINE_API NavMeshBuilder {
public:
    struct BuildConfig {
        double maxWalkableSlope = 45.0;   // 最大可走坡度（度）
        double maxStepHeight    = 0.5;    // 最大可跨越台阶高度
        double agentRadius      = 0.3f;   // 智能体半径（用于边缘侵蚀）
        double walkableHeight   = 1.8;    // 可走区域最小高度
        bool   filterBySlope    = true;   // 是否按坡度过滤
        bool   buildNeighbors   = true;   // 是否构建邻居关系

        // OBJ 加载配置
        bool   flipYZ           = true;   // 是否翻转 Y/Z 轴
    };

    NavMeshBuilder() = default;

    /**
     * @brief 从三角形网格构建 NavMesh
     * @param vertices 顶点数组（每 3 个 double = 一个顶点）
     * @param vertexCount 顶点数量
     * @param indices 索引数组（每 3 个 uint32_t = 一个三角形）
     * @param indexCount 索引数量
     * @param config 构建配置
     * @return 构建的 NavMesh
     */
    NavMesh Build(const double* vertices, size_t vertexCount,
                  const uint32_t* indices, size_t indexCount,
                  const BuildConfig& config = BuildConfig()) const {
        std::vector<NavPolygon> polygons;
        size_t triangleCount = indexCount / 3;

        if (triangleCount == 0) return NavMesh{};

        polygons.reserve(triangleCount);

        // 阶段 1: 创建多边形
        for (size_t t = 0; t < triangleCount; ++t) {
            NavPolygon poly;
            for (int v = 0; v < 3; ++v) {
                uint32_t idx = indices[t * 3 + v];
                if (idx < vertexCount) {
                    poly.vertices[v] = glm::dvec3(
                        vertices[idx * 3 + 0],
                        vertices[idx * 3 + 1],
                        vertices[idx * 3 + 2]
                    );
                }
            }
            poly.id = static_cast<uint32_t>(t);
            polygons.push_back(poly);
        }

        // 阶段 2: 可选坡度过滤
        if (config.filterBySlope) {
            FilterBySlope(polygons, config.maxWalkableSlope);
        }

        // 阶段 3: 构建邻居关系
        if (config.buildNeighbors) {
            BuildNeighborRelations(polygons);
        }

        return NavMesh(std::move(polygons));
    }

    /**
     * @brief 从顶点和索引向量构建 NavMesh
     */
    NavMesh Build(const std::vector<double>& vertices,
                  const std::vector<uint32_t>& indices,
                  const BuildConfig& config = BuildConfig()) const {
        return Build(vertices.data(), vertices.size() / 3,
                     indices.data(), indices.size(), config);
    }

    /**
     * @brief 从 glm::dvec3 顶点列表和索引构建 NavMesh
     */
    NavMesh Build(const std::vector<glm::dvec3>& vertices,
                  const std::vector<uint32_t>& indices,
                  const BuildConfig& config = BuildConfig()) const {
        std::vector<double> flatVerts;
        flatVerts.reserve(vertices.size() * 3);
        for (const auto& v : vertices) {
            flatVerts.push_back(v.x);
            flatVerts.push_back(v.y);
            flatVerts.push_back(v.z);
        }
        return Build(flatVerts, indices, config);
    }

    /**
     * @brief 从 .obj 格式字符串构建 NavMesh
     *
     * 解析简单的 OBJ 文件（仅支持顶点 v 和面 f）。
     * 仅支持三角形面。
     *
     * @param objData OBJ 文件内容
     * @param config 构建配置
     * @return 构建的 NavMesh
     */
    NavMesh BuildFromOBJ(const std::string& objData,
                         const BuildConfig& config = BuildConfig()) const {
        std::vector<double> vertices;
        std::vector<uint32_t> indices;

        std::istringstream stream(objData);
        std::string line;

        while (std::getline(stream, line)) {
            // 跳过空行和注释
            if (line.empty() || line[0] == '#') continue;

            if (line.size() >= 2 && line[0] == 'v' && line[1] == ' ') {
                // 顶点: v x y z
                double x, y, z;
                if (sscanf(line.c_str(), "v %lf %lf %lf", &x, &y, &z) == 3) {
                    if (config.flipYZ) {
                        vertices.push_back(x);   // X
                        vertices.push_back(z);   // Y ← flipped from Z
                        vertices.push_back(y);   // Z ← flipped from Y
                    } else {
                        vertices.push_back(x);
                        vertices.push_back(y);
                        vertices.push_back(z);
                    }
                }
            } else if (line.size() >= 2 && line[0] == 'f' && line[1] == ' ') {
                // 面: f v1 v2 v3 (仅支持三角形)
                int a, b, c;
                if (sscanf(line.c_str(), "f %d %d %d", &a, &b, &c) == 3) {
                    // OBJ 索引从 1 开始
                    indices.push_back(static_cast<uint32_t>(a - 1));
                    indices.push_back(static_cast<uint32_t>(b - 1));
                    indices.push_back(static_cast<uint32_t>(c - 1));
                }
            }
        }

        return Build(vertices, indices, config);
    }

    /**
     * @brief 合并两个 NavMesh
     * @param a 第一个网格
     * @param b 第二个网格
     * @return 合并后的网格
     */
    static NavMesh Merge(const NavMesh& a, const NavMesh& b) {
        std::vector<NavPolygon> polygons;
        polygons.reserve(a.GetPolygonCount() + b.GetPolygonCount());

        uint32_t offset = static_cast<uint32_t>(a.GetPolygonCount());

        // 复制第一个网格，保留原 ID
        for (size_t i = 0; i < a.GetPolygonCount(); ++i) {
            auto poly = a.GetPolygon(i);
            polygons.push_back(poly);
        }

        // 复制第二个网格，调整 ID 和邻居索引
        for (size_t i = 0; i < b.GetPolygonCount(); ++i) {
            auto poly = b.GetPolygon(i);
            poly.id += offset;
            for (int n = 0; n < 3; ++n) {
                if (poly.neighbors[n] >= 0) {
                    poly.neighbors[n] += offset;
                }
            }
            polygons.push_back(poly);
        }

        return NavMesh(std::move(polygons));
    }

private:
    /** 按坡度过滤不可走三角形 */
    static void FilterBySlope(std::vector<NavPolygon>& polygons,
                              double maxSlopeDegrees) {
        double maxSlopeRad = glm::radians(maxSlopeDegrees);
        double cosMaxSlope = std::cos(maxSlopeRad);
        glm::dvec3 up(0, 1, 0);

        for (auto& poly : polygons) {
            glm::dvec3 normal = poly.GetNormal();
            double cosAngle = glm::abs(glm::dot(normal, up));

            if (cosAngle < cosMaxSlope) {
                poly.areaType = AreaType::Obstacle;
            }
        }
    }

    /** 构建邻居关系（共享边） */
    static void BuildNeighborRelations(std::vector<NavPolygon>& polygons) {
        size_t count = polygons.size();
        if (count == 0) return;

        // 使用哈希映射边 → 多边形索引
        // 边键值: (minVertexIdx, maxVertexIdx) 按顶点位置哈希
        struct EdgeKey {
            glm::dvec3 v0, v1;

            bool operator==(const EdgeKey& other) const {
                return glm::all(glm::equal(v0, other.v0))
                    && glm::all(glm::equal(v1, other.v1));
            }
        };

        struct EdgeKeyHash {
            size_t operator()(const EdgeKey& key) const {
                size_t h = 0;
                auto hashDouble = [](double d) -> size_t {
                    int64_t bits;
                    std::memcpy(&bits, &d, sizeof(bits));
                    return static_cast<size_t>(bits);
                };
                h ^= hashDouble(key.v0.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= hashDouble(key.v0.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= hashDouble(key.v0.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= hashDouble(key.v1.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= hashDouble(key.v1.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
                h ^= hashDouble(key.v1.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
                return h;
            }
        };

        std::unordered_map<EdgeKey, int32_t, EdgeKeyHash> edgeMap;

        auto makeEdgeKey = [](const glm::dvec3& a, const glm::dvec3& b) -> EdgeKey {
            // 规范化边：确保一致的排序
            if (a.x < b.x || (a.x == b.x && (a.y < b.y || (a.y == b.y && a.z < b.z)))) {
                return {a, b};
            }
            return {b, a};
        };

        // 第一遍：记录每条边所属的多边形
        for (size_t i = 0; i < count; ++i) {
            auto& poly = polygons[i];
            for (int e = 0; e < 3; ++e) {
                int vi0 = e;
                int vi1 = (e + 1) % 3;
                EdgeKey key = makeEdgeKey(poly.vertices[vi0], poly.vertices[vi1]);

                auto it = edgeMap.find(key);
                if (it == edgeMap.end()) {
                    edgeMap[key] = static_cast<int32_t>(i);
                } else if (it->second != static_cast<int32_t>(i)) {
                    // 找到共享边的多边形
                    int32_t neighborIdx = it->second;
                    poly.neighbors[e] = neighborIdx;

                    // 也在邻居中设置对应边
                    auto& neighbor = polygons[neighborIdx];
                    for (int ne = 0; ne < 3; ++ne) {
                        int nvi0 = ne;
                        int nvi1 = (ne + 1) % 3;
                        EdgeKey nk = makeEdgeKey(neighbor.vertices[nvi0], neighbor.vertices[nvi1]);
                        // 检查两条边是否匹配（方向相反）
                        glm::dvec3 edgeDir = poly.vertices[vi1] - poly.vertices[vi0];
                        glm::dvec3 nEdgeDir = neighbor.vertices[nvi1] - neighbor.vertices[nvi0];
                        if (glm::length2(edgeDir + nEdgeDir) < 1e-10) {
                            neighbor.neighbors[ne] = static_cast<int32_t>(i);
                            break;
                        }
                    }
                }
            }
        }
    }
};

} // namespace Navigation
} // namespace Prisma
