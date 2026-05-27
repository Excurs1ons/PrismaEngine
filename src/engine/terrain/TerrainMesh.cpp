#include "terrain/TerrainMesh.h"
#include "Logger.h"
#include <cmath>
#include <algorithm>

namespace Prisma::Terrain {

// ============================================================================
// 网格生成
// ============================================================================

void TerrainMesh::Generate(const HeightMap& heightMap, const Vector3& cameraCenter,
                            const TerrainConfig& config)
{
    if (!heightMap.IsValid()) {
        LOG_WARNING("Terrain", "跳过网格生成：高度图无效");
        return;
    }

    // 检测相机是否移动足够大
    float distSq = glm::distance2(cameraCenter, m_LastCameraPosition);
    if (distSq < k_RegenThreshold * k_RegenThreshold && !m_LODLevels.empty()) {
        return; // 相机未显著移动，跳过重新生成
    }

    ForceGenerate(heightMap, cameraCenter, config);
}

void TerrainMesh::ForceGenerate(const HeightMap& heightMap,
                                 const Vector3& cameraCenter,
                                 const TerrainConfig& config)
{
    m_LODLevels.clear();
    m_LastCameraPosition = cameraCenter;

    size_t lodCount = config.lodDistances.size();
    m_LODLevels.reserve(lodCount);

    for (size_t i = 0; i < lodCount; ++i) {
        uint32_t vertsPerSide = (i < config.lodVertexCounts.size())
            ? config.lodVertexCounts[i] : 5;
        float radius = (i < config.lodRadii.size())
            ? config.lodRadii[i] : 100.0f;
        float minDist = config.lodDistances[i];

        // 每个 LOD 的步长 = 1 << i（LOD 0 = 步长 1, LOD 1 = 步长 2, ...）
        uint32_t step = static_cast<uint32_t>(1) << i;

        // 计算最大距离：当前 LOD 到下一个 LOD 的距离，或无限远
        float maxDist = (i + 1 < lodCount)
            ? config.lodDistances[i + 1]
            : std::max(config.lodDistances.back() * 2.0f, 2000.0f);

        // 如果此 LOD 的距离超出最远距离，跳过
        if (minDist > config.lodDistances.back() + radius) {
            continue;
        }

        TerrainLODLevel level = GenerateLODLevel(
            heightMap, cameraCenter,
            static_cast<uint32_t>(i),
            vertsPerSide, step, radius,
            minDist, maxDist
        );

        if (!level.IsEmpty()) {
            m_LODLevels.push_back(std::move(level));
        }
    }

    if (!m_LODLevels.empty()) {
        m_ActiveLODStart = 0;
        m_ActiveLODEnd = static_cast<uint32_t>(m_LODLevels.size());

        LOG_INFO("Terrain", "已生成地形网格: {} LOD 等级, 相机位置 ({:.1f}, {:.1f}, {:.1f})",
                 m_LODLevels.size(), cameraCenter.x, cameraCenter.y, cameraCenter.z);

        // 打印每个 LOD 等级的统计
        for (size_t i = 0; i < m_LODLevels.size(); ++i) {
            const auto& lvl = m_LODLevels[i];
            LOG_TRACE("Terrain", "  LOD {}: {} 顶点, {} 三角形, 范围 [{:.0f}, {:.0f}]",
                      i, lvl.GetVertexCount(), lvl.GetTriangleCount(),
                      lvl.minDistance, lvl.maxDistance);
        }
    } else {
        LOG_WARNING("Terrain", "地形网格生成失败：没有 LOD 等级生成");
    }
}

TerrainLODLevel TerrainMesh::GenerateLODLevel(const HeightMap& heightMap,
                                               const Vector3& cameraCenter,
                                               uint32_t lodIndex,
                                               uint32_t vertsPerSide,
                                               uint32_t step,
                                               float radius,
                                               float minDist,
                                               float maxDist)
{
    TerrainLODLevel level;
    level.vertsPerSide = vertsPerSide;
    level.step = step;
    level.lodRadius = radius;
    level.minDistance = minDist;
    level.maxDistance = maxDist;
    level.center = cameraCenter;

    // 顶点网格尺寸
    // 每个 LOD 等级生成一个以相机为中心的网格
    // 网格覆盖半径 'radius' 的范围，顶点间距由 step 决定
    float halfSize = radius;
    float spacing = (radius * 2.0f) / static_cast<float>(vertsPerSide - 1);

    // 预分配
    size_t vertexCount = static_cast<size_t>(vertsPerSide) * vertsPerSide;
    size_t indexCount = static_cast<size_t>(vertsPerSide - 1) * (vertsPerSide - 1) * 6;

    level.vertices.reserve(vertexCount);
    level.indices.reserve(indexCount);

    // 初始化 AABB
    Vector3 aabbMin(std::numeric_limits<float>::max());
    Vector3 aabbMax(std::numeric_limits<float>::lowest());

    // 生成顶点
    for (uint32_t z = 0; z < vertsPerSide; ++z) {
        for (uint32_t x = 0; x < vertsPerSide; ++x) {
            // 计算世界坐标
            float wx = cameraCenter.x - halfSize + static_cast<float>(x) * spacing;
            float wz = cameraCenter.z - halfSize + static_cast<float>(z) * spacing;

            // 采样高度
            float wy = heightMap.GetHeightAt(wx, wz);

            // 位置
            Vector3 position(wx, wy, wz);

            // 法线（从高度图采样）
            Vector3 normal = heightMap.GetNormalAt(wx, wz);

            // UV（基于世界坐标平铺）
            float tileScale = 32.0f;
            Vector2 uv(wx / tileScale, wz / tileScale);

            level.vertices.push_back({position, normal, uv});

            // 更新 AABB
            aabbMin = glm::min(aabbMin, position);
            aabbMax = glm::max(aabbMax, position);
        }
    }

    // 生成索引（三角形网格）
    for (uint32_t z = 0; z < vertsPerSide - 1; ++z) {
        for (uint32_t x = 0; x < vertsPerSide - 1; ++x) {
            uint32_t i0 = z * vertsPerSide + x;
            uint32_t i1 = z * vertsPerSide + (x + 1);
            uint32_t i2 = (z + 1) * vertsPerSide + x;
            uint32_t i3 = (z + 1) * vertsPerSide + (x + 1);

            // 两个三角形形成一个四边形
            // 三角形 1: (i0, i1, i2)
            level.indices.push_back(i0);
            level.indices.push_back(i1);
            level.indices.push_back(i2);

            // 三角形 2: (i1, i3, i2)
            level.indices.push_back(i1);
            level.indices.push_back(i3);
            level.indices.push_back(i2);
        }
    }

    // 存储 AABB
    level.aabbMin = aabbMin;
    level.aabbMax = aabbMax;

    return level;
}

void TerrainMesh::Clear()
{
    m_LODLevels.clear();
    m_ActiveLODStart = 0;
    m_ActiveLODEnd = 0;
}

// ============================================================================
// 视锥剔除
// ============================================================================

std::vector<bool> TerrainMesh::CullAgainstFrustum(
    const Graphic::Frustum& frustum) const
{
    std::vector<bool> visibility;
    visibility.reserve(m_LODLevels.size());

    for (const auto& level : m_LODLevels) {
        // Frustum 使用 double 精度，转换 AABB
        glm::dvec3 minAABB(level.aabbMin.x, level.aabbMin.y, level.aabbMin.z);
        glm::dvec3 maxAABB(level.aabbMax.x, level.aabbMax.y, level.aabbMax.z);

        bool visible = frustum.isVisible(minAABB, maxAABB);
        visibility.push_back(visible);
    }

    return visibility;
}

const TerrainLODLevel* TerrainMesh::GetLODLevel(uint32_t index) const
{
    if (index >= m_LODLevels.size()) return nullptr;
    return &m_LODLevels[index];
}

} // namespace Prisma::Terrain
