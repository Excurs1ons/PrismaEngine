#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <array>
#include <functional>
#include <algorithm>
#include <limits>
#include <cmath>

namespace PrismaEngine {
    namespace Physics {

        /**
         * @brief 轴对齐包围盒 (AABB) - Axis-Aligned Bounding Box
         *
         * 用于碰撞检测的基础数据结构
         * 对应 Minecraft: net.minecraft.world.phys.AABB
         */
        class AABB {
        public:
            double minX, minY, minZ;
            double maxX, maxY, maxZ;

            // ========== 构造函数 ==========

            constexpr AABB() noexcept
                : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {}

            constexpr AABB(double x1, double y1, double z1, double x2, double y2, double z2) noexcept
                : minX(x1), minY(y1), minZ(z1), maxX(x2), maxY(y2), maxZ(z2) {}

            /**
             * @brief 从中心点和尺寸创建 AABB
             */
            static AABB fromCenterSize(double cx, double cy, double cz, double sx, double sy, double sz) {
                return AABB(
                    cx - sx * 0.5, cy - sy * 0.5, cz - sz * 0.5,
                    cx + sx * 0.5, cy + sy * 0.5, cz + sz * 0.5
                );
            }

            /**
             * @brief 创建方块大小的 AABB
             */
            static constexpr AABB ofSize(double x, double y, double z, double size) noexcept {
                return AABB(x, y, z, x + size, y + size, z + size);
            }

            // ========== 碰撞检测 ==========

            /**
             * @brief 检查是否与另一个 AABB 相交
             */
            constexpr bool intersects(const AABB& other) const noexcept {
                return minX < other.maxX && maxX > other.minX &&
                       minY < other.maxY && maxY > other.minY &&
                       minZ < other.maxZ && maxZ > other.minZ;
            }

            /**
             * @brief 检查是否包含点
             */
            constexpr bool contains(double x, double y, double z) const noexcept {
                return x >= minX && x <= maxX &&
                       y >= minY && y <= maxY &&
                       z >= minZ && z <= maxZ;
            }

            constexpr bool contains(const glm::dvec3& point) const noexcept {
                return contains(point.x, point.y, point.z);
            }

            /**
             * @brief 检查点是否在 AABB 内部（不包括边界）
             */
            constexpr bool isInside(double x, double y, double z) const noexcept {
                return x > minX && x < maxX &&
                       y > minY && y < maxY &&
                       z > minZ && z < maxZ;
            }

            // ========== AABB 操作 ==========

            /**
             * @brief 获取两个 AABB 的交集
             */
            AABB intersection(const AABB& other) const noexcept {
                return AABB(
                    std::max(minX, other.minX),
                    std::max(minY, other.minY),
                    std::max(minZ, other.minZ),
                    std::min(maxX, other.maxX),
                    std::min(maxY, other.maxY),
                    std::min(maxZ, other.maxZ)
                );
            }

            /**
             * @brief 移动 AABB
             */
            AABB move(double dx, double dy, double dz) const noexcept {
                return AABB(minX + dx, minY + dy, minZ + dz,
                           maxX + dx, maxY + dy, maxZ + dz);
            }

            AABB move(const glm::dvec3& delta) const noexcept {
                return move(delta.x, delta.y, delta.z);
            }

            /**
             * @brief 缩小 AABB
             */
            AABB shrink(double x, double y, double z) const noexcept {
                return AABB(minX + x, minY + y, minZ + z,
                           maxX - x, maxY - y, maxZ - z);
            }

            /**
             * @brief 扩展 AABB
             */
            AABB expand(double x, double y, double z) const noexcept {
                return AABB(minX - x, minY - y, minZ - z,
                           maxX + x, maxY + y, maxZ + z);
            }

            /**
             * @brief 合并两个 AABB（创建包含两者的最小 AABB）
             */
            AABB unionAABB(const AABB& other) const noexcept {
                return AABB(
                    std::min(minX, other.minX),
                    std::min(minY, other.minY),
                    std::min(minZ, other.minZ),
                    std::max(maxX, other.maxX),
                    std::max(maxY, other.maxY),
                    std::max(maxZ, other.maxZ)
                );
            }

            // ========== 属性查询 ==========

            /**
             * @brief 获取 X 大小
             */
            constexpr double getXsize() const noexcept { return maxX - minX; }

            /**
             * @brief 获取 Y 大小
             */
            constexpr double getYsize() const noexcept { return maxY - minY; }

            /**
             * @brief 获取 Z 大小
             */
            constexpr double getZsize() const noexcept { return maxZ - minZ; }

            /**
             * @brief 获取平均边长（用于碰撞检测误差）
             */
            constexpr double getAverageEdgeLength() const noexcept {
                return (getXsize() + getYsize() + getZsize()) / 3.0;
            }

            /**
             * @brief 获取中心点
             */
            constexpr glm::dvec3 getCenter() const noexcept {
                return glm::dvec3(
                    (minX + maxX) * 0.5,
                    (minY + maxY) * 0.5,
                    (minZ + maxZ) * 0.5
                );
            }

            /**
             * @brief 获取体积
             */
            constexpr double getVolume() const noexcept {
                return (maxX - minX) * (maxY - minY) * (maxZ - minZ);
            }

            // ========== 运算符 ==========

            constexpr bool operator==(const AABB& other) const noexcept {
                return minX == other.minX && minY == other.minY && minZ == other.minZ &&
                       maxX == other.maxX && maxY == other.maxY && maxZ == other.maxZ;
            }

            constexpr bool operator!=(const AABB& other) const noexcept {
                return !(*this == other);
            }
        };

        /**
         * @brief 射线 - 用于射线检测和拾取
         * 对应 Minecraft: net.minecraft.world.level.ClipContext
         */
        class Ray {
        public:
            glm::dvec3 origin;      // 射线起点
            glm::dvec3 direction;   // 射线方向（归一化）

            constexpr Ray() noexcept
                : origin(0), direction(0, 0, 1) {}

            constexpr Ray(const glm::dvec3& origin, const glm::dvec3& direction) noexcept
                : origin(origin), direction(direction) {}

            /**
             * @brief 获取射线上的点
             * @param t 距离参数
             */
            constexpr glm::dvec3 getPoint(double t) const noexcept {
                return origin + direction * t;
            }

            /**
             * @brief 从两点创建射线
             */
            static Ray fromPoints(const glm::dvec3& from, const glm::dvec3& to) {
                glm::dvec3 dir = glm::normalize(to - from);
                return Ray(from, dir);
            }
        };

        /**
         * @brief 射线检测结果
         * 对应 Minecraft: net.minecraft.world.phys.BlockHitResult
         */
        struct RaycastHit {
            glm::dvec3 position;      // 碰撞位置
            glm::dvec3 normal;        // 碰撞面法线
            AABB* boundingBox;        // 碰撞的 AABB
            bool isValid;             // 是否有效
            double distance;          // 碰撞距离

            /**
             * @brief 碰撞面方向枚举
             */
            enum class FaceDirection {
                DOWN,   // -Y
                UP,     // +Y
                NORTH,  // -Z
                SOUTH,  // +Z
                WEST,   // -X
                EAST    // +X
            };

            FaceDirection faceDirection;

            RaycastHit() noexcept
                : boundingBox(nullptr), isValid(false), distance(0) {}
        };

        /**
         * @brief 碰撞系统核心类
         * 提供各种碰撞检测功能
         */
        class CollisionSystem {
        public:
            /**
             * @brief AABB vs AABB 碰撞检测
             * @return 是否碰撞
             */
            static constexpr bool checkAABB(const AABB& a, const AABB& b) noexcept {
                return a.intersects(b);
            }

            /**
             * @brief AABB vs AABB 碰撞检测（带穿透深度）
             * @param a 第一个 AABB
             * @param b 第二个 AABB
             * @param penetration 输出穿透向量
             * @return 是否碰撞
             */
            static bool checkAABBPenetration(const AABB& a, const AABB& b, glm::dvec3& penetration) {
                if (!a.intersects(b)) {
                    return false;
                }

                // 计算各轴的穿透深度
                double minOverlapX = std::min(a.maxX, b.maxX) - std::max(a.minX, b.minX);
                double minOverlapY = std::min(a.maxY, b.maxY) - std::max(a.minY, b.minY);
                double minOverlapZ = std::min(a.maxZ, b.maxZ) - std::max(a.minZ, b.minZ);

                // 找出最小穿透轴
                if (minOverlapX < minOverlapY && minOverlapX < minOverlapZ) {
                    penetration = glm::dvec3(minOverlapX, 0, 0);
                    // 确定方向
                    if (a.minX < b.minX) penetration.x = -penetration.x;
                } else if (minOverlapY < minOverlapZ) {
                    penetration = glm::dvec3(0, minOverlapY, 0);
                    if (a.minY < b.minY) penetration.y = -penetration.y;
                } else {
                    penetration = glm::dvec3(0, 0, minOverlapZ);
                    if (a.minZ < b.minZ) penetration.z = -penetration.z;
                }

                return true;
            }

            /**
             * @brief 射线 vs AABB 检测
             * @param ray 射线
             * @param aabb 包围盒
             * @param tMin 输出：最近碰撞距离
             * @param tMax 输出：最远碰撞距离
             * @return 是否碰撞
             *
             * 使用 slab 算法实现高效的射线-AABB 检测
             */
            static bool rayCastAABB(const Ray& ray, const AABB& aabb, double& tMin, double& tMax) {
                tMin = 0.0;
                tMax = std::numeric_limits<double>::max();

                auto checkAxis = [&](double origin, double dir, double minVal, double maxVal) -> bool {
                    if (std::abs(dir) < 1e-9) {
                        if (origin < minVal || origin > maxVal) return false;
                    } else {
                        double invDir = 1.0 / dir;
                        double t1 = (minVal - origin) * invDir;
                        double t2 = (maxVal - origin) * invDir;

                        if (t1 > t2) std::swap(t1, t2);

                        tMin = std::max(tMin, t1);
                        tMax = std::min(tMax, t2);

                        if (tMin > tMax) return false;
                    }
                    return true;
                };

                if (!checkAxis(ray.origin.x, ray.direction.x, aabb.minX, aabb.maxX)) return false;
                if (!checkAxis(ray.origin.y, ray.direction.y, aabb.minY, aabb.maxY)) return false;
                if (!checkAxis(ray.origin.z, ray.direction.z, aabb.minZ, aabb.maxZ)) return false;

                return tMin >= 0.0 && tMin <= tMax;
            }

            /**
             * @brief 射线 vs AABB 检测（带法线）
             * @param ray 射线
             * @param aabb 包围盒
             * @param hit 输出碰撞信息
             * @return 是否碰撞
             */
            static bool rayCastAABB(const Ray& ray, const AABB& aabb, RaycastHit& hit) {
                double tMin, tMax;
                if (!rayCastAABB(ray, aabb, tMin, tMax)) {
                    return false;
                }

                hit.position = ray.getPoint(tMin);
                hit.distance = tMin;
                hit.isValid = true;

                // 计算法线（通过判断哪个面的 t 值等于 tMin）
                for (int i = 0; i < 3; i++) {
                    double rayOrigin = ray.origin[i];
                    double rayDir = ray.direction[i];
                    double boxMin = aabb.minX + (i == 1) * (aabb.minY - aabb.minX);
                    double boxMax = aabb.maxX + (i == 1) * (aabb.maxY - aabb.maxX) + (i == 2) * (aabb.maxZ - aabb.maxX);

                    double t1 = (boxMin - rayOrigin) / rayDir;
                    double t2 = (boxMax - rayOrigin) / rayDir;

                    if (std::abs(t1 - tMin) < 1e-6) {
                        hit.normal = glm::dvec3(0);
                        hit.normal[i] = -1.0;
                        return true;
                    }
                    if (std::abs(t2 - tMin) < 1e-6) {
                        hit.normal = glm::dvec3(0);
                        hit.normal[i] = 1.0;
                        return true;
                    }
                }

                return true;
            }

            /**
             * @brief 多个 AABB 射线检测（返回最近的碰撞）
             * @param ray 射线
             * @param aabbs AABB 列表
             * @param maxDistance 最大检测距离
             * @param hit 输出最近的碰撞
             * @return 是否碰撞
             */
            static bool rayCastMultiple(const Ray& ray, const std::vector<AABB>& aabbs,
                                       double maxDistance, RaycastHit& hit) {
                hit.distance = maxDistance;
                hit.isValid = false;

                for (const auto& aabb : aabbs) {
                    RaycastHit currentHit;
                    if (rayCastAABB(ray, aabb, currentHit)) {
                        if (currentHit.distance < hit.distance && currentHit.distance > 0) {
                            hit = currentHit;
                        }
                    }
                }

                return hit.isValid;
            }

            /**
             * @brief 扫描检测（Sweep test）- 检测物体移动过程中的碰撞
             * @param movingAABB 移动的 AABB
             * @param movement 移动向量
             * @param staticAABB 静态 AABB
             * @param hitTime 输出碰撞时间 (0-1)
             * @return 是否碰撞
             */
            static bool sweepAABB(const AABB& movingAABB, const glm::dvec3& movement,
                                 const AABB& staticAABB, double& hitTime) {
                hitTime = 1.0;

                // 检查起始位置是否碰撞
                if (checkAABB(movingAABB, staticAABB)) {
                    hitTime = 0.0;
                    return true;
                }

                // 对每个轴进行扫描
                for (int i = 0; i < 3; i++) {
                    if (std::abs(movement[i]) < 1e-9) continue;

                    double entryTime, exitTime;
                    if (!sweepAxis(movingAABB, movement, staticAABB, i, entryTime, exitTime)) {
                        continue;
                    }

                    if (entryTime > hitTime) {
                        continue;
                    }

                    hitTime = entryTime;
                }

                return hitTime < 1.0;
            }

            /**
             * @brief 碰撞响应 - 解决实体与环境的碰撞
             * @param entityAABB 实体 AABB
             * @param velocity 实体速度（会被修改）
             * @param worldCollisions 世界中的碰撞 AABB 列表
             * @param onGround 输出：是否在地面上
             * @return 是否发生碰撞
             */
            static bool resolveCollisions(const AABB& entityAABB, glm::dvec3& velocity,
                                          const std::vector<AABB>& worldCollisions, bool& onGround) {
                onGround = false;
                bool collided = false;

                // 分轴处理碰撞
                for (int axis = 0; axis < 3; axis++) {
                    if (std::abs(velocity[axis]) < 1e-9) continue;

                    // 移动实体
                    AABB movedAABB = entityAABB;
                    if (axis == 0) movedAABB = movedAABB.move(velocity.x, 0, 0);
                    else if (axis == 1) movedAABB = movedAABB.move(0, velocity.y, 0);
                    else movedAABB = movedAABB.move(0, 0, velocity.z);

                    // 检测碰撞
                    for (const auto& collision : worldCollisions) {
                        if (checkAABB(movedAABB, collision)) {
                            collided = true;

                            if (axis == 1) {
                                // Y 轴碰撞
                                if (velocity.y < 0) {
                                    onGround = true;
                                }
                                velocity.y = 0;
                            } else {
                                // X 或 Z 轴碰撞
                                velocity[axis] = 0;
                            }

                            break;
                        }
                    }
                }

                return collided;
            }

        private:
            /**
             * @brief 单轴扫描检测
             */
            static bool sweepAxis(const AABB& moving, const glm::dvec3& movement,
                                  const AABB& stationary, int axis,
                                  double& entryTime, double& exitTime) {
                double boxMin, boxMax, statMin, statMax;
                if (axis == 0) {
                    boxMin = moving.minX; boxMax = moving.maxX;
                    statMin = stationary.minX; statMax = stationary.maxX;
                } else if (axis == 1) {
                    boxMin = moving.minY; boxMax = moving.maxY;
                    statMin = stationary.minY; statMax = stationary.maxY;
                } else {
                    boxMin = moving.minZ; boxMax = moving.maxZ;
                    statMin = stationary.minZ; statMax = stationary.maxZ;
                }
                
                double move = movement[axis];

                if (std::abs(move) < 1e-9) {
                    entryTime = 0.0;
                    exitTime = 1.0;
                    return boxMin < statMax && boxMax > statMin;
                }

                double invMove = 1.0 / move;
                if (move > 0) {
                    entryTime = (statMin - boxMax) * invMove;
                    exitTime = (statMax - boxMin) * invMove;
                } else {
                    entryTime = (statMax - boxMin) * invMove;
                    exitTime = (statMin - boxMax) * invMove;
                }

                if (entryTime > exitTime || exitTime < 0.0 || entryTime > 1.0) {
                    return false;
                }

                return true;
            }
        };

    } // namespace Physics
} // namespace PrismaEngine
