#include <gtest/gtest.h>
#include "physics/RigidBody.h"

namespace Prisma {
namespace Physics {
namespace {

// ============================================================================
// RigidBody — Type Defaults
// ============================================================================
TEST(RigidBodyTest, DefaultConstructorIsDynamic) {
    RigidBody body;
    EXPECT_EQ(body.getType(), RigidBodyType::Dynamic);
    EXPECT_TRUE(body.isDynamic());
    EXPECT_FALSE(body.isStatic());
    EXPECT_FALSE(body.isKinematic());
    EXPECT_TRUE(body.isActive());
    EXPECT_TRUE(body.isAwake());
}

TEST(RigidBodyTest, StaticConstructor) {
    RigidBody body(RigidBodyType::Static);
    EXPECT_EQ(body.getType(), RigidBodyType::Static);
    EXPECT_TRUE(body.isStatic());
    EXPECT_FALSE(body.isDynamic());
    EXPECT_FALSE(body.isKinematic());
    EXPECT_TRUE(body.isActive());
    EXPECT_FALSE(body.isAwake());
}

TEST(RigidBodyTest, KinematicConstructor) {
    RigidBody body(RigidBodyType::Kinematic);
    EXPECT_EQ(body.getType(), RigidBodyType::Kinematic);
    EXPECT_FALSE(body.isStatic());
    EXPECT_FALSE(body.isDynamic());
    EXPECT_TRUE(body.isKinematic());
    EXPECT_TRUE(body.isActive());
    EXPECT_TRUE(body.isAwake());
}

TEST(RigidBodyTest, SetType) {
    RigidBody body(RigidBodyType::Dynamic);
    body.setType(RigidBodyType::Static);
    EXPECT_TRUE(body.isStatic());
    body.setType(RigidBodyType::Kinematic);
    EXPECT_TRUE(body.isKinematic());
}

// ============================================================================
// RigidBody — Mass
// ============================================================================
TEST(RigidBodyTest, DefaultMass) {
    RigidBody body;
    EXPECT_DOUBLE_EQ(body.getMass(), 1.0);
    EXPECT_DOUBLE_EQ(body.getInverseMass(), 1.0);
}

TEST(RigidBodyTest, SetMassPositive) {
    RigidBody body;
    body.setMass(5.0);
    EXPECT_DOUBLE_EQ(body.getMass(), 5.0);
    EXPECT_DOUBLE_EQ(body.getInverseMass(), 0.2);
}

TEST(RigidBodyTest, SetMassZero) {
    RigidBody body;
    body.setMass(0.0);
    EXPECT_DOUBLE_EQ(body.getMass(), 0.0);
    EXPECT_DOUBLE_EQ(body.getInverseMass(), 0.0);
    EXPECT_TRUE(body.hasInfiniteMass());
}

TEST(RigidBodyTest, SetMassNegative) {
    RigidBody body;
    body.setMass(-1.0);
    EXPECT_DOUBLE_EQ(body.getMass(), 0.0);
    EXPECT_DOUBLE_EQ(body.getInverseMass(), 0.0);
    EXPECT_TRUE(body.hasInfiniteMass());
}

TEST(RigidBodyTest, SetMassMakesStatic) {
    RigidBody body;
    EXPECT_TRUE(body.isDynamic());
    body.setMass(0.0);
    EXPECT_TRUE(body.isStatic());
}

// ============================================================================
// RigidBody — Position
// ============================================================================
TEST(RigidBodyTest, DefaultPosition) {
    RigidBody body;
    glm::dvec3 pos = body.getPosition();
    EXPECT_DOUBLE_EQ(pos.x, 0.0);
    EXPECT_DOUBLE_EQ(pos.y, 0.0);
    EXPECT_DOUBLE_EQ(pos.z, 0.0);
}

TEST(RigidBodyTest, SetPosition) {
    RigidBody body;
    body.setPosition(glm::dvec3(10.0, 20.0, 30.0));
    glm::dvec3 pos = body.getPosition();
    EXPECT_DOUBLE_EQ(pos.x, 10.0);
    EXPECT_DOUBLE_EQ(pos.y, 20.0);
    EXPECT_DOUBLE_EQ(pos.z, 30.0);
}

// ============================================================================
// RigidBody — Velocity
// ============================================================================
TEST(RigidBodyTest, DefaultVelocity) {
    RigidBody body;
    glm::dvec3 vel = body.getLinearVelocity();
    EXPECT_DOUBLE_EQ(vel.x, 0.0);
    EXPECT_DOUBLE_EQ(vel.y, 0.0);
    EXPECT_DOUBLE_EQ(vel.z, 0.0);
}

TEST(RigidBodyTest, SetLinearVelocityDynamic) {
    RigidBody body;
    body.setLinearVelocity(glm::dvec3(5.0, 0.0, 0.0));
    glm::dvec3 vel = body.getLinearVelocity();
    EXPECT_DOUBLE_EQ(vel.x, 5.0);
}

TEST(RigidBodyTest, SetLinearVelocityStaticIgnored) {
    RigidBody body(RigidBodyType::Static);
    body.setLinearVelocity(glm::dvec3(5.0, 0.0, 0.0));
    glm::dvec3 vel = body.getLinearVelocity();
    EXPECT_DOUBLE_EQ(vel.x, 0.0); // Static bodies ignore velocity
}

TEST(RigidBodyTest, SetAngularVelocity) {
    RigidBody body;
    body.setAngularVelocity(glm::dvec3(0.0, 1.0, 0.0));
    glm::dvec3 angVel = body.getAngularVelocity();
    EXPECT_DOUBLE_EQ(angVel.y, 1.0);
}

// ============================================================================
// RigidBody — Damping
// ============================================================================
TEST(RigidBodyTest, DefaultDamping) {
    RigidBody body;
    EXPECT_DOUBLE_EQ(body.getLinearDamping(), 0.01);
    EXPECT_DOUBLE_EQ(body.getAngularDamping(), 0.05);
}

TEST(RigidBodyTest, SetDamping) {
    RigidBody body;
    body.setLinearDamping(0.1);
    body.setAngularDamping(0.2);
    EXPECT_DOUBLE_EQ(body.getLinearDamping(), 0.1);
    EXPECT_DOUBLE_EQ(body.getAngularDamping(), 0.2);
}

// ============================================================================
// RigidBody — Apply Impulse
// ============================================================================
TEST(RigidBodyTest, ApplyImpulse) {
    RigidBody body;
    body.applyImpulse(glm::dvec3(10.0, 0.0, 0.0));
    // impulse / mass = 10.0 / 1.0 = 10.0
    glm::dvec3 vel = body.getLinearVelocity();
    EXPECT_DOUBLE_EQ(vel.x, 10.0);
}

TEST(RigidBodyTest, ApplyImpulseHeavierMass) {
    RigidBody body;
    body.setMass(2.0);
    body.applyImpulse(glm::dvec3(10.0, 0.0, 0.0));
    // impulse / mass = 10.0 / 2.0 = 5.0
    glm::dvec3 vel = body.getLinearVelocity();
    EXPECT_DOUBLE_EQ(vel.x, 5.0);
}

TEST(RigidBodyTest, ApplyImpulseStaticIgnored) {
    RigidBody body(RigidBodyType::Static);
    body.applyImpulse(glm::dvec3(10.0, 0.0, 0.0));
    glm::dvec3 vel = body.getLinearVelocity();
    EXPECT_DOUBLE_EQ(vel.x, 0.0); // Static bodies ignore impulse
}

// ============================================================================
// RigidBody — Apply Force
// ============================================================================
TEST(RigidBodyTest, ApplyForce) {
    RigidBody body;
    body.applyForce(glm::dvec3(10.0, 0.0, 0.0));
    glm::dvec3 force = body.getAccumulatedForce();
    EXPECT_DOUBLE_EQ(force.x, 10.0);
}

TEST(RigidBodyTest, ApplyForceMultiple) {
    RigidBody body;
    body.applyForce(glm::dvec3(5.0, 0.0, 0.0));
    body.applyForce(glm::dvec3(3.0, 0.0, 0.0));
    glm::dvec3 force = body.getAccumulatedForce();
    EXPECT_DOUBLE_EQ(force.x, 8.0);
}

TEST(RigidBodyTest, ApplyForceStaticIgnored) {
    RigidBody body(RigidBodyType::Static);
    body.applyForce(glm::dvec3(10.0, 0.0, 0.0));
    glm::dvec3 force = body.getAccumulatedForce();
    EXPECT_DOUBLE_EQ(force.x, 0.0); // Static bodies ignore force
}

// ============================================================================
// RigidBody — Clear Forces
// ============================================================================
TEST(RigidBodyTest, ClearForces) {
    RigidBody body;
    body.applyForce(glm::dvec3(10.0, 5.0, 3.0));
    body.applyTorque(glm::dvec3(1.0, 0.0, 0.0));
    body.clearForces();
    glm::dvec3 force = body.getAccumulatedForce();
    glm::dvec3 torque = body.getAccumulatedTorque();
    EXPECT_DOUBLE_EQ(force.x, 0.0);
    EXPECT_DOUBLE_EQ(force.y, 0.0);
    EXPECT_DOUBLE_EQ(force.z, 0.0);
    EXPECT_DOUBLE_EQ(torque.x, 0.0);
    EXPECT_DOUBLE_EQ(torque.y, 0.0);
    EXPECT_DOUBLE_EQ(torque.z, 0.0);
}

// ============================================================================
// RigidBody — Apply Damping
// ============================================================================
TEST(RigidBodyTest, ApplyDampingReducesVelocity) {
    RigidBody body;
    body.setLinearDamping(0.5);
    body.setLinearVelocity(glm::dvec3(10.0, 0.0, 0.0));
    body.applyDamping(0.1); // dt=0.1, factor = 1 - 0.5*0.1 = 0.95
    glm::dvec3 vel = body.getLinearVelocity();
    EXPECT_NEAR(vel.x, 9.5, 1e-9);
}

// ============================================================================
// RigidBody — Wake/Sleep
// ============================================================================
TEST(RigidBodyTest, SleepAndWake) {
    RigidBody body;
    EXPECT_TRUE(body.isAwake());
    body.sleep();
    EXPECT_FALSE(body.isAwake());
    body.wakeUp();
    EXPECT_TRUE(body.isAwake());
}

TEST(RigidBodyTest, StaticBodyNeverAwake) {
    RigidBody body(RigidBodyType::Static);
    EXPECT_FALSE(body.isAwake());
    body.wakeUp();
    EXPECT_FALSE(body.isAwake()); // Static remains not awaked
}

// ============================================================================
// RigidBody — Collision Half Size
// ============================================================================
TEST(RigidBodyTest, DefaultCollisionHalfSize) {
    RigidBody body;
    glm::dvec3 halfSize = body.getCollisionHalfSize();
    EXPECT_DOUBLE_EQ(halfSize.x, 0.5);
    EXPECT_DOUBLE_EQ(halfSize.y, 0.5);
    EXPECT_DOUBLE_EQ(halfSize.z, 0.5);
}

TEST(RigidBodyTest, SetCollisionHalfSize) {
    RigidBody body;
    body.setCollisionHalfSize(glm::dvec3(1.0, 2.0, 3.0));
    glm::dvec3 halfSize = body.getCollisionHalfSize();
    EXPECT_DOUBLE_EQ(halfSize.x, 1.0);
    EXPECT_DOUBLE_EQ(halfSize.y, 2.0);
    EXPECT_DOUBLE_EQ(halfSize.z, 3.0);
}

// ============================================================================
// RigidBody — World AABB
// ============================================================================
TEST(RigidBodyTest, GetWorldAABBAtOrigin) {
    RigidBody body;
    body.setCollisionHalfSize(glm::dvec3(0.5, 0.5, 0.5));
    AABB aabb = body.getWorldAABB();
    EXPECT_DOUBLE_EQ(aabb.minX, -0.5);
    EXPECT_DOUBLE_EQ(aabb.minY, -0.5);
    EXPECT_DOUBLE_EQ(aabb.minZ, -0.5);
    EXPECT_DOUBLE_EQ(aabb.maxX,  0.5);
    EXPECT_DOUBLE_EQ(aabb.maxY,  0.5);
    EXPECT_DOUBLE_EQ(aabb.maxZ,  0.5);
}

TEST(RigidBodyTest, GetWorldAABBAtPosition) {
    RigidBody body;
    body.setPosition(glm::dvec3(10.0, 20.0, 30.0));
    body.setCollisionHalfSize(glm::dvec3(1.0, 1.0, 1.0));
    AABB aabb = body.getWorldAABB();
    EXPECT_DOUBLE_EQ(aabb.minX, 9.0);
    EXPECT_DOUBLE_EQ(aabb.minY, 19.0);
    EXPECT_DOUBLE_EQ(aabb.minZ, 29.0);
    EXPECT_DOUBLE_EQ(aabb.maxX, 11.0);
    EXPECT_DOUBLE_EQ(aabb.maxY, 21.0);
    EXPECT_DOUBLE_EQ(aabb.maxZ, 31.0);
}

// ============================================================================
// RigidBody — CCD
// ============================================================================
TEST(RigidBodyTest, CCDDisabledByDefault) {
    RigidBody body;
    EXPECT_FALSE(body.isCCDEnabled());
}

TEST(RigidBodyTest, SetCCDEnabled) {
    RigidBody body;
    body.setCCDEnabled(true);
    EXPECT_TRUE(body.isCCDEnabled());
}

TEST(RigidBodyTest, CCDMotionThresholdDefault) {
    RigidBody body;
    EXPECT_DOUBLE_EQ(body.getCcdMotionThreshold(), 5.0);
}

TEST(RigidBodyTest, SetCCDMotionThreshold) {
    RigidBody body;
    body.setCcdMotionThreshold(10.0);
    EXPECT_DOUBLE_EQ(body.getCcdMotionThreshold(), 10.0);
}

// ============================================================================
// RigidBody — User Data
// ============================================================================
TEST(RigidBodyTest, UserData) {
    RigidBody body;
    int data = 42;
    body.setUserData(&data);
    EXPECT_EQ(*(static_cast<int*>(body.getUserData())), 42);
}

TEST(RigidBodyTest, DefaultUserDataNull) {
    RigidBody body;
    EXPECT_EQ(body.getUserData(), nullptr);
}

// ============================================================================
// RigidBody — Collision Flags
// ============================================================================
TEST(RigidBodyTest, DefaultCollisionFlagsDynamic) {
    RigidBody body;
    EXPECT_EQ(body.getCollisionFlags(), CollisionFlags::Dynamic);
}

TEST(RigidBodyTest, SetCollisionFlags) {
    RigidBody body;
    body.setCollisionFlags(CollisionFlags::Static | CollisionFlags::Kinematic);
    EXPECT_EQ(body.getCollisionFlags(), CollisionFlags::Static | CollisionFlags::Kinematic);
}

// ============================================================================
// RigidBody — Rotation
// ============================================================================
TEST(RigidBodyTest, DefaultRotation) {
    RigidBody body;
    glm::dquat rot = body.getRotation();
    EXPECT_DOUBLE_EQ(rot.x, 0.0);
    EXPECT_DOUBLE_EQ(rot.y, 0.0);
    EXPECT_DOUBLE_EQ(rot.z, 0.0);
    EXPECT_DOUBLE_EQ(rot.w, 1.0);
}

TEST(RigidBodyTest, SetRotation) {
    RigidBody body;
    glm::dquat q = glm::angleAxis(glm::radians(90.0), glm::dvec3(0.0, 1.0, 0.0));
    body.setRotation(q);
    glm::dquat rot = body.getRotation();
    EXPECT_NEAR(rot.x, q.x, 1e-9);
    EXPECT_NEAR(rot.y, q.y, 1e-9);
    EXPECT_NEAR(rot.z, q.z, 1e-9);
    EXPECT_NEAR(rot.w, q.w, 1e-9);
}

// ============================================================================
// RigidBody — Scale
// ============================================================================
TEST(RigidBodyTest, DefaultScale) {
    RigidBody body;
    glm::dvec3 scale = body.getScale();
    EXPECT_DOUBLE_EQ(scale.x, 1.0);
    EXPECT_DOUBLE_EQ(scale.y, 1.0);
    EXPECT_DOUBLE_EQ(scale.z, 1.0);
}

TEST(RigidBodyTest, SetScale) {
    RigidBody body;
    body.setScale(glm::dvec3(2.0, 3.0, 4.0));
    glm::dvec3 scale = body.getScale();
    EXPECT_DOUBLE_EQ(scale.x, 2.0);
    EXPECT_DOUBLE_EQ(scale.y, 3.0);
    EXPECT_DOUBLE_EQ(scale.z, 4.0);
}

// ============================================================================
// RigidBody — Transform Matrix
// ============================================================================
TEST(RigidBodyTest, TransformMatrixAtOrigin) {
    RigidBody body;
    glm::dmat4 mat = body.getTransformMatrix();
    EXPECT_DOUBLE_EQ(mat[3][0], 0.0);
    EXPECT_DOUBLE_EQ(mat[3][1], 0.0);
    EXPECT_DOUBLE_EQ(mat[3][2], 0.0);
    EXPECT_DOUBLE_EQ(mat[0][0], 1.0);
    EXPECT_DOUBLE_EQ(mat[1][1], 1.0);
    EXPECT_DOUBLE_EQ(mat[2][2], 1.0);
}

TEST(RigidBodyTest, TransformMatrixWithPosition) {
    RigidBody body;
    body.setPosition(glm::dvec3(10.0, 20.0, 30.0));
    glm::dmat4 mat = body.getTransformMatrix();
    EXPECT_DOUBLE_EQ(mat[3][0], 10.0);
    EXPECT_DOUBLE_EQ(mat[3][1], 20.0);
    EXPECT_DOUBLE_EQ(mat[3][2], 30.0);
}

} // namespace
} // namespace Physics
} // namespace Prisma
