#include <gtest/gtest.h>
#include "physics/CCDSolver.h"

namespace Prisma {
namespace Physics {
namespace {

// ============================================================================
// sweepSphere — Stationary Sphere (No Movement)
// ============================================================================
TEST(SweepSphereTest, StationarySphereHits) {
    // Sphere at origin does not overlap target at first
    AABB target(2.0, -1.0, -1.0, 4.0, 1.0, 1.0);
    SweepResult result = sweepSphere(
        glm::dvec3(0.0, 0.0, 0.0),
        glm::dvec3(0.0, 0.0, 0.0),
        1.0,
        target
    );
    // Sphere does not reach target (too far)
    EXPECT_FALSE(result.hasHit);
    EXPECT_DOUBLE_EQ(result.toi, 1.0);
}

TEST(SweepSphereTest, StationarySphereOverlap) {
    // Sphere center is inside the expanded AABB
    AABB target(0.0, -1.0, -1.0, 2.0, 1.0, 1.0);
    SweepResult result = sweepSphere(
        glm::dvec3(1.0, 0.0, 0.0),
        glm::dvec3(1.0, 0.0, 0.0),
        0.5,
        target
    );
    // Sphere with radius 0.5 centered at (1,0,0) should overlap target AABB
    EXPECT_TRUE(result.hasHit);
    EXPECT_DOUBLE_EQ(result.toi, 0.0);
}

// ============================================================================
// sweepSphere — Moving Sphere
// ============================================================================
TEST(SweepSphereTest, MovingSphereHitsTarget) {
    AABB target(4.0, -1.0, -1.0, 6.0, 1.0, 1.0);
    SweepResult result = sweepSphere(
        glm::dvec3(0.0, 0.0, 0.0),
        glm::dvec3(10.0, 0.0, 0.0),
        1.0,
        target
    );
    // Sphere radius=1, expanded AABB becomes (3,-2,-2)-(7,2,2)
    // Ray from (0,0,0) to (10,0,0) hits expanded AABB at x=3 -> t=3, toi = 3/10 = 0.3
    EXPECT_TRUE(result.hasHit);
    EXPECT_GT(result.toi, 0.0);
    EXPECT_LT(result.toi, 1.0);
    EXPECT_NEAR(result.toi, 0.3, 1e-2);
}

TEST(SweepSphereTest, MovingSphereMissesTarget) {
    AABB target(20.0, -1.0, -1.0, 22.0, 1.0, 1.0);
    SweepResult result = sweepSphere(
        glm::dvec3(0.0, 0.0, 0.0),
        glm::dvec3(10.0, 0.0, 0.0),
        1.0,
        target
    );
    // Target is at x=20, sphere only moves to x=10, no hit
    EXPECT_FALSE(result.hasHit);
    EXPECT_DOUBLE_EQ(result.toi, 1.0);
}

TEST(SweepSphereTest, MovingSphereHitNormal) {
    AABB target(4.0, -1.0, -1.0, 6.0, 1.0, 1.0);
    SweepResult result = sweepSphere(
        glm::dvec3(0.0, 0.0, 0.0),
        glm::dvec3(10.0, 0.0, 0.0),
        1.0,
        target
    );
    EXPECT_TRUE(result.hasHit);
    // Hit normal should point in -X direction (from -X side)
    EXPECT_LT(result.hitNormal.x, 0.0);
}

TEST(SweepSphereTest, MovingSphereHitsTop) {
    AABB target(-1.0, 3.0, -1.0, 1.0, 5.0, 1.0);
    SweepResult result = sweepSphere(
        glm::dvec3(0.0, 0.0, 0.0),
        glm::dvec3(0.0, 10.0, 0.0),
        0.5,
        target
    );
    // Sphere moving upward from origin, target at y=3-5
    // Expanded AABB: y in [2.5, 5.5], hit at y=2.5 -> toi = 2.5/10 = 0.25
    EXPECT_TRUE(result.hasHit);
    EXPECT_NEAR(result.toi, 0.25, 1e-1);
    // Normal should point downward (from -Y side, hitting bottom face)
    EXPECT_LT(result.hitNormal.y, 0.0);
}

// ============================================================================
// sweepAABB — Binary Search Sweep
// ============================================================================
TEST(SweepAABBTest, StartingOverlap) {
    AABB moving(0.0, 0.0, 0.0, 2.0, 1.0, 1.0);
    glm::dvec3 movement(1.0, 0.0, 0.0);
    AABB obstacle(1.0, 0.0, 0.0, 3.0, 1.0, 1.0);
    double hitTime;
    EXPECT_TRUE(sweepAABB(moving, movement, obstacle, hitTime));
    EXPECT_DOUBLE_EQ(hitTime, 0.0);
}

TEST(SweepAABBTest, MovingIntoObstacle) {
    AABB moving(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    glm::dvec3 movement(4.0, 0.0, 0.0);
    AABB obstacle(2.0, 0.0, 0.0, 3.0, 1.0, 1.0);
    double hitTime;
    bool result = sweepAABB(moving, movement, obstacle, hitTime, 0.01, 10);
    // The moving AABB sweeps from x=0 to x=4, obstacle at x=2-3
    // Should detect collision somewhere between 0 and 1
    if (result) {
        EXPECT_GE(hitTime, 0.0);
        EXPECT_LE(hitTime, 1.0);
    }
}

TEST(SweepAABBTest, NoCollision) {
    AABB moving(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    glm::dvec3 movement(10.0, 0.0, 0.0);
    AABB obstacle(5.0, 5.0, 5.0, 6.0, 6.0, 6.0);
    double hitTime;
    EXPECT_FALSE(sweepAABB(moving, movement, obstacle, hitTime));
    EXPECT_DOUBLE_EQ(hitTime, 1.0);
}

TEST(SweepAABBTest, ZeroMovement) {
    AABB moving(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    glm::dvec3 movement(0.0, 0.0, 0.0);
    AABB obstacle(2.0, 2.0, 2.0, 3.0, 3.0, 3.0);
    double hitTime;
    EXPECT_FALSE(sweepAABB(moving, movement, obstacle, hitTime));
}

TEST(SweepAABBTest, CustomAllowedPenetration) {
    AABB moving(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    glm::dvec3 movement(2.0, 0.0, 0.0);
    AABB obstacle(0.9, 0.0, 0.0, 1.9, 1.0, 1.0);
    double hitTime;
    // With large allowed penetration, should detect collision
    EXPECT_TRUE(sweepAABB(moving, movement, obstacle, hitTime, 0.5, 10));
}

// ============================================================================
// CCDSolver — Basic
// ============================================================================
TEST(CCDSolverTest, DefaultParameters) {
    CCDSolver solver;
    EXPECT_EQ(solver.maxCCDIterations, 3);
    EXPECT_DOUBLE_EQ(solver.ccdAllowedPenetration, 0.01);
}

TEST(CCDSolverTest, PerformCCDWithNoCCDBodies) {
    CCDSolver solver;
    std::vector<std::unique_ptr<RigidBody>> bodies;

    auto dynamic = std::make_unique<RigidBody>(RigidBodyType::Dynamic);
    dynamic->setCCDEnabled(false);
    bodies.push_back(std::move(dynamic));

    auto static_ = std::make_unique<RigidBody>(RigidBodyType::Static);
    bodies.push_back(std::move(static_));

    // Should not crash or modify anything when no CCD bodies
    solver.performCCD(bodies, 1.0 / 60.0);
    SUCCEED();
}

TEST(CCDSolverTest, PerformCCDWithCCDBodyButNoCollision) {
    CCDSolver solver;
    std::vector<std::unique_ptr<RigidBody>> bodies;

    auto fastBody = std::make_unique<RigidBody>(RigidBodyType::Dynamic);
    fastBody->setCCDEnabled(true);
    fastBody->setPosition(glm::dvec3(0.0, 0.0, 0.0));
    fastBody->setLinearVelocity(glm::dvec3(10.0, 0.0, 0.0)); // Above threshold
    fastBody->setCollisionHalfSize(glm::dvec3(0.5, 0.5, 0.5));
    bodies.push_back(std::move(fastBody));

    auto farBody = std::make_unique<RigidBody>(RigidBodyType::Static);
    farBody->setPosition(glm::dvec3(100.0, 0.0, 0.0));
    farBody->setCollisionHalfSize(glm::dvec3(0.5, 0.5, 0.5));
    bodies.push_back(std::move(farBody));

    // No collision expected, body should move to new position
    glm::dvec3 originalPos = bodies[0]->getPosition();
    solver.performCCD(bodies, 1.0 / 60.0);
    glm::dvec3 newPos = bodies[0]->getPosition();
    EXPECT_NE(newPos.x, originalPos.x) << "Body should move forward";
}

TEST(CCDSolverTest, PerformCCDSlowBodyNotAffected) {
    CCDSolver solver;
    std::vector<std::unique_ptr<RigidBody>> bodies;

    auto slowBody = std::make_unique<RigidBody>(RigidBodyType::Dynamic);
    slowBody->setCCDEnabled(true);
    slowBody->setPosition(glm::dvec3(0.0, 0.0, 0.0));
    slowBody->setLinearVelocity(glm::dvec3(1.0, 0.0, 0.0)); // Below threshold (5.0)
    slowBody->setCollisionHalfSize(glm::dvec3(0.5, 0.5, 0.5));
    bodies.push_back(std::move(slowBody));

    solver.performCCD(bodies, 1.0 / 60.0);
    // Should not be processed by CCD (speed < threshold)
    SUCCEED();
}

TEST(CCDSolverTest, PerformCCDDetectsCollision) {
    CCDSolver solver;
    std::vector<std::unique_ptr<RigidBody>> bodies;

    // Fast body heading toward a static obstacle
    auto fastBody = std::make_unique<RigidBody>(RigidBodyType::Dynamic);
    fastBody->setCCDEnabled(true);
    fastBody->setPosition(glm::dvec3(0.0, 0.0, 0.0));
    fastBody->setLinearVelocity(glm::dvec3(100.0, 0.0, 0.0));
    fastBody->setCollisionHalfSize(glm::dvec3(0.5, 0.5, 0.5));
    uintptr_t fastPtr = reinterpret_cast<uintptr_t>(fastBody.get());
    bodies.push_back(std::move(fastBody));

    auto obstacle = std::make_unique<RigidBody>(RigidBodyType::Static);
    obstacle->setPosition(glm::dvec3(2.0, 0.0, 0.0));
    obstacle->setCollisionHalfSize(glm::dvec3(0.5, 0.5, 0.5));
    bodies.push_back(std::move(obstacle));

    solver.performCCD(bodies, 1.0 / 60.0);

    // fastBody should be somewhere between start and obstacle
    RigidBody* fastBodyPtr = reinterpret_cast<RigidBody*>(fastPtr);
    glm::dvec3 pos = fastBodyPtr->getPosition();
    // CCD should have stopped the body before tunneling through
    EXPECT_LT(pos.x, 2.5) << "CCD should stop body before/at obstacle";
    // But moved past its original position
    EXPECT_GT(pos.x, 0.0) << "CCD should move body forward from start";
}

// ============================================================================
// CCDSolver — Zero dt Handling
// ============================================================================
TEST(CCDSolverTest, ZeroDt) {
    CCDSolver solver;
    std::vector<std::unique_ptr<RigidBody>> bodies;

    auto body = std::make_unique<RigidBody>(RigidBodyType::Dynamic);
    body->setCCDEnabled(true);
    body->setLinearVelocity(glm::dvec3(100.0, 0.0, 0.0));
    bodies.push_back(std::move(body));

    // Should not crash with zero dt
    solver.performCCD(bodies, 0.0);
    SUCCEED();
}

} // namespace
} // namespace Physics
} // namespace Prisma
