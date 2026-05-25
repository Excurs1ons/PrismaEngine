#include "physics/RigidBody.h"
#include <algorithm>
#include <cmath>

namespace Prisma {
namespace Physics {

// 睡眠阈值常量
static constexpr double SLEEP_LINEAR_EPSILON = 0.01;    // 1 cm/s
static constexpr double SLEEP_ANGULAR_EPSILON = 0.05;   // ~3 deg/s
static constexpr double SLEEP_TIME_THRESHOLD = 0.5;     // 0.5 seconds

RigidBody::RigidBody()
    : m_type(RigidBodyType::Dynamic)
    , m_collisionFlags(CollisionFlags::Dynamic)
    , m_isActive(true)
    , m_isAwake(true)
    , m_sleepTime(0.0)
    , m_position(0.0)
    , m_rotation(1.0, 0.0, 0.0, 0.0)
    , m_scale(1.0)
    , m_linearVelocity(0.0)
    , m_angularVelocity(0.0)
    , m_mass(1.0)
    , m_inverseMass(1.0)
    , m_inertiaTensor(1.0)
    , m_inverseInertiaTensorWorld(1.0)
    , m_linearDamping(0.01)
    , m_angularDamping(0.05)
    , m_accumulatedForce(0.0)
    , m_accumulatedTorque(0.0)
    , m_collisionHalfSize(0.5)
    , m_userData(nullptr)
{
}

RigidBody::RigidBody(RigidBodyType type)
    : m_type(type)
    , m_collisionFlags(
          type == RigidBodyType::Static ? CollisionFlags::Static :
          type == RigidBodyType::Kinematic ? CollisionFlags::Kinematic : CollisionFlags::Dynamic)
    , m_isActive(true)
    , m_isAwake(type != RigidBodyType::Static)
    , m_sleepTime(0.0)
    , m_position(0.0)
    , m_rotation(1.0, 0.0, 0.0, 0.0)
    , m_scale(1.0)
    , m_linearVelocity(0.0)
    , m_angularVelocity(0.0)
    , m_mass(type == RigidBodyType::Static ? 0.0 : 1.0)
    , m_inverseMass(type == RigidBodyType::Static ? 0.0 : 1.0)
    , m_inertiaTensor(1.0)
    , m_inverseInertiaTensorWorld(1.0)
    , m_linearDamping(0.01)
    , m_angularDamping(0.05)
    , m_accumulatedForce(0.0)
    , m_accumulatedTorque(0.0)
    , m_collisionHalfSize(0.5)
    , m_userData(nullptr)
{
}

void RigidBody::setMass(double mass) {
    if (mass <= 0.0) {
        m_mass = 0.0;
        m_inverseMass = 0.0;
        m_type = RigidBodyType::Static;
    } else {
        m_mass = mass;
        m_inverseMass = 1.0 / mass;
    }
}

void RigidBody::setInertiaTensor(const glm::dmat3& tensor) {
    m_inertiaTensor = tensor;
    updateInertiaTensorWorld();
}

void RigidBody::updateInertiaTensorWorld() {
    glm::dmat3 rotMatrix = glm::mat4_cast(m_rotation);
    glm::dmat3 rotMatrixT = glm::transpose(rotMatrix);
    // I_world = R * I_body * R^T
    glm::dmat3 inertiaWorld = rotMatrix * m_inertiaTensor * rotMatrixT;
    m_inverseInertiaTensorWorld = glm::inverse(inertiaWorld);
}

void RigidBody::setAwake(bool awake) {
    if (m_type == RigidBodyType::Static) {
        // Static bodies never sleep or wake
        m_isAwake = false;
        return;
    }
    m_isAwake = awake;
    if (!awake) {
        m_linearVelocity = glm::dvec3(0.0);
        m_angularVelocity = glm::dvec3(0.0);
        m_accumulatedForce = glm::dvec3(0.0);
        m_accumulatedTorque = glm::dvec3(0.0);
        m_sleepTime = 0.0;
    }
}

void RigidBody::applyForce(const glm::dvec3& force) {
    if (m_type == RigidBodyType::Static || !m_isActive) return;
    m_accumulatedForce += force;
    if (!m_isAwake) wakeUp();
}

void RigidBody::applyForceAtPoint(const glm::dvec3& force, const glm::dvec3& point) {
    if (m_type == RigidBodyType::Static || !m_isActive) return;
    m_accumulatedForce += force;
    glm::dvec3 torque = glm::cross(point - m_position, force);
    m_accumulatedTorque += torque;
    if (!m_isAwake) wakeUp();
}

void RigidBody::applyTorque(const glm::dvec3& torque) {
    if (m_type == RigidBodyType::Static || !m_isActive) return;
    m_accumulatedTorque += torque;
    if (!m_isAwake) wakeUp();
}

void RigidBody::applyImpulse(const glm::dvec3& impulse) {
    if (m_type == RigidBodyType::Static || !m_isActive) return;
    m_linearVelocity += impulse * m_inverseMass;
    if (!m_isAwake) wakeUp();
}

void RigidBody::applyImpulseAtPoint(const glm::dvec3& impulse, const glm::dvec3& point) {
    if (m_type == RigidBodyType::Static || !m_isActive) return;
    m_linearVelocity += impulse * m_inverseMass;
    glm::dvec3 torqueImpulse = glm::cross(point - m_position, impulse);
    m_angularVelocity += m_inverseInertiaTensorWorld * torqueImpulse;
    if (!m_isAwake) wakeUp();
}

void RigidBody::clearForces() {
    m_accumulatedForce = glm::dvec3(0.0);
    m_accumulatedTorque = glm::dvec3(0.0);
}

void RigidBody::applyDamping(double ts) {
    if (m_type == RigidBodyType::Static || !m_isAwake) return;

    // Exponential damping: v *= (1 - damping) ^ dt
    double linearFactor = 1.0 - m_linearDamping * ts;
    if (linearFactor < 0.0) linearFactor = 0.0;
    m_linearVelocity *= linearFactor;

    double angularFactor = 1.0 - m_angularDamping * ts;
    if (angularFactor < 0.0) angularFactor = 0.0;
    m_angularVelocity *= angularFactor;
}

void RigidBody::integrate(double ts) {
    if (m_type == RigidBodyType::Static || !m_isAwake) return;

    // Store previous motion state for interpolation
    m_motionState.storeCurrentState();

    // Semi-implicit Euler: update velocity first, then position

    // Linear acceleration: a = F / m
    glm::dvec3 acceleration = m_accumulatedForce * m_inverseMass;
    // Apply gravity (handled externally through applyForce)
    m_linearVelocity += acceleration * ts;

    // Angular acceleration: alpha = I^{-1} * tau
    m_angularVelocity += m_inverseInertiaTensorWorld * m_accumulatedTorque * ts;

    // Update position
    m_position += m_linearVelocity * ts;

    // Update orientation: dq/dt = 0.5 * omega * q
    glm::dquat omegaQuat(0.0, m_angularVelocity.x, m_angularVelocity.y, m_angularVelocity.z);
    m_rotation += ts * 0.5 * omegaQuat * m_rotation;
    m_rotation = glm::normalize(m_rotation);

    // Update world-space inertia tensor
    updateInertiaTensorWorld();

    // Sleep detection
    double linVelSq = glm::dot(m_linearVelocity, m_linearVelocity);
    double angVelSq = glm::dot(m_angularVelocity, m_angularVelocity);
    if (linVelSq < SLEEP_LINEAR_EPSILON && angVelSq < SLEEP_ANGULAR_EPSILON) {
        m_sleepTime += ts;
        if (m_sleepTime > SLEEP_TIME_THRESHOLD) {
            setAwake(false);
            return;
        }
    } else {
        m_sleepTime = 0.0;
    }

    // Clear forces for next timestep
    m_accumulatedForce = glm::dvec3(0.0);
    m_accumulatedTorque = glm::dvec3(0.0);

    // Update motion state with new position
    m_motionState.setCurrentState(m_position, m_rotation);
}

} // namespace Physics
} // namespace Prisma
