#include "Physics2D.h"
#include <algorithm>
#include <cmath>

namespace Prisma::Physics2D {

bool Physics2D::CheckAABB(const AABB2D& a, const AABB2D& b) {
    return a.Intersects(b);
}

bool Physics2D::SweepAABB(const AABB2D& moving, glm::vec2 velocity,
                          const AABB2D& static_, float& hitTime, glm::vec2& normal) {
    if (velocity.x == 0.0f && velocity.y == 0.0f) {
        hitTime = 1.0f;
        return moving.Intersects(static_);
    }

    // 将静态 AABB 扩展移动体的尺寸，将移动体缩为点
    AABB2D expanded(
        static_.minX - moving.GetWidth() * 0.5f,
        static_.minY - moving.GetHeight() * 0.5f,
        static_.maxX + moving.GetWidth() * 0.5f,
        static_.maxY + moving.GetHeight() * 0.5f
    );

    // 从移动体中心沿速度方向进行射线检测
    glm::vec2 rayOrigin = moving.GetCenter();
    glm::vec2 rayDir = velocity;
    float invDirX = (rayDir.x != 0.0f) ? 1.0f / rayDir.x : 1e10f;
    float invDirY = (rayDir.y != 0.0f) ? 1.0f / rayDir.y : 1e10f;

    float tMin, tMax;
    if (!RayVsAABB(rayOrigin, {invDirX, invDirY}, expanded, tMin, tMax))
        return false;

    if (tMin < 0.0f) tMin = 0.0f;
    if (tMin > 1.0f) return false;

    hitTime = tMin;

    // 从接触点计算法线
    glm::vec2 contact = rayOrigin + rayDir * tMin;
    glm::vec2 center = expanded.GetCenter();
    glm::vec2 halfSize = expanded.GetSize() * 0.5f;
    glm::vec2 diff = contact - center;

    float overlapX = halfSize.x - std::abs(diff.x);
    float overlapY = halfSize.y - std::abs(diff.y);

    if (overlapX < overlapY) {
        normal = (diff.x > 0.0f) ? glm::vec2(-1.0f, 0.0f) : glm::vec2(1.0f, 0.0f);
    } else {
        normal = (diff.y > 0.0f) ? glm::vec2(0.0f, -1.0f) : glm::vec2(0.0f, 1.0f);
    }

    return true;
}

bool Physics2D::ResolvePlatform(const AABB2D& player, glm::vec2& velocity,
                                const AABB2D* solids, uint32_t count,
                                bool& onGround, bool& hitCeiling) {
    onGround = false;
    hitCeiling = false;
    bool anyCollision = false;

    struct Hit {
        float time;
        glm::vec2 normal;
    };
    std::vector<Hit> hits;
    hits.reserve(count);

    for (uint32_t i = 0; i < count; i++) {
        float hitTime;
        glm::vec2 normal;
        if (SweepAABB(player, velocity, solids[i], hitTime, normal)) {
            hits.push_back({hitTime, normal});
        }
    }

    // 按碰撞时间排序（最近的优先）
    std::sort(hits.begin(), hits.end(),
        [](const Hit& a, const Hit& b) { return a.time < b.time; });

    float remainingTime = 1.0f;
    for (const auto& hit : hits) {
        if (hit.time > remainingTime) break;

        // 沿法线方向消除速度分量
        float dot = glm::dot(velocity, hit.normal);
        if (dot < 0.0f) {
            velocity -= hit.normal * dot;
        }

        if (hit.normal.y < -0.5f) onGround = true;
        if (hit.normal.y > 0.5f) hitCeiling = true;

        remainingTime -= hit.time;
        anyCollision = true;
    }

    return anyCollision;
}

bool Physics2D::CheckOneWayPlatform(const AABB2D& player,
                                     const AABB2D& platform,
                                     float playerPrevBottom,
                                     float playerVelY) {
    // 仅当玩家正在下落、之前位于平台上方时发生碰撞
    if (playerVelY >= 0.0f) return false;                    // 不在下落
    if (playerPrevBottom >= platform.maxY) return false;     // 已经在平台下方
    if (player.minY >= platform.maxY) return false;          // 仍然在平台上方

    // 检查水平方向重叠
    return player.minX < platform.maxX && player.maxX > platform.minX;
}

bool Physics2D::RayVsAABB(glm::vec2 origin, glm::vec2 invDir,
                          const AABB2D& box, float& tMin, float& tMax) {
    float t1 = (box.minX - origin.x) * invDir.x;
    float t2 = (box.maxX - origin.x) * invDir.x;
    float t3 = (box.minY - origin.y) * invDir.y;
    float t4 = (box.maxY - origin.y) * invDir.y;

    tMin = std::max(std::min(t1, t2), std::min(t3, t4));
    tMax = std::min(std::max(t1, t2), std::max(t3, t4));

    return tMax >= tMin && tMax >= 0.0f;
}

RaycastHit2D Physics2D::RayCast(glm::vec2 origin, glm::vec2 direction,
                                float maxDist, const AABB2D* targets, uint32_t count) {
    RaycastHit2D result;
    float closest = maxDist;

    // 计算逆方向（用于 slab 测试）
    glm::vec2 invDir = direction;
    if (invDir.x != 0.0f) invDir.x = 1.0f / invDir.x;
    else invDir.x = 1e10f;
    if (invDir.y != 0.0f) invDir.y = 1.0f / invDir.y;
    else invDir.y = 1e10f;

    for (uint32_t i = 0; i < count; i++) {
        float tMin, tMax;
        if (!RayVsAABB(origin, invDir, targets[i], tMin, tMax)) continue;
        if (tMin > 0.0f && tMin < closest) {
            closest = tMin;
            result.hit = true;
            result.distance = tMin * std::sqrt(direction.x * direction.x + direction.y * direction.y);
            result.point = origin + direction * tMin;

            // 计算法线
            glm::vec2 center = targets[i].GetCenter();
            glm::vec2 halfSize = targets[i].GetSize() * 0.5f;
            glm::vec2 diff = result.point - center;
            float ox = halfSize.x - std::abs(diff.x);
            float oy = halfSize.y - std::abs(diff.y);
            if (ox < oy)
                result.normal = (diff.x > 0) ? glm::vec2(-1, 0) : glm::vec2(1, 0);
            else
                result.normal = (diff.y > 0) ? glm::vec2(0, -1) : glm::vec2(0, 1);
        }
    }
    return result;
}

} // namespace Prisma::Physics2D
