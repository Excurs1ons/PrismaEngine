#pragma once

#include "math/MathTypes.h"
#include "HeightMap.h"
#include "TerrainConfig.h"
#include "graphic/FrustumCulling.h"
#include "Export.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace Prisma::Terrain {

// ========== 地形顶点结构 ==========

struct TerrainVertex {
    Vector3 position;  // 顶点位置
    Vector3 normal;    // 顶点法线
    Vector2 uv;        // 纹理 UV
};

// ========== LOD 等级网格 ==========

struct TerrainLODLevel {
    std::vector<TerrainVertex> vertices;  // 顶点数据
    std::vector<uint32_t> indices;         // 索引数据
    uint32_t vertsPerSide = 0;            // 每边顶点数
    uint32_t step = 1;                    // 顶点步长
    float lodRadius = 0.0f;              // 此 LOD 覆盖半径
    float minDistance = 0.0f;            // 距离范围下限
    float maxDistance = 0.0f;            // 距离范围上限
    Vector3 center;                       // 网格中心（相机位置）

    // 包围盒（用于视锥剔除）
    Vector3 aabbMin = Vector3(0.0f);
    Vector3 aabbMax = Vector3(0.0f);

    bool IsEmpty() const { return vertices.empty() || indices.empty(); }
    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetIndexCount() const { return indices.size(); }
    size_t GetTriangleCount() const { return indices.size() / 3; }
};

/**
 * @brief 地形网格 - 基于 clipmap 的 LOD 地形网格生成
 *
 * 使用 clipmap 方式实现多层次细节（LOD）：
 * - LOD 0（最高细节）：小范围、高分辨率，最近距离
 * - LOD N（最低细节）：大范围、低分辨率，最远距离
 * - 每次相机移动超过阈值时重新生成网格
 *
 * 支持视锥体裁剪：每个 LOD 等级单独检测可见性。
 */
class TerrainMesh {
public:
    TerrainMesh() = default;
    ~TerrainMesh() = default;

    // ========== 网格生成 ==========

    /**
     * @brief 生成地形网格
     * @param heightMap 高度图引用
     * @param cameraCenter 相机世界位置
     * @param config 地形配置
     *
     * 生成多个 LOD 等级的同心网格。
     * 如果相机移动距离小于阈值，跳过重新生成。
     */
    void Generate(const HeightMap& heightMap, const Vector3& cameraCenter,
                  const TerrainConfig& config);

    /**
     * @brief 强制重新生成（忽略相机移动阈值）
     */
    void ForceGenerate(const HeightMap& heightMap, const Vector3& cameraCenter,
                       const TerrainConfig& config);

    /**
     * @brief 生成单个 LOD 等级的网格
     * @param heightMap 高度图
     * @param cameraCenter 相机中心
     * @param lodIndex LOD 等级索引
     * @param vertsPerSide 每边顶点数
     * @param step 顶点步长
     * @param radius LOD 半径
     * @param minDist 最小距离
     * @param maxDist 最大距离
     */
    TerrainLODLevel GenerateLODLevel(const HeightMap& heightMap,
                                     const Vector3& cameraCenter,
                                     uint32_t lodIndex,
                                     uint32_t vertsPerSide,
                                     uint32_t step,
                                     float radius,
                                     float minDist,
                                     float maxDist);

    /**
     * @brief 清空所有 LOD 数据
     */
    void Clear();

    // ========== 视锥剔除 ==========

    /**
     * @brief 对视锥体执行剔除
     * @param frustum 视锥体
     * @return 每个 LOD 等级的可见性（true=可见）
     *
     * 返回与 LOD 等级数量相同的布尔数组。
     * 不可见的 LOD 等级将跳过渲染。
     */
    std::vector<bool> CullAgainstFrustum(const Graphic::Frustum& frustum) const;

    // ========== 属性 ==========

    const std::vector<TerrainLODLevel>& GetLODLevels() const { return m_LODLevels; }
    const TerrainLODLevel* GetLODLevel(uint32_t index) const;
    size_t GetLODCount() const { return m_LODLevels.size(); }
    bool IsValid() const { return !m_LODLevels.empty(); }

    // LOD 等级索引范围
    uint32_t GetActiveLODStart() const { return m_ActiveLODStart; }
    uint32_t GetActiveLODEnd() const { return m_ActiveLODEnd; }
    uint32_t GetActiveLODCount() const {
        return m_ActiveLODEnd - m_ActiveLODStart;
    }

    // 上次生成的相机位置
    Vector3 GetLastCameraPosition() const { return m_LastCameraPosition; }

private:
    std::vector<TerrainLODLevel> m_LODLevels;
    Vector3 m_LastCameraPosition = Vector3(0.0f);
    uint32_t m_ActiveLODStart = 0;
    uint32_t m_ActiveLODEnd = 0;

    // 重新生成阈值（相机移动超过此距离才重新生成）
    static constexpr float k_RegenThreshold = 2.0f;
};

} // namespace Prisma::Terrain
