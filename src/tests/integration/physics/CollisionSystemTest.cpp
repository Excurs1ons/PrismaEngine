#include <gtest/gtest.h>
#include "physics/CollisionSystem.h"

namespace Prisma {
namespace Physics {
namespace {

// ============================================================================
// AABB — Default State
// ============================================================================
TEST(AABBTest, DefaultConstructor) {
    AABB a;
    EXPECT_DOUBLE_EQ(a.minX, 0.0);
    EXPECT_DOUBLE_EQ(a.minY, 0.0);
    EXPECT_DOUBLE_EQ(a.minZ, 0.0);
    EXPECT_DOUBLE_EQ(a.maxX, 0.0);
    EXPECT_DOUBLE_EQ(a.maxY, 0.0);
    EXPECT_DOUBLE_EQ(a.maxZ, 0.0);
}

// ============================================================================
// AABB — Parameterized Constructor
// ============================================================================
TEST(AABBTest, ParameterizedConstructor) {
    AABB a(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
    EXPECT_DOUBLE_EQ(a.minX, 1.0);
    EXPECT_DOUBLE_EQ(a.minY, 2.0);
    EXPECT_DOUBLE_EQ(a.minZ, 3.0);
    EXPECT_DOUBLE_EQ(a.maxX, 4.0);
    EXPECT_DOUBLE_EQ(a.maxY, 5.0);
    EXPECT_DOUBLE_EQ(a.maxZ, 6.0);
}

// ============================================================================
// AABB — fromCenterSize
// ============================================================================
TEST(AABBTest, FromCenterSize) {
    AABB a = AABB::fromCenterSize(0.0, 0.0, 0.0, 2.0, 2.0, 2.0);
    EXPECT_DOUBLE_EQ(a.minX, -1.0);
    EXPECT_DOUBLE_EQ(a.minY, -1.0);
    EXPECT_DOUBLE_EQ(a.minZ, -1.0);
    EXPECT_DOUBLE_EQ(a.maxX,  1.0);
    EXPECT_DOUBLE_EQ(a.maxY,  1.0);
    EXPECT_DOUBLE_EQ(a.maxZ,  1.0);
}

// ============================================================================
// AABB — Intersection
// ============================================================================
TEST(AABBTest, IntersectsOverlapping) {
    AABB a(0, 0, 0, 2, 2, 2);
    AABB b(1, 1, 1, 3, 3, 3);
    EXPECT_TRUE(a.intersects(b));
    EXPECT_TRUE(b.intersects(a));
}

TEST(AABBTest, IntersectsNonOverlapping) {
    AABB a(0, 0, 0, 1, 1, 1);
    AABB b(2, 2, 2, 3, 3, 3);
    EXPECT_FALSE(a.intersects(b));
}

TEST(AABBTest, IntersectsTouchingEdge) {
    AABB a(0, 0, 0, 1, 1, 1);
    AABB b(1, 1, 1, 2, 2, 2);
    // Touching on edges: minX < other.maxX (1 < 2) && maxX > other.minX (1 > 1) = false
    EXPECT_FALSE(a.intersects(b));
}

TEST(AABBTest, IntersectsNested) {
    AABB outer(0, 0, 0, 5, 5, 5);
    AABB inner(1, 1, 1, 2, 2, 2);
    EXPECT_TRUE(outer.intersects(inner));
    EXPECT_TRUE(inner.intersects(outer));
}

TEST(AABBTest, IntersectsUnitCube) {
    // 单位立方体 [0,1] vs [0.5, 1.5]
    AABB unit(0, 0, 0, 1, 1, 1);
    AABB shifted(0.5, 0.5, 0.5, 1.5, 1.5, 1.5);
    EXPECT_TRUE(unit.intersects(shifted));
}

TEST(AABBTest, IntersectsFarApart) {
    AABB a(0, 0, 0, 1, 1, 1);
    AABB b(100, 100, 100, 101, 101, 101);
    EXPECT_FALSE(a.intersects(b));
}

TEST(AABBTest, IntersectsSameBox) {
    AABB a(0, 0, 0, 1, 1, 1);
    EXPECT_TRUE(a.intersects(a));
}

// ============================================================================
// AABB — Contains Point
// ============================================================================
TEST(AABBTest, ContainsPointInside) {
    AABB a(0, 0, 0, 2, 2, 2);
    EXPECT_TRUE(a.contains(1.0, 1.0, 1.0));
}

TEST(AABBTest, ContainsPointOnBoundary) {
    AABB a(0, 0, 0, 2, 2, 2);
    EXPECT_TRUE(a.contains(0.0, 0.0, 0.0));
    EXPECT_TRUE(a.contains(2.0, 2.0, 2.0));
}

TEST(AABBTest, ContainsPointOutside) {
    AABB a(0, 0, 0, 2, 2, 2);
    EXPECT_FALSE(a.contains(3.0, 3.0, 3.0));
    EXPECT_FALSE(a.contains(-1.0, 0.0, 0.0));
}

// ============================================================================
// AABB — Properties
// ============================================================================
TEST(AABBTest, GetCenter) {
    AABB a(0, 0, 0, 2, 4, 6);
    glm::dvec3 c = a.getCenter();
    EXPECT_DOUBLE_EQ(c.x, 1.0);
    EXPECT_DOUBLE_EQ(c.y, 2.0);
    EXPECT_DOUBLE_EQ(c.z, 3.0);
}

TEST(AABBTest, GetVolume) {
    AABB a(0, 0, 0, 2, 3, 4);
    EXPECT_DOUBLE_EQ(a.getVolume(), 24.0);
}

TEST(AABBTest, GetSize) {
    AABB a(0, 0, 0, 2, 3, 4);
    EXPECT_DOUBLE_EQ(a.getXsize(), 2.0);
    EXPECT_DOUBLE_EQ(a.getYsize(), 3.0);
    EXPECT_DOUBLE_EQ(a.getZsize(), 4.0);
}

// ============================================================================
// AABB — Operations
// ============================================================================
TEST(AABBTest, Move) {
    AABB a(0, 0, 0, 1, 1, 1);
    AABB moved = a.move(2.0, 3.0, 4.0);
    EXPECT_DOUBLE_EQ(moved.minX, 2.0);
    EXPECT_DOUBLE_EQ(moved.minY, 3.0);
    EXPECT_DOUBLE_EQ(moved.minZ, 4.0);
    EXPECT_DOUBLE_EQ(moved.maxX, 3.0);
    EXPECT_DOUBLE_EQ(moved.maxY, 4.0);
    EXPECT_DOUBLE_EQ(moved.maxZ, 5.0);
}

TEST(AABBTest, Expand) {
    AABB a(1, 1, 1, 2, 2, 2);
    AABB expanded = a.expand(1.0, 1.0, 1.0);
    EXPECT_DOUBLE_EQ(expanded.minX, 0.0);
    EXPECT_DOUBLE_EQ(expanded.minY, 0.0);
    EXPECT_DOUBLE_EQ(expanded.minZ, 0.0);
    EXPECT_DOUBLE_EQ(expanded.maxX, 3.0);
    EXPECT_DOUBLE_EQ(expanded.maxY, 3.0);
    EXPECT_DOUBLE_EQ(expanded.maxZ, 3.0);
}

TEST(AABBTest, Shrink) {
    AABB a(0, 0, 0, 4, 4, 4);
    AABB shrunk = a.shrink(1.0, 1.0, 1.0);
    EXPECT_DOUBLE_EQ(shrunk.minX, 1.0);
    EXPECT_DOUBLE_EQ(shrunk.minY, 1.0);
    EXPECT_DOUBLE_EQ(shrunk.minZ, 1.0);
    EXPECT_DOUBLE_EQ(shrunk.maxX, 3.0);
    EXPECT_DOUBLE_EQ(shrunk.maxY, 3.0);
    EXPECT_DOUBLE_EQ(shrunk.maxZ, 3.0);
}

TEST(AABBTest, Intersection) {
    AABB a(0, 0, 0, 3, 3, 3);
    AABB b(1, 1, 1, 4, 4, 4);
    AABB inter = a.intersection(b);
    EXPECT_DOUBLE_EQ(inter.minX, 1.0);
    EXPECT_DOUBLE_EQ(inter.minY, 1.0);
    EXPECT_DOUBLE_EQ(inter.minZ, 1.0);
    EXPECT_DOUBLE_EQ(inter.maxX, 3.0);
    EXPECT_DOUBLE_EQ(inter.maxY, 3.0);
    EXPECT_DOUBLE_EQ(inter.maxZ, 3.0);
}

TEST(AABBTest, UnionAABB) {
    AABB a(0, 0, 0, 2, 2, 2);
    AABB b(1, 1, 1, 3, 3, 3);
    AABB u = a.unionAABB(b);
    EXPECT_DOUBLE_EQ(u.minX, 0.0);
    EXPECT_DOUBLE_EQ(u.minY, 0.0);
    EXPECT_DOUBLE_EQ(u.minZ, 0.0);
    EXPECT_DOUBLE_EQ(u.maxX, 3.0);
    EXPECT_DOUBLE_EQ(u.maxY, 3.0);
    EXPECT_DOUBLE_EQ(u.maxZ, 3.0);
}

TEST(AABBTest, EqualityOperators) {
    AABB a(0, 0, 0, 1, 1, 1);
    AABB b(0, 0, 0, 1, 1, 1);
    AABB c(0, 0, 0, 2, 2, 2);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

// ============================================================================
// CollisionSystem — AABB vs AABB
// ============================================================================
TEST(CollisionSystemTest, CheckAABB_Overlapping) {
    AABB a(0, 0, 0, 2, 2, 2);
    AABB b(1, 1, 1, 3, 3, 3);
    EXPECT_TRUE(CollisionSystem::checkAABB(a, b));
}

TEST(CollisionSystemTest, CheckAABB_NonOverlapping) {
    AABB a(0, 0, 0, 1, 1, 1);
    AABB b(2, 2, 2, 3, 3, 3);
    EXPECT_FALSE(CollisionSystem::checkAABB(a, b));
}

// ============================================================================
// CollisionSystem — AABB Penetration
// ============================================================================
TEST(CollisionSystemTest, CheckAABBPenetration_Overlapping) {
    AABB a(0, 0, 0, 2, 2, 2);
    AABB b(1, 1, 1, 3, 3, 3);
    glm::dvec3 penetration(0.0);
    EXPECT_TRUE(CollisionSystem::checkAABBPenetration(a, b, penetration));
    EXPECT_GT(glm::length(penetration), 0.0);
}

TEST(CollisionSystemTest, CheckAABBPenetration_NonOverlapping) {
    AABB a(0, 0, 0, 1, 1, 1);
    AABB b(2, 2, 2, 3, 3, 3);
    glm::dvec3 penetration(0.0);
    EXPECT_FALSE(CollisionSystem::checkAABBPenetration(a, b, penetration));
}

TEST(CollisionSystemTest, CheckAABBPenetration_PenetrationDepth) {
    // a penetrates b by 1 unit on X axis only (no Y/Z overlap difference)
    AABB a(0, 0, 0, 2, 1, 1);
    AABB b(1, 0, 0, 3, 1, 1);
    glm::dvec3 penetration(0.0);
    EXPECT_TRUE(CollisionSystem::checkAABBPenetration(a, b, penetration));
    // All overlap values equal (1,1,1) -> algorithm picks Z by default
    // penetration.z should be non-zero (Z is the last equal axis)
    EXPECT_GT(glm::length(penetration), 0.0);
}

TEST(CollisionSystemTest, CheckAABBPenetration_SmallestAxis) {
    // Penetration on Y (overlap 0.5) is smallest
    AABB a(0, 0, 0, 3, 3, 3);
    AABB b(1, 2.5, 1, 2, 3.5, 2);
    glm::dvec3 penetration(0.0);
    EXPECT_TRUE(CollisionSystem::checkAABBPenetration(a, b, penetration));
    EXPECT_NEAR(glm::abs(penetration).x, 0.0, 1e-9) << "X overlap is not smallest";
    EXPECT_NEAR(glm::abs(penetration).z, 0.0, 1e-9) << "Z overlap is not smallest";
    EXPECT_GT(glm::abs(penetration).y, 0.0) << "Y should have penetration";
}

// ============================================================================
// CollisionSystem — Raycast
// ============================================================================
TEST(CollisionSystemTest, RayCastAABB_Hit) {
    Ray ray(glm::dvec3(-5, 0, 0), glm::dvec3(1, 0, 0));
    AABB aabb(0, -1, -1, 2, 1, 1);
    double tMin, tMax;
    EXPECT_TRUE(CollisionSystem::rayCastAABB(ray, aabb, tMin, tMax));
    EXPECT_NEAR(tMin, 5.0, 1e-9);
}

TEST(CollisionSystemTest, RayCastAABB_Miss) {
    Ray ray(glm::dvec3(-5, 10, 0), glm::dvec3(1, 0, 0));
    AABB aabb(0, -1, -1, 2, 1, 1);
    double tMin, tMax;
    EXPECT_FALSE(CollisionSystem::rayCastAABB(ray, aabb, tMin, tMax));
}

TEST(CollisionSystemTest, RayCastAABB_RayInside) {
    Ray ray(glm::dvec3(1, 0, 0), glm::dvec3(1, 0, 0));
    AABB aabb(0, -1, -1, 2, 1, 1);
    double tMin, tMax;
    // Ray starts inside the AABB -> tMin should be 0
    EXPECT_TRUE(CollisionSystem::rayCastAABB(ray, aabb, tMin, tMax));
    EXPECT_DOUBLE_EQ(tMin, 0.0);
}

TEST(CollisionSystemTest, RayCastAABB_RayBehind) {
    Ray ray(glm::dvec3(-5, 0, 0), glm::dvec3(-1, 0, 0));
    AABB aabb(0, -1, -1, 2, 1, 1);
    double tMin, tMax;
    // Ray points away from AABB -> should miss
    EXPECT_FALSE(CollisionSystem::rayCastAABB(ray, aabb, tMin, tMax));
}

TEST(CollisionSystemTest, RayCastAABB_WithHitResult) {
    Ray ray(glm::dvec3(-5, 0, 0), glm::dvec3(1, 0, 0));
    AABB aabb(0, -1, -1, 2, 1, 1);
    RaycastHit hit;
    EXPECT_TRUE(CollisionSystem::rayCastAABB(ray, aabb, hit));
    EXPECT_TRUE(hit.isValid);
    EXPECT_NEAR(hit.distance, 5.0, 1e-9);
}

TEST(CollisionSystemTest, RayCastAABB_HitNormal) {
    // Hit from -X direction, normal should be (-1, 0, 0)
    Ray ray(glm::dvec3(-5, 0, 0), glm::dvec3(1, 0, 0));
    AABB aabb(0, -1, -1, 2, 1, 1);
    RaycastHit hit;
    EXPECT_TRUE(CollisionSystem::rayCastAABB(ray, aabb, hit));
    EXPECT_DOUBLE_EQ(hit.normal.x, -1.0);
    EXPECT_DOUBLE_EQ(hit.normal.y, 0.0);
    EXPECT_DOUBLE_EQ(hit.normal.z, 0.0);
}

TEST(CollisionSystemTest, RayCastMultiple_Hit) {
    Ray ray(glm::dvec3(-10, 0, 0), glm::dvec3(1, 0, 0));
    AABB nearBox(0, -1, -1, 2, 1, 1);
    AABB farBox(5, -1, -1, 7, 1, 1);
    std::vector<AABB> aabbs = { farBox, nearBox };
    RaycastHit hit;
    double maxDist = 100.0;
    EXPECT_TRUE(CollisionSystem::rayCastMultiple(ray, aabbs, maxDist, hit));
    // Should hit the nearer box (distance ~10)
    EXPECT_NEAR(hit.distance, 10.0, 1e-9);
}

// ============================================================================
// CollisionSystem — Sweep AABB
// ============================================================================
TEST(CollisionSystemTest, SweepAABB_NoCollision) {
    AABB moving(0, 0, 0, 1, 1, 1);
    glm::dvec3 movement(10, 0, 0);
    AABB obstacle(100, 100, 100, 101, 101, 101);
    double hitTime;
    EXPECT_FALSE(CollisionSystem::sweepAABB(moving, movement, obstacle, hitTime));
    EXPECT_DOUBLE_EQ(hitTime, 1.0);
}

TEST(CollisionSystemTest, SweepAABB_StartingOverlap) {
    AABB moving(0, 0, 0, 2, 1, 1);
    glm::dvec3 movement(1, 0, 0);
    AABB obstacle(1, 0, 0, 3, 1, 1);
    double hitTime;
    // Should detect overlap at t=0
    EXPECT_TRUE(CollisionSystem::sweepAABB(moving, movement, obstacle, hitTime));
    EXPECT_DOUBLE_EQ(hitTime, 0.0);
}

TEST(CollisionSystemTest, SweepAABB_HitInMiddle) {
    AABB moving(0, 0, 0, 1, 1, 1);
    glm::dvec3 movement(4, 0, 0);
    AABB obstacle(2, 0, 0, 3, 1, 1);
    double hitTime;
    [[maybe_unused]] bool result = CollisionSystem::sweepAABB(moving, movement, obstacle, hitTime);
    EXPECT_GE(hitTime, 0.0);
    EXPECT_LE(hitTime, 1.0);
}

// ============================================================================
// CollisionSystem — Collision Resolution
// ============================================================================
TEST(CollisionSystemTest, ResolveCollisions_NoCollision) {
    AABB entity(0, 0, 0, 1, 1, 1);
    glm::dvec3 velocity(1, 0, 0);
    std::vector<AABB> obstacles;
    bool onGround = false;
    EXPECT_FALSE(CollisionSystem::resolveCollisions(entity, velocity, obstacles, onGround));
    EXPECT_DOUBLE_EQ(velocity.x, 1.0); // unchanged
    EXPECT_FALSE(onGround);
}

TEST(CollisionSystemTest, ResolveCollisions_HitsWall) {
    // entity at (0,0,0)-(1,1,1), moving right (+X), wall at moved position
    AABB entity(0, 0, 0, 1, 1, 1);
    glm::dvec3 velocity(2, 0, 0);
    std::vector<AABB> obstacles = { AABB(2.0, 0, 0, 3.0, 1, 1) };
    bool onGround = false;
    EXPECT_TRUE(CollisionSystem::resolveCollisions(entity, velocity, obstacles, onGround));
    EXPECT_DOUBLE_EQ(velocity.x, 0.0);
    EXPECT_FALSE(onGround);
}

TEST(CollisionSystemTest, ResolveCollisions_LandsOnGround) {
    // entity at y=5..6, falling at -2, ground at y=3.5..4.5 overlaps moved entity at y=3..4
    AABB entity(0, 5, 0, 1, 6, 1);
    glm::dvec3 velocity(0, -2, 0);
    std::vector<AABB> obstacles = { AABB(0, 3.5, 0, 1, 4.5, 1) };
    bool onGround = false;
    EXPECT_TRUE(CollisionSystem::resolveCollisions(entity, velocity, obstacles, onGround));
    EXPECT_DOUBLE_EQ(velocity.y, 0.0);
    EXPECT_TRUE(onGround);
}

TEST(CollisionSystemTest, ResolveCollisions_MultipleAxes) {
    AABB entity(0, 0, 0, 1, 1, 1);
    glm::dvec3 velocity(1, -1, 0);
    std::vector<AABB> obstacles = {
        AABB(0.5, 0.5, 0, 1.5, 1.5, 1),  // X and Y overlap
        AABB(-1, -1, -1, 2, 0, 2)         // ground at y=0
    };
    bool onGround = false;
    bool collided = CollisionSystem::resolveCollisions(entity, velocity, obstacles, onGround);
    EXPECT_TRUE(collided);
    // At least one axis should have been zeroed
    EXPECT_TRUE(velocity.x == 0.0 || velocity.y == 0.0);
}

// ============================================================================
// Ray — Basic Properties
// ============================================================================
TEST(RayTest, DefaultConstructor) {
    Ray r;
    EXPECT_DOUBLE_EQ(r.origin.x, 0.0);
    EXPECT_DOUBLE_EQ(r.origin.y, 0.0);
    EXPECT_DOUBLE_EQ(r.origin.z, 0.0);
    EXPECT_DOUBLE_EQ(r.direction.x, 0.0);
    EXPECT_DOUBLE_EQ(r.direction.y, 0.0);
    EXPECT_DOUBLE_EQ(r.direction.z, 1.0);
}

TEST(RayTest, GetPoint) {
    Ray r(glm::dvec3(1, 2, 3), glm::dvec3(0, 1, 0));
    glm::dvec3 p = r.getPoint(5.0);
    EXPECT_DOUBLE_EQ(p.x, 1.0);
    EXPECT_DOUBLE_EQ(p.y, 7.0);
    EXPECT_DOUBLE_EQ(p.z, 3.0);
}

TEST(RayTest, FromPoints) {
    Ray r = Ray::fromPoints(glm::dvec3(0, 0, 0), glm::dvec3(0, 5, 0));
    EXPECT_DOUBLE_EQ(r.origin.x, 0.0);
    EXPECT_DOUBLE_EQ(r.origin.y, 0.0);
    EXPECT_DOUBLE_EQ(r.origin.z, 0.0);
    EXPECT_NEAR(r.direction.x, 0.0, 1e-9);
    EXPECT_NEAR(r.direction.y, 1.0, 1e-9);
    EXPECT_NEAR(r.direction.z, 0.0, 1e-9);
}

} // namespace
} // namespace Physics
} // namespace Prisma
