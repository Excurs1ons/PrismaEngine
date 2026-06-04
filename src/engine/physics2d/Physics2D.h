#pragma once

#include "AABB2D.h"
#include <glm/glm.hpp>
#include <vector>

namespace Prisma::Physics2D {

class Quadtree2D;

/**
 * @brief 2D AABB 碰撞检测结果
 */
struct RaycastHit2D {
    bool hit = false;
    float distance = 0.0f;
    glm::vec2 point{0.0f};
    glm::vec2 normal{0.0f};
};

/**
 * @brief 纯 2D 碰撞检测系统
 *
 * 提供平台游戏所需的碰撞原语：
 * - AABB 重叠检测
 * - 移动 AABB 扫掠检测（SweepAABB）
 * - 平台碰撞解析（ResolvePlatform）
 * - 单向平台检测（CheckOneWayPlatform）
 * - AABB 射线投射（RayCast）
 *
 * 所有方法为静态，无状态，线程安全。
 */
class Physics2D {
public:
    /// @brief AABB 重叠检测
    static bool CheckAABB(const AABB2D& a, const AABB2D& b);

    /**
     * @brief 移动 AABB vs 静态 AABB 扫掠检测
     * @param moving   移动物体的 AABB
     * @param velocity 移动速度向量
     * @param static_  静态障碍物的 AABB
     * @param hitTime  [out] 碰撞时间 (0.0-1.0)
     * @param normal   [out] 碰撞法线
     * @return 是否发生碰撞
     */
    static bool SweepAABB(const AABB2D& moving, glm::vec2 velocity,
                          const AABB2D& static_, float& hitTime, glm::vec2& normal);

    /**
     * @brief 玩家 vs 世界碰撞解析
     * @param player     玩家的 AABB
     * @param velocity   [in/out] 玩家速度（会被碰撞响应修改）
     * @param solids     静态障碍物 AABB 数组
     * @param count      障碍物数量
     * @param onGround   [out] 玩家是否着地
     * @param hitCeiling [out] 玩家是否撞到天花板
     * @return 是否发生任何碰撞
     */
    static bool ResolvePlatform(const AABB2D& player, glm::vec2& velocity,
                                const AABB2D* solids, uint32_t count,
                                bool& onGround, bool& hitCeiling);

    /**
     * @brief 单向平台检测（允许从下方穿过）
     * @param player         玩家的 AABB
     * @param platform       平台的 AABB
     * @param playerPrevBottom 玩家上一帧的底部位置
     * @param playerVelY       玩家垂直速度
     * @return 是否应该与平台碰撞
     */
    static bool CheckOneWayPlatform(const AABB2D& player,
                                    const AABB2D& platform,
                                    float playerPrevBottom,
                                    float playerVelY);

    /**
     * @brief 射线投射 vs 多个 AABB
     * @param origin    射线起点
     * @param direction 射线方向（会被归一化）
     * @param maxDist   最大检测距离
     * @param targets   AABB 数组
     * @param count     AABB 数量
     * @return 最近的击中结果
     */
    static RaycastHit2D RayCast(glm::vec2 origin, glm::vec2 direction,
                                float maxDist, const AABB2D* targets, uint32_t count);

    /**
     * @brief 设置全局四叉树用于空间加速
     *
     * 设置后 ResolvePlatform 将使用四叉树筛选候选固体，
     * 仅对玩家扩展范围内的固体执行扫掠检测。
     * 传入 nullptr 恢复纯数组模式。
     */
    static void SetQuadtree(Quadtree2D* quadtree);

private:
    /// @brief 内部：AABB vs 射线 slab 测试
    static bool RayVsAABB(glm::vec2 origin, glm::vec2 invDir,
                          const AABB2D& box, float& tMin, float& tMax);

    static Quadtree2D* s_quadtree;
};

} // namespace Prisma::Physics2D
