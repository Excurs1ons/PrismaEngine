#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "physics/CollisionSystem.h"
#include <functional>
#include <cstdint>
#include <unordered_set>

namespace Prisma {
namespace Physics {

// 触发体积形状
enum class TriggerShapeType {
    AABB,   // 轴对齐包围盒
    Sphere, // 球体
    Box     // 朝向包围盒
};

// 触发体积 - 检测实体进入/离开/停留
class TriggerVolume {
public:
    TriggerShapeType shapeType;
    AABB aabb;                          // 用于 AABB 类
    glm::dvec3 center{ 0.0 };           // 用于球体
    double radius = 1.0;               // 用于球体
    glm::dquat rotation = glm::dquat(1.0, 0.0, 0.0, 0.0); // 用于朝向 Box
    glm::dvec3 halfSize{ 0.5 };         // 用于 Box

    // 上一帧在触发区内的实体集合
    std::unordered_set<uint32_t> previousOverlaps;

    // 回调函数
    std::function<void(uint32_t)> onEnter;
    std::function<void(uint32_t)> onExit;
    std::function<void(uint32_t)> onStay;

    // 是否启用
    bool enabled = true;

    // 用户数据
    void* userData = nullptr;

    TriggerVolume() = default;

    // AABB trigger constructor
    static TriggerVolume createAABB(const AABB& aabb,
                                     std::function<void(uint32_t)> enter = nullptr,
                                     std::function<void(uint32_t)> exit_ = nullptr,
                                     std::function<void(uint32_t)> stay = nullptr) {
        TriggerVolume tv;
        tv.shapeType = TriggerShapeType::AABB;
        tv.aabb = aabb;
        tv.center = aabb.getCenter();
        tv.onEnter = std::move(enter);
        tv.onExit = std::move(exit_);
        tv.onStay = std::move(stay);
        return tv;
    }

    // Sphere trigger constructor
    static TriggerVolume createSphere(const glm::dvec3& center, double radius,
                                       std::function<void(uint32_t)> enter = nullptr,
                                       std::function<void(uint32_t)> exit_ = nullptr,
                                       std::function<void(uint32_t)> stay = nullptr) {
        TriggerVolume tv;
        tv.shapeType = TriggerShapeType::Sphere;
        tv.center = center;
        tv.radius = radius;
        tv.onEnter = std::move(enter);
        tv.onExit = std::move(exit_);
        tv.onStay = std::move(stay);
        return tv;
    }

    // Box (OBB) trigger constructor
    static TriggerVolume createBox(const glm::dvec3& center, const glm::dquat& rotation,
                                    const glm::dvec3& halfSize,
                                    std::function<void(uint32_t)> enter = nullptr,
                                    std::function<void(uint32_t)> exit_ = nullptr,
                                    std::function<void(uint32_t)> stay = nullptr) {
        TriggerVolume tv;
        tv.shapeType = TriggerShapeType::Box;
        tv.center = center;
        tv.rotation = rotation;
        tv.halfSize = halfSize;
        tv.onEnter = std::move(enter);
        tv.onExit = std::move(exit_);
        tv.onStay = std::move(stay);
        return tv;
    }

    // 检测一个点（实体的简化表示）是否在触发区内
    bool contains(const glm::dvec3& point) const {
        if (!enabled) return false;

        switch (shapeType) {
        case TriggerShapeType::AABB:
            return aabb.contains(point);
        case TriggerShapeType::Sphere:
            return glm::length(point - center) <= radius;
        case TriggerShapeType::Box: {
            // OBB 检测：将点转换到 OBB 局部空间
            glm::dquat invRot = glm::inverse(rotation);
            glm::dvec3 localPoint = invRot * (point - center);
            return std::abs(localPoint.x) <= halfSize.x &&
                   std::abs(localPoint.y) <= halfSize.y &&
                   std::abs(localPoint.z) <= halfSize.z;
        }
        }
        return false;
    }

    // 检测 AABB 是否与触发区重叠
    bool overlaps(const AABB& entityAABB) const {
        if (!enabled) return false;

        switch (shapeType) {
        case TriggerShapeType::AABB:
            return aabb.intersects(entityAABB);
        case TriggerShapeType::Sphere: {
            // AABB vs Sphere
            glm::dvec3 closest = glm::clamp(center,
                glm::dvec3(entityAABB.minX, entityAABB.minY, entityAABB.minZ),
                glm::dvec3(entityAABB.maxX, entityAABB.maxY, entityAABB.maxZ));
            glm::dvec3 diff = center - closest;
            return glm::dot(diff, diff) <= radius * radius;
        }
        case TriggerShapeType::Box: {
            // Simplified: use sphere approximation for box vs AABB
            // For robust OBB-AABB, use SAT - but this is sufficient for most cases
            glm::dvec3 boxCenter = center;
            double maxHalfExtent = std::max({ halfSize.x, halfSize.y, halfSize.z });
            glm::dvec3 closest = glm::clamp(boxCenter,
                glm::dvec3(entityAABB.minX, entityAABB.minY, entityAABB.minZ),
                glm::dvec3(entityAABB.maxX, entityAABB.maxY, entityAABB.maxZ));
            glm::dvec3 diff = boxCenter - closest;
            return glm::dot(diff, diff) <= maxHalfExtent * maxHalfExtent * 3.0;
        }
        }
        return false;
    }
};

} // namespace Physics
} // namespace Prisma
