#pragma once

#include "math/MathTypes.h"
#include "HeightMap.h"
#include <optional>
#include <utility>

namespace Prisma::Terrain {

/**
 * @brief 射线与地形交点
 */
struct TerrainRaycastHit {
    Vector3 point;       // 交点世界坐标
    Vector3 normal;      // 交点法线
    float distance;      // 从射线起点到交点的距离
    bool valid = false;  // 是否有效命中
};

/**
 * @brief 地形碰撞检测 - 提供基于高度图的地形碰撞查询
 *
 * 功能：
 * - getHeightAt/getNormalAt：地形表面查询
 * - AABB vs 地形碰撞测试
 * - 射线与地形求交
 *
 * 注意：高度图地形没有悬垂/洞穴（仅地表碰撞）。
 */
class TerrainCollision {
public:
    TerrainCollision() = default;
    explicit TerrainCollision(const HeightMap* heightMap)
        : m_HeightMap(heightMap) {}

    ~TerrainCollision() = default;

    // ========== 高度图引用 ==========

    void SetHeightMap(const HeightMap* heightMap) { m_HeightMap = heightMap; }
    const HeightMap* GetHeightMap() const { return m_HeightMap; }

    // ========== 地表查询 ==========

    /**
     * @brief 获取地形在世界坐标 (x, z) 处的高度
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 地形高度（如果高度图无效则返回 0）
     */
    float GetHeightAt(float x, float z) const;

    /**
     * @brief 获取地形在世界坐标 (x, z) 处的法线
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 归一化法线
     */
    Vector3 GetNormalAt(float x, float z) const;

    // ========== AABB 碰撞 ==========

    /**
     * @brief 检测 AABB 是否与地形相交
     * @param boxMin AABB 最小值
     * @param boxMax AABB 最大值
     * @return 是否相交
     *
     * 检测 AABB 底部的任何角点是否低于地形高度。
     */
    bool TestAABB(const Vector3& boxMin, const Vector3& boxMax) const;

    /**
     * @brief 获取 AABB 与地形碰撞的穿透深度和法线
     * @param boxMin AABB 最小值
     * @param boxMax AABB 最大值
     * @return (穿透深度, 碰撞法线)，如果不碰撞则穿透深度 <= 0
     *
     * 穿透深度 = 地形高度 - AABB 底部
     * 碰撞法线 = 地形法线
     */
    std::pair<float, Vector3> ResolveAABB(const Vector3& boxMin,
                                           const Vector3& boxMax) const;

    // ========== 射线检测 ==========

    /**
     * @brief 在高度图地形上执行射线检测
     * @param origin 射线起点
     * @param direction 射线方向（应归一化）
     * @param maxDistance 最大检测距离
     * @param stepSize 步长（越小越精确，但越慢）
     * @return 射线检测结果
     *
     * 使用逐步前进算法（步进法）沿射线方向采样高度图。
     * 当射线位置低于地形表面时，使用二分法精确求交。
     */
    TerrainRaycastHit Raycast(const Vector3& origin, const Vector3& direction,
                              float maxDistance = 1000.0f,
                              float stepSize = 1.0f) const;

    // ========== 工具方法 ==========

    /**
     * @brief 判断点是否在地形表面下方
     */
    bool IsBelowTerrain(const Vector3& point) const;

    /**
     * @brief 将点提升到地形表面
     * @param point 要提升的点
     * @return 地形表面上的点（保持 x, z 不变，y 为地形高度）
     */
    Vector3 ProjectToSurface(const Vector3& point) const;

private:
    const HeightMap* m_HeightMap = nullptr;
};

} // namespace Prisma::Terrain
