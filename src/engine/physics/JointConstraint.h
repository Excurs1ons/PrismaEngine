#pragma once

#include "physics/Constraint.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace Prisma {
namespace Physics {

// B = {position, normal} constraint matrix for sequential impulse formulation
// Each joint type implements Solve() which:
//   1. Computes position error (violation of constraint)
//   2. Computes Jacobian matrix J
//   3. Computes effective mass: (J * M^{-1} * J^T)^{-1}
//   4. Computes impulse: -effectiveMass * (J * v + bias / dt)
//   5. Applies impulse to both bodies

// ── Ball Joint (Point-to-Point Constraint) ──
// Constrains body B's anchor point to body A's anchor point.
// Removes 3 degrees of freedom (translation only).
class BallJoint : public IConstraint {
public:
    RigidBody* m_bodyA;
    RigidBody* m_bodyB;
    glm::dvec3 m_anchorA;     // 锚点在 A 的局部空间
    glm::dvec3 m_anchorB;     // 锚点在 B 的局部空间
    double m_biasFactor = 0.2; // Baumgarte 稳定系数

    BallJoint(RigidBody* bodyA, RigidBody* bodyB,
              const glm::dvec3& anchorA, const glm::dvec3& anchorB)
        : m_bodyA(bodyA), m_bodyB(bodyB), m_anchorA(anchorA), m_anchorB(anchorB) {}

    double Solve(double dt) override {
        if (!m_bodyA || !m_bodyB) return 0.0;

        // 计算世界空间锚点
        glm::dvec3 worldAnchorA = m_bodyA->getRotation() * m_anchorA + m_bodyA->getPosition();
        glm::dvec3 worldAnchorB = m_bodyB->getRotation() * m_anchorB + m_bodyB->getPosition();

        // 位置误差
        glm::dvec3 error = worldAnchorB - worldAnchorA;

        // 计算每个体的速度在锚点的贡献
        glm::dvec3 rA = worldAnchorA - m_bodyA->getPosition();
        glm::dvec3 rB = worldAnchorB - m_bodyB->getPosition();

        // Jacobian: J = [-I, -skew(rA), I, skew(rB)]
        // 有效质量: effectiveMass = (1/mA + 1/mB + (rA x n)^T * I_A^{-1} * (rA x n) + (rB x n)^T * I_B^{-1} * (rB x n))
        // 简化：对于球约束，我们计算沿误差方向的约束

        glm::dvec3 normal = error;
        double errorLen = glm::length(normal);
        if (errorLen < 1e-8) return 0.0;
        normal /= errorLen;

        // 计算有效质量 (scalar)
        glm::dvec3 rAxN = glm::cross(rA, normal);
        glm::dvec3 rBxN = glm::cross(rB, normal);

        double invMassA = m_bodyA->getInverseMass();
        double invMassB = m_bodyB->getInverseMass();
        const glm::dmat3& invIA = m_bodyA->getInverseInertiaTensorWorld();
        const glm::dmat3& invIB = m_bodyB->getInverseInertiaTensorWorld();

        double effectiveMass = invMassA + invMassB;
        effectiveMass += glm::dot(rAxN, invIA * rAxN);
        effectiveMass += glm::dot(rBxN, invIB * rBxN);

        if (effectiveMass < 1e-10) return 0.0;
        effectiveMass = 1.0 / effectiveMass;

        // 相对速度沿约束方向
        glm::dvec3 velA = m_bodyA->getLinearVelocity() + glm::cross(m_bodyA->getAngularVelocity(), rA);
        glm::dvec3 velB = m_bodyB->getLinearVelocity() + glm::cross(m_bodyB->getAngularVelocity(), rB);
        glm::dvec3 relVel = velB - velA;
        double velAlongNormal = glm::dot(relVel, normal);

        // Baumgarte 稳定：将位置误差转化为速度修正
        double bias = m_biasFactor * errorLen / std::max(dt, 1e-8);

        // 计算脉冲标量
        double impulse = effectiveMass * (-velAlongNormal - bias);

        // 应用脉冲
        glm::dvec3 impulseVec = normal * impulse;
        m_bodyA->setLinearVelocity(m_bodyA->getLinearVelocity() - impulseVec * invMassA);
        m_bodyA->setAngularVelocity(m_bodyA->getAngularVelocity() - invIA * rAxN * impulse);
        m_bodyB->setLinearVelocity(m_bodyB->getLinearVelocity() + impulseVec * invMassB);
        m_bodyB->setAngularVelocity(m_bodyB->getAngularVelocity() + invIB * rBxN * impulse);

        return std::abs(impulse);
    }

    void DrawDebug() override {}

    RigidBody* GetBodyA() const override { return m_bodyA; }
    RigidBody* GetBodyB() const override { return m_bodyB; }
};

// ── Hinge Joint ──
// Like ball joint but allows rotation around a single axis.
// Removes 5 DOF, leaves 1 rotational DOF.
class HingeJoint : public IConstraint {
public:
    RigidBody* m_bodyA;
    RigidBody* m_bodyB;
    glm::dvec3 m_anchorA;
    glm::dvec3 m_anchorB;
    glm::dvec3 m_axisA;        // 铰链轴在 A 的局部空间
    glm::dvec3 m_axisB;        // 铰链轴在 B 的局部空间
    double m_biasFactor = 0.2;

    HingeJoint(RigidBody* bodyA, RigidBody* bodyB,
               const glm::dvec3& anchorA, const glm::dvec3& anchorB,
               const glm::dvec3& axisA, const glm::dvec3& axisB)
        : m_bodyA(bodyA), m_bodyB(bodyB)
        , m_anchorA(anchorA), m_anchorB(anchorB)
        , m_axisA(glm::normalize(axisA)), m_axisB(glm::normalize(axisB)) {}

    double Solve(double dt) override {
        if (!m_bodyA || !m_bodyB) return 0.0;

        // Part 1: 球约束（与 BallJoint 相同的点对点约束）
        glm::dvec3 worldAnchorA = m_bodyA->getRotation() * m_anchorA + m_bodyA->getPosition();
        glm::dvec3 worldAnchorB = m_bodyB->getRotation() * m_anchorB + m_bodyB->getPosition();

        glm::dvec3 error = worldAnchorB - worldAnchorA;
        glm::dvec3 normal = error;
        double errorLen = glm::length(normal);
        if (errorLen > 1e-8) {
            normal /= errorLen;

            glm::dvec3 rA = worldAnchorA - m_bodyA->getPosition();
            glm::dvec3 rB = worldAnchorB - m_bodyB->getPosition();
            glm::dvec3 rAxN = glm::cross(rA, normal);
            glm::dvec3 rBxN = glm::cross(rB, normal);

            double invMassA = m_bodyA->getInverseMass();
            double invMassB = m_bodyB->getInverseMass();
            const glm::dmat3& invIA = m_bodyA->getInverseInertiaTensorWorld();
            const glm::dmat3& invIB = m_bodyB->getInverseInertiaTensorWorld();

            double effectiveMass = invMassA + invMassB;
            effectiveMass += glm::dot(rAxN, invIA * rAxN);
            effectiveMass += glm::dot(rBxN, invIB * rBxN);

            if (effectiveMass > 1e-10) {
                effectiveMass = 1.0 / effectiveMass;
                glm::dvec3 velA = m_bodyA->getLinearVelocity() + glm::cross(m_bodyA->getAngularVelocity(), rA);
                glm::dvec3 velB = m_bodyB->getLinearVelocity() + glm::cross(m_bodyB->getAngularVelocity(), rB);
                double velAlongNormal = glm::dot(velB - velA, normal);
                double bias = m_biasFactor * errorLen / std::max(dt, 1e-8);
                double impulse = effectiveMass * (-velAlongNormal - bias);

                m_bodyA->setLinearVelocity(m_bodyA->getLinearVelocity() - normal * impulse * invMassA);
                m_bodyA->setAngularVelocity(m_bodyA->getAngularVelocity() - invIA * rAxN * impulse);
                m_bodyB->setLinearVelocity(m_bodyB->getLinearVelocity() + normal * impulse * invMassB);
                m_bodyB->setAngularVelocity(m_bodyB->getAngularVelocity() + invIB * rBxN * impulse);
            }
        }

        // Part 2: 限制角轴对齐（约束两个体的铰链轴对齐）
        glm::dvec3 worldAxisA = glm::normalize(m_bodyA->getRotation() * m_axisA);
        glm::dvec3 worldAxisB = glm::normalize(m_bodyB->getRotation() * m_axisB);

        // 计算需要约束的垂直方向偏差
        glm::dvec3 axisError = glm::cross(worldAxisA, worldAxisB);
        double axisErrorLen = glm::length(axisError);
        if (axisErrorLen < 1e-8) return 0.0;

        // 对两个垂直轴施加约束脉冲
        glm::dvec3 perpAxis1 = glm::normalize(axisError);
        glm::dvec3 perpAxis2 = glm::normalize(glm::cross(worldAxisA, perpAxis1));

        double totalImpulse = 0.0;
        for (const auto& perpAxis : { perpAxis1, perpAxis2 }) {
            const glm::dmat3& invIA = m_bodyA->getInverseInertiaTensorWorld();
            const glm::dmat3& invIB = m_bodyB->getInverseInertiaTensorWorld();

            // Angular constraint effective mass
            glm::dvec3 iA_perp = invIA * perpAxis;
            glm::dvec3 iB_perp = invIB * perpAxis;
            double angMass = glm::dot(perpAxis, iA_perp) + glm::dot(perpAxis, iB_perp);
            if (angMass < 1e-10) continue;
            angMass = 1.0 / angMass;

            double angVelAlong = glm::dot(m_bodyB->getAngularVelocity() - m_bodyA->getAngularVelocity(), perpAxis);
            double angBias = m_biasFactor * axisErrorLen / std::max(dt, 1e-8);
            double impulse = angMass * (-angVelAlong - angBias);

            m_bodyA->setAngularVelocity(m_bodyA->getAngularVelocity() - iA_perp * impulse);
            m_bodyB->setAngularVelocity(m_bodyB->getAngularVelocity() + iB_perp * impulse);
            totalImpulse += std::abs(impulse);
        }

        return totalImpulse;
    }

    void DrawDebug() override {}

    RigidBody* GetBodyA() const override { return m_bodyA; }
    RigidBody* GetBodyB() const override { return m_bodyB; }
};

// ── Fixed Joint ──
// Completely eliminates relative motion between two bodies.
// Removes all 6 DOF.
class FixedJoint : public IConstraint {
public:
    RigidBody* m_bodyA;
    RigidBody* m_bodyB;
    glm::dvec3 m_anchorA;
    glm::dvec3 m_anchorB;
    glm::dquat m_initialRotDiff; // 初始相对旋转
    double m_biasFactor = 0.2;

    FixedJoint(RigidBody* bodyA, RigidBody* bodyB,
               const glm::dvec3& anchorA, const glm::dvec3& anchorB)
        : m_bodyA(bodyA), m_bodyB(bodyB)
        , m_anchorA(anchorA), m_anchorB(anchorB)
        , m_initialRotDiff(glm::inverse(bodyA->getRotation()) * bodyB->getRotation()) {}

    double Solve(double dt) override {
        if (!m_bodyA || !m_bodyB) return 0.0;

        // Part 1: Point constraint (same as BallJoint)
        glm::dvec3 worldAnchorA = m_bodyA->getRotation() * m_anchorA + m_bodyA->getPosition();
        glm::dvec3 worldAnchorB = m_bodyB->getRotation() * m_anchorB + m_bodyB->getPosition();

        glm::dvec3 error = worldAnchorB - worldAnchorA;
        glm::dvec3 normal = error;
        double errorLen = glm::length(normal);
        double totalImpulse = 0.0;

        if (errorLen > 1e-8) {
            normal /= errorLen;

            glm::dvec3 rA = worldAnchorA - m_bodyA->getPosition();
            glm::dvec3 rB = worldAnchorB - m_bodyB->getPosition();
            glm::dvec3 rAxN = glm::cross(rA, normal);
            glm::dvec3 rBxN = glm::cross(rB, normal);

            double invMassA = m_bodyA->getInverseMass();
            double invMassB = m_bodyB->getInverseMass();
            const glm::dmat3& invIA = m_bodyA->getInverseInertiaTensorWorld();
            const glm::dmat3& invIB = m_bodyB->getInverseInertiaTensorWorld();

            double effectiveMass = invMassA + invMassB;
            effectiveMass += glm::dot(rAxN, invIA * rAxN);
            effectiveMass += glm::dot(rBxN, invIB * rBxN);

            if (effectiveMass > 1e-10) {
                effectiveMass = 1.0 / effectiveMass;
                glm::dvec3 velA = m_bodyA->getLinearVelocity() + glm::cross(m_bodyA->getAngularVelocity(), rA);
                glm::dvec3 velB = m_bodyB->getLinearVelocity() + glm::cross(m_bodyB->getAngularVelocity(), rB);
                double velAlongNormal = glm::dot(velB - velA, normal);
                double bias = m_biasFactor * errorLen / std::max(dt, 1e-8);
                double impulse = effectiveMass * (-velAlongNormal - bias);

                m_bodyA->setLinearVelocity(m_bodyA->getLinearVelocity() - normal * impulse * invMassA);
                m_bodyA->setAngularVelocity(m_bodyA->getAngularVelocity() - invIA * rAxN * impulse);
                m_bodyB->setLinearVelocity(m_bodyB->getLinearVelocity() + normal * impulse * invMassB);
                m_bodyB->setAngularVelocity(m_bodyB->getAngularVelocity() + invIB * rBxN * impulse);
                totalImpulse += std::abs(impulse);
            }
        }

        // Part 2: Angular constraint - maintain initial relative rotation
        glm::dquat currentRelRot = glm::inverse(m_bodyA->getRotation()) * m_bodyB->getRotation();
        glm::dquat rotError = glm::inverse(m_initialRotDiff) * currentRelRot;

        // Convert quaternion error to axis-angle
        double angle = 2.0 * std::acos(std::clamp(rotError.w, -1.0, 1.0));
        if (angle > glm::pi<double>()) angle -= 2.0 * glm::pi<double>();

        if (std::abs(angle) > 1e-6) {
            glm::dvec3 axis;
            double sinHalf = std::sqrt(1.0 - rotError.w * rotError.w);
            if (sinHalf > 1e-6) {
                axis = glm::dvec3(rotError.x, rotError.y, rotError.z) / sinHalf;
            } else {
                axis = glm::dvec3(1.0, 0.0, 0.0);
            }

            const glm::dmat3& invIA = m_bodyA->getInverseInertiaTensorWorld();
            const glm::dmat3& invIB = m_bodyB->getInverseInertiaTensorWorld();

            glm::dvec3 iA_axis = invIA * axis;
            glm::dvec3 iB_axis = invIB * axis;
            double angMass = glm::dot(axis, iA_axis) + glm::dot(axis, iB_axis);
            if (angMass > 1e-10) {
                angMass = 1.0 / angMass;
                double angVel = glm::dot(m_bodyB->getAngularVelocity() - m_bodyA->getAngularVelocity(), axis);
                double bias = m_biasFactor * angle / std::max(dt, 1e-8);
                double impulse = angMass * (-angVel - bias);

                m_bodyA->setAngularVelocity(m_bodyA->getAngularVelocity() - iA_axis * impulse);
                m_bodyB->setAngularVelocity(m_bodyB->getAngularVelocity() + iB_axis * impulse);
                totalImpulse += std::abs(impulse);
            }
        }

        return totalImpulse;
    }

    void DrawDebug() override {}

    RigidBody* GetBodyA() const override { return m_bodyA; }
    RigidBody* GetBodyB() const override { return m_bodyB; }
};

} // namespace Physics
} // namespace Prisma
