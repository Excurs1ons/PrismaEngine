#include "terrain/TerrainCollision.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Prisma::Terrain {

// ============================================================================
// 地表查询
// ============================================================================

float TerrainCollision::GetHeightAt(float x, float z) const
{
    if (!m_HeightMap) return 0.0f;
    return m_HeightMap->GetHeightAt(x, z);
}

Vector3 TerrainCollision::GetNormalAt(float x, float z) const
{
    if (!m_HeightMap) return Vector3(0.0f, 1.0f, 0.0f);
    return m_HeightMap->GetNormalAt(x, z);
}

// ============================================================================
// AABB 碰撞
// ============================================================================

bool TerrainCollision::TestAABB(const Vector3& boxMin, const Vector3& boxMax) const
{
    if (!m_HeightMap) return false;

    // 对 AABB 底部四个角检测地形高度
    // AABB 底部 Y = boxMin.y，如果地形高度 > boxMin.y 则碰撞
    float terrainH;

    // 左下
    terrainH = GetHeightAt(boxMin.x, boxMin.z);
    if (terrainH > boxMin.y) return true;

    // 右下
    terrainH = GetHeightAt(boxMax.x, boxMin.z);
    if (terrainH > boxMin.y) return true;

    // 左上
    terrainH = GetHeightAt(boxMin.x, boxMax.z);
    if (terrainH > boxMin.y) return true;

    // 右上
    terrainH = GetHeightAt(boxMax.x, boxMax.z);
    if (terrainH > boxMin.y) return true;

    // 检测中心
    float centerX = (boxMin.x + boxMax.x) * 0.5f;
    float centerZ = (boxMin.z + boxMax.z) * 0.5f;
    terrainH = GetHeightAt(centerX, centerZ);
    if (terrainH > boxMin.y) return true;

    return false;
}

std::pair<float, Vector3> TerrainCollision::ResolveAABB(
    const Vector3& boxMin, const Vector3& boxMax) const
{
    if (!m_HeightMap) return {0.0f, Vector3(0.0f, 1.0f, 0.0f)};

    // 在多个点采样高度，取最大穿透深度
    float maxPenetration = -std::numeric_limits<float>::infinity();
    Vector3 collisionNormal(0.0f, 1.0f, 0.0f);

    // 采样点集（四个角 + 中心）
    const Vector3 samplePoints[] = {
        Vector3(boxMin.x, 0.0f, boxMin.z),
        Vector3(boxMax.x, 0.0f, boxMin.z),
        Vector3(boxMin.x, 0.0f, boxMax.z),
        Vector3(boxMax.x, 0.0f, boxMax.z),
        Vector3((boxMin.x + boxMax.x) * 0.5f, 0.0f, (boxMin.z + boxMax.z) * 0.5f)
    };

    for (const auto& pt : samplePoints) {
        float terrainH = GetHeightAt(pt.x, pt.z);
        float penetration = terrainH - boxMin.y; // 正数 = 穿透

        if (penetration > maxPenetration) {
            maxPenetration = penetration;
            collisionNormal = GetNormalAt(pt.x, pt.z);
        }
    }

    if (maxPenetration < 0.0f) {
        return {0.0f, Vector3(0.0f, 1.0f, 0.0f)};
    }

    return {maxPenetration, collisionNormal};
}

// ============================================================================
// 射线检测
// ============================================================================

TerrainRaycastHit TerrainCollision::Raycast(const Vector3& origin,
                                             const Vector3& direction,
                                             float maxDistance,
                                             float stepSize) const
{
    TerrainRaycastHit hit;
    if (!m_HeightMap) return hit;

    // 逐步前进算法
    Vector3 currentPos = origin;
    float traveled = 0.0f;

    // 确保方向向量是归一化的
    Vector3 dir = glm::normalize(direction);

    while (traveled < maxDistance) {
        float terrainH = GetHeightAt(currentPos.x, currentPos.z);
        float diff = currentPos.y - terrainH;

        if (diff <= 0.0f) {
            // 射线进入地形内部，使用二分法精确求交
            float low = traveled - stepSize;
            float high = traveled;

            for (int i = 0; i < 10; ++i) { // 10 次二分迭代
                float mid = (low + high) * 0.5f;
                Vector3 midPos = origin + dir * mid;
                float midH = GetHeightAt(midPos.x, midPos.z);

                if (midPos.y <= midH) {
                    high = mid;
                } else {
                    low = mid;
                }
            }

            float finalDist = (low + high) * 0.5f;
            hit.point = origin + dir * finalDist;
            hit.distance = finalDist;
            hit.normal = GetNormalAt(hit.point.x, hit.point.z);
            hit.valid = true;
            return hit;
        }

        currentPos += dir * stepSize;
        traveled += stepSize;
    }

    return hit; // 未命中
}

// ============================================================================
// 工具方法
// ============================================================================

bool TerrainCollision::IsBelowTerrain(const Vector3& point) const
{
    if (!m_HeightMap) return false;
    return point.y <= GetHeightAt(point.x, point.z);
}

Vector3 TerrainCollision::ProjectToSurface(const Vector3& point) const
{
    if (!m_HeightMap) return point;
    float h = GetHeightAt(point.x, point.z);
    return Vector3(point.x, h, point.z);
}

} // namespace Prisma::Terrain
