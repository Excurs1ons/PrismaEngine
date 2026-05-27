#pragma once

#include "CollisionSystem.h"
#include "RigidBody.h"
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <limits>

namespace Prisma {
namespace Physics {

struct SweepResult {
    double toi              = 1.0;   ///< 碰撞时间 [0, 1]，1.0 表示无碰撞
    glm::dvec3 hitNormal    = glm::dvec3(0.0, 0.0, 0.0); ///< 碰撞法线
    bool hasHit             = false; ///< 是否检测到碰撞
};

// 将目标 AABB 扩展球体半径，然后从球心起点到终点做射线检测。
// 命中扩展 AABB 即表示球体在 TOI 时刻接触原始 AABB。
inline SweepResult sweepSphere(
    const glm::dvec3& sphereStart,
    const glm::dvec3& sphereEnd,
    double sphereRadius,
    const AABB& targetAABB) noexcept
{
    SweepResult result;

    AABB expandedAABB = targetAABB.expand(sphereRadius, sphereRadius, sphereRadius);
    glm::dvec3 direction = sphereEnd - sphereStart;
    double dist = glm::length(direction);

    if (dist < 1e-9) {
        AABB sphereAABB = AABB::fromCenterSize(
            sphereStart.x, sphereStart.y, sphereStart.z,
            sphereRadius * 2.0, sphereRadius * 2.0, sphereRadius * 2.0);
        result.hasHit = CollisionSystem::checkAABB(sphereAABB, targetAABB);
        result.toi = result.hasHit ? 0.0 : 1.0;
        if (result.hasHit) {
            glm::dvec3 center = targetAABB.getCenter();
            glm::dvec3 diff = sphereStart - center;
            glm::dvec3 absDiff = glm::abs(diff);
            if (absDiff.x > absDiff.y && absDiff.x > absDiff.z)
                result.hitNormal = glm::dvec3(diff.x > 0.0 ? 1.0 : -1.0, 0.0, 0.0);
            else if (absDiff.y > absDiff.z)
                result.hitNormal = glm::dvec3(0.0, diff.y > 0.0 ? 1.0 : -1.0, 0.0);
            else
                result.hitNormal = glm::dvec3(0.0, 0.0, diff.z > 0.0 ? 1.0 : -1.0);
            if (glm::length(result.hitNormal) > 1e-9)
                result.hitNormal = glm::normalize(result.hitNormal);
            else
                result.hitNormal = glm::dvec3(0.0, 1.0, 0.0);
        }
        return result;
    }

    direction /= dist;
    Ray ray(sphereStart, direction);
    double tMin = 0.0, tMax = 0.0;

    if (CollisionSystem::rayCastAABB(ray, expandedAABB, tMin, tMax)) {
        double clampedTMin = std::max(0.0, tMin);
        if (clampedTMin <= dist) {
            result.hasHit = true;
            result.toi = clampedTMin / dist;

            glm::dvec3 hitPoint = ray.getPoint(clampedTMin);
            const double eps = 1e-4;
            if (std::abs(hitPoint.x - expandedAABB.minX) < eps)
                result.hitNormal = glm::dvec3(-1.0, 0.0, 0.0);
            else if (std::abs(hitPoint.x - expandedAABB.maxX) < eps)
                result.hitNormal = glm::dvec3(1.0, 0.0, 0.0);
            else if (std::abs(hitPoint.y - expandedAABB.minY) < eps)
                result.hitNormal = glm::dvec3(0.0, -1.0, 0.0);
            else if (std::abs(hitPoint.y - expandedAABB.maxY) < eps)
                result.hitNormal = glm::dvec3(0.0, 1.0, 0.0);
            else if (std::abs(hitPoint.z - expandedAABB.minZ) < eps)
                result.hitNormal = glm::dvec3(0.0, 0.0, -1.0);
            else if (std::abs(hitPoint.z - expandedAABB.maxZ) < eps)
                result.hitNormal = glm::dvec3(0.0, 0.0, 1.0);
            else
                result.hitNormal = glm::dvec3(0.0, 1.0, 0.0);
        }
    }

    return result;
}

// 二分搜索逼近首次碰撞时间。每步推进 movement * t 并检查 overlap，
// 穿透小于 allowedPenetration 时视为碰撞。
inline bool sweepAABB(
    const AABB& movingAABB,
    const glm::dvec3& movement,
    const AABB& staticAABB,
    double& hitTime,
    double allowedPenetration = 0.01,
    int maxIterations = 10) noexcept
{
    hitTime = 1.0;
    if (CollisionSystem::checkAABB(movingAABB, staticAABB)) {
        hitTime = 0.0;
        return true;
    }

    double moveLen = glm::length(movement);
    if (moveLen < 1e-9) return false;

    double low = 0.0, high = 1.0;
    bool foundCollision = false;

    for (int iter = 0; iter < maxIterations; ++iter) {
        double mid = (low + high) * 0.5;
        AABB sweptAABB = movingAABB.move(movement * mid);

        if (CollisionSystem::checkAABB(sweptAABB, staticAABB)) {
            glm::dvec3 penetration(0.0);
            if (CollisionSystem::checkAABBPenetration(sweptAABB, staticAABB, penetration)) {
                if (glm::length(penetration) <= allowedPenetration) {
                    hitTime = mid;
                    return true;
                }
            }
            high = mid;
            foundCollision = true;
        } else {
            low = mid;
        }

        if (high - low < 1e-9) break;
    }

    if (foundCollision) {
        hitTime = (low + high) * 0.5;
        return true;
    }
    return false;
}

/**
 * @brief 连续碰撞检测求解器
 * 对启用 CCD 且速度超过阈值的刚体执行扫描检测，
 * 碰撞时回滚位置、反射速度，推进剩余时间。
 */
class CCDSolver {
public:
    int maxCCDIterations = 3;
    double ccdAllowedPenetration = 0.01;

    void performCCD(std::vector<std::unique_ptr<RigidBody>>& bodies, double dt) noexcept {
        for (size_t i = 0; i < bodies.size(); ++i) {
            auto& body = bodies[i];
            if (!body->isActive() || body->isStatic() || !body->isAwake()) continue;
            if (!body->isCCDEnabled()) continue;

            double speed = glm::length(body->m_linearVelocity);
            if (speed < body->getCcdMotionThreshold()) continue;

            glm::dvec3 remainingMovement = body->m_linearVelocity * dt;
            double remainingDt = dt;

            for (int ccdIter = 0; ccdIter < maxCCDIterations; ++ccdIter) {
                if (glm::length(remainingMovement) < 1e-9) break;

                glm::dvec3 startPos = body->m_position;

                double sphereRadius = (body->m_collisionHalfSize.x +
                                       body->m_collisionHalfSize.y +
                                       body->m_collisionHalfSize.z) / 3.0;

                double earliestTOI = 1.0;
                glm::dvec3 collisionNormal(0.0);
                bool foundCollision = false;

                for (size_t j = 0; j < bodies.size(); ++j) {
                    if (i == j) continue;
                    auto& other = bodies[j];
                    if (!other->isActive()) continue;

                    AABB otherAABB = other->getWorldAABB();
                    SweepResult sweepResult = sweepSphere(
                        startPos,
                        startPos + remainingMovement,
                        sphereRadius,
                        otherAABB
                    );

                    if (sweepResult.hasHit && sweepResult.toi < earliestTOI) {
                        earliestTOI = sweepResult.toi;
                        collisionNormal = sweepResult.hitNormal;
                        foundCollision = true;
                    }
                }

                if (!foundCollision || earliestTOI >= 1.0) {
                    body->m_position += remainingMovement;
                    break;
                }

                body->m_position = startPos + remainingMovement * earliestTOI;

                double velAlongNormal = glm::dot(body->m_linearVelocity, collisionNormal);
                if (velAlongNormal > 0.0) {
                    body->m_linearVelocity -= collisionNormal * velAlongNormal;
                }

                remainingDt *= (1.0 - earliestTOI);
                remainingMovement = body->m_linearVelocity * remainingDt;

                if (remainingDt < 1e-9) break;
            }
        }
    }
};

} // namespace Physics
} // namespace Prisma
