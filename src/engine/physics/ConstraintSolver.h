#pragma once

#include "physics/RigidBody.h"
#include "physics/Constraint.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

namespace Prisma {
namespace Physics {

// 接触约束（碰撞产生的临时约束）
struct ContactConstraint {
    RigidBody* bodyA = nullptr;
    RigidBody* bodyB = nullptr;
    glm::dvec3 contactNormal{ 0.0 };     // 从 A 指向 B 的法线
    glm::dvec3 contactPoint{ 0.0 };     // 接触点（世界空间）
    double penetration = 0.0;            // 穿透深度
    double restitution = 0.0;            // 恢复系数 (0 = 完全非弹性, 1 = 完全弹性)
    double friction = 0.5;               // 摩擦系数
    double accumulatedNormalImpulse = 0.0;
    double accumulatedTangentImpulse = 0.0;

    bool isValid() const { return bodyA != nullptr && bodyB != nullptr; }
};

// Sequential Impulse Solver
// 使用 Gauss-Seidel 风格的顺序脉冲求解器
// 迭代多次收敛约束解
class ConstraintSolver {
public:
    // 迭代次数（5-10 次以保证稳定）
    int m_iterations = 8;

    ConstraintSolver() = default;

    void clear() {
        m_contacts.clear();
        m_joints.clear();
    }

    void addContact(const ContactConstraint& contact) {
        m_contacts.push_back(contact);
    }

    void addJoint(IConstraint* joint) {
        if (joint && joint->IsValid()) {
            m_joints.push_back(joint);
        }
    }

    // 求解所有约束，执行指定迭代次数
    void solve(double dt) {
        if (dt <= 0.0) return;

        // 多次迭代收敛约束解
        for (int iter = 0; iter < m_iterations; ++iter) {
            // 求解接触约束
            for (auto& contact : m_contacts) {
                solveContact(contact, dt);
            }

            // 求解关节约束
            for (auto* joint : m_joints) {
                joint->Solve(dt);
            }
        }
    }

    int getContactCount() const { return static_cast<int>(m_contacts.size()); }
    int getJointCount() const { return static_cast<int>(m_joints.size()); }

private:
    std::vector<ContactConstraint> m_contacts;
    std::vector<IConstraint*> m_joints;

    void solveContact(ContactConstraint& contact, double dt) {
        if (!contact.isValid()) return;
        if (contact.bodyA->isStatic() && contact.bodyB->isStatic()) return;

        RigidBody* bodyA = contact.bodyA;
        RigidBody* bodyB = contact.bodyB;

        // 计算从质心到接触点的向量
        glm::dvec3 rA = contact.contactPoint - bodyA->getPosition();
        glm::dvec3 rB = contact.contactPoint - bodyB->getPosition();
        glm::dvec3 normal = contact.contactNormal;

        // 计算接触点的相对速度
        glm::dvec3 velA = bodyA->getLinearVelocity() + glm::cross(bodyA->getAngularVelocity(), rA);
        glm::dvec3 velB = bodyB->getLinearVelocity() + glm::cross(bodyB->getAngularVelocity(), rB);
        glm::dvec3 relVel = velB - velA;

        double velAlongNormal = glm::dot(relVel, normal);

        // Baumgarte stabilization - 修正穿透
        double bias = 0.0;
        if (contact.penetration > 0.0) {
            bias = (0.2 / std::max(dt, 1e-8)) * contact.penetration;
        }

        // 计算法线方向的脉冲
        double invMassA = bodyA->getInverseMass();
        double invMassB = bodyB->getInverseMass();
        const glm::dmat3& invIA = bodyA->getInverseInertiaTensorWorld();
        const glm::dmat3& invIB = bodyB->getInverseInertiaTensorWorld();

        glm::dvec3 rAxN = glm::cross(rA, normal);
        glm::dvec3 rBxN = glm::cross(rB, normal);

        double effectiveMass = invMassA + invMassB;
        effectiveMass += glm::dot(rAxN, invIA * rAxN);
        effectiveMass += glm::dot(rBxN, invIB * rBxN);

        if (effectiveMass < 1e-10) return;
        effectiveMass = 1.0 / effectiveMass;

        // 恢复系数影响速度修正
        double restitutionTerm = contact.restitution * velAlongNormal;
        double normalImpulse = effectiveMass * (-velAlongNormal - restitutionTerm - bias);

        // 累积脉冲 Clamping（防止脉冲方向反转）
        double oldImpulse = contact.accumulatedNormalImpulse;
        contact.accumulatedNormalImpulse = std::max(0.0, oldImpulse + normalImpulse);
        normalImpulse = contact.accumulatedNormalImpulse - oldImpulse;

        // 应用法线脉冲
        glm::dvec3 impulseVec = normal * normalImpulse;
        if (!bodyA->isStatic()) {
            bodyA->setLinearVelocity(bodyA->getLinearVelocity() - impulseVec * invMassA);
            bodyA->setAngularVelocity(bodyA->getAngularVelocity() - invIA * rAxN * normalImpulse);
        }
        if (!bodyB->isStatic()) {
            bodyB->setLinearVelocity(bodyB->getLinearVelocity() + impulseVec * invMassB);
            bodyB->setAngularVelocity(bodyB->getAngularVelocity() + invIB * rBxN * normalImpulse);
        }

        // ── 摩擦（切向脉冲）──
        glm::dvec3 tangentVel = relVel - normal * velAlongNormal;
        double tangentSpeed = glm::length(tangentVel);
        if (tangentSpeed > 1e-8) {
            glm::dvec3 tangentDir = tangentVel / tangentSpeed;

            glm::dvec3 rAxT = glm::cross(rA, tangentDir);
            glm::dvec3 rBxT = glm::cross(rB, tangentDir);

            double tangentMass = invMassA + invMassB;
            tangentMass += glm::dot(rAxT, invIA * rAxT);
            tangentMass += glm::dot(rBxT, invIB * rBxT);

            if (tangentMass > 1e-10) {
                tangentMass = 1.0 / tangentMass;
                double frictionImpulse = -tangentSpeed * tangentMass;

                // Coulomb friction model: clamp friction impulse
                double maxFriction = contact.friction * contact.accumulatedNormalImpulse;
                double oldTangentImpulse = contact.accumulatedTangentImpulse;
                contact.accumulatedTangentImpulse += frictionImpulse;
                contact.accumulatedTangentImpulse = std::clamp(
                    contact.accumulatedTangentImpulse, -maxFriction, maxFriction);
                frictionImpulse = contact.accumulatedTangentImpulse - oldTangentImpulse;

                glm::dvec3 frictionVec = tangentDir * frictionImpulse;
                if (!bodyA->isStatic()) {
                    bodyA->setLinearVelocity(bodyA->getLinearVelocity() - frictionVec * invMassA);
                    bodyA->setAngularVelocity(bodyA->getAngularVelocity() - invIA * rAxT * frictionImpulse);
                }
                if (!bodyB->isStatic()) {
                    bodyB->setLinearVelocity(bodyB->getLinearVelocity() + frictionVec * invMassB);
                    bodyB->setAngularVelocity(bodyB->getAngularVelocity() + invIB * rBxT * frictionImpulse);
                }
            }
        }
    }
};

} // namespace Physics
} // namespace Prisma
