#include "PhysicsSystem.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Engine.h"
#include <algorithm>

namespace Prisma {

int PhysicsSystem::Initialize() {
    LOG_INFO("Physics", "Prisma 物理子系统正在初始化...");

    // 默认配置
    m_gravity = glm::dvec3(0.0, -9.81, 0.0);
    m_solver.m_iterations = 8;
    m_ccdEnabled = true;
    m_ccdThreshold = 5.0;

    LOG_INFO("Physics", "物理系统初始化完成: 重力={}, {}, {} | 求解器迭代={} | CCD={}",
             m_gravity.x, m_gravity.y, m_gravity.z,
             m_solver.m_iterations,
             m_ccdEnabled ? "启用" : "禁用");
    return 0;
}

void PhysicsSystem::Shutdown() {
    LOG_INFO("Physics", "物理子系统正在关闭。");

    // 清理约束
    m_constraints.clear();
    m_pendingDestroyConstraints.clear();

    // 清理触发
    m_triggerManager.clear();

    // 清理刚体
    m_bodies.clear();

    LOG_INFO("Physics", "物理子系统已关闭。");
}

void PhysicsSystem::Update(Timestep ts) {
    float dt = ts.GetSeconds();
    if (dt <= 0.0f) return;

    // 限制最大步长避免爆炸
    if (dt > 0.1f) dt = 0.1f;

    // 完整物理流水线
    // 顺序: applyForces -> damp -> integrateVelocity -> CCD -> collide
    //       -> solveConstraints -> integratePosition -> triggers

    stepApplyForces(static_cast<double>(dt));
    stepApplyDamping(static_cast<double>(dt));
    stepIntegrateVelocity(static_cast<double>(dt));

    if (m_ccdEnabled) {
        stepCCD(static_cast<double>(dt));
    }

    stepCollide();
    stepSolveConstraints(static_cast<double>(dt));
    stepIntegratePosition(static_cast<double>(dt));
    stepTriggers(static_cast<double>(dt));
}

// ========== 刚体管理 ==========

Physics::RigidBody* PhysicsSystem::createRigidBody(Physics::RigidBodyType type) {
    auto body = std::make_unique<Physics::RigidBody>(type);
    Physics::RigidBody* ptr = body.get();
    m_bodies.push_back(std::move(body));
    return ptr;
}

void PhysicsSystem::destroyRigidBody(Physics::RigidBody* body) {
    auto it = std::find_if(m_bodies.begin(), m_bodies.end(),
        [body](const auto& b) { return b.get() == body; });
    if (it != m_bodies.end()) {
        // 从约束中移除对该刚体的引用
        m_constraints.erase(
            std::remove_if(m_constraints.begin(), m_constraints.end(),
                [body](Physics::IConstraint* c) {
                    return c->GetBodyA() == body || c->GetBodyB() == body;
                }),
            m_constraints.end());
        m_bodies.erase(it);
    }
}

// ========== 约束管理 ==========

void PhysicsSystem::addConstraint(Physics::IConstraint* constraint) {
    if (constraint && constraint->IsValid()) {
        m_constraints.push_back(constraint);
    }
}

void PhysicsSystem::removeConstraint(Physics::IConstraint* constraint) {
    auto it = std::find(m_constraints.begin(), m_constraints.end(), constraint);
    if (it != m_constraints.end()) {
        m_constraints.erase(it);
    }
}

// ========== 触发管理 ==========

uint32_t PhysicsSystem::addTrigger(const Physics::TriggerVolume& trigger) {
    return m_triggerManager.addTrigger(trigger);
}

void PhysicsSystem::removeTrigger(uint32_t triggerId) {
    m_triggerManager.removeTrigger(triggerId);
}

// ========== 内部物理步 ==========

void PhysicsSystem::stepApplyForces(double dt) {
    for (auto& body : m_bodies) {
        if (!body->isActive() || body->isStatic() || !body->isAwake()) continue;

        // 应用重力
        body->applyForce(m_gravity * body->getMass());
    }
}

void PhysicsSystem::stepApplyDamping(double dt) {
    for (auto& body : m_bodies) {
        if (!body->isActive() || body->isStatic() || !body->isAwake()) continue;
        body->applyDamping(dt);
    }
}

void PhysicsSystem::stepIntegrateVelocity(double dt) {
    for (auto& body : m_bodies) {
        if (!body->isActive() || body->isStatic() || !body->isAwake()) continue;

        // Semi-implicit Euler: velocity update first
        glm::dvec3 acceleration = body->m_accumulatedForce * body->m_inverseMass;
        body->m_linearVelocity += acceleration * dt;

        // Angular acceleration
        body->m_angularVelocity += body->m_inverseInertiaTensorWorld * body->m_accumulatedTorque * dt;

        // 运动状态（用于插值）
        body->m_motionState.storeCurrentState();
    }
}

void PhysicsSystem::stepCCD(double dt) {
    m_ccdSolver.performCCD(m_bodies, dt);
}

void PhysicsSystem::stepCollide() {
    m_solver.clear();

    // 对所有体对进行碰撞检测
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        auto& bodyA = m_bodies[i];
        if (!bodyA->isActive() || !bodyA->isAwake()) continue;

        // 静态体不主动碰撞（但可以被碰撞）
        if (bodyA->isStatic()) continue;

        Physics::AABB aabbA = bodyA->getWorldAABB();

        for (size_t j = i + 1; j < m_bodies.size(); ++j) {
            auto& bodyB = m_bodies[j];
            if (!bodyB->isActive()) continue;
            if (bodyA->isStatic() && bodyB->isStatic()) continue;
            if (!bodyB->isAwake() && bodyA->isStatic()) continue;

            Physics::AABB aabbB = bodyB->getWorldAABB();

            // AABB 粗略检测
            if (!Physics::CollisionSystem::checkAABB(aabbA, aabbB)) continue;

            // AABB 穿透检测（获取穿透深度和方向）
            glm::dvec3 penetration(0.0);
            if (!Physics::CollisionSystem::checkAABBPenetration(aabbA, aabbB, penetration)) continue;

            // 创建接触约束
            Physics::ContactConstraint contact;
            contact.bodyA = bodyA.get();
            contact.bodyB = bodyB.get();
            contact.penetration = glm::length(penetration);

            // 法线方向（从 A 指向 B）
            if (contact.penetration > 1e-8) {
                contact.contactNormal = penetration / contact.penetration;
            } else {
                contact.contactNormal = glm::dvec3(0.0, 1.0, 0.0);
            }

            // 接触点：两个 AABB 中心的中点
            contact.contactPoint = (aabbA.getCenter() + aabbB.getCenter()) * 0.5;

            // 默认恢复系数和摩擦
            contact.restitution = 0.3;
            contact.friction = 0.5;

            // 应用到求解器
            m_solver.addContact(contact);
        }
    }
}

void PhysicsSystem::stepSolveConstraints(double dt) {
    // 添加所有关节约束到求解器
    for (auto* constraint : m_constraints) {
        m_solver.addJoint(constraint);
    }

    // 求解所有约束
    m_solver.solve(dt);
}

void PhysicsSystem::stepIntegratePosition(double dt) {
    for (auto& body : m_bodies) {
        if (!body->isActive() || body->isStatic() || !body->isAwake()) continue;

        // 使用约束修正后的速度更新位置
        body->m_position += body->m_linearVelocity * dt;

        // 使用约束修正后的角速度更新朝向
        glm::dquat omegaQuat(0.0,
            body->m_angularVelocity.x,
            body->m_angularVelocity.y,
            body->m_angularVelocity.z);
        body->m_rotation += dt * 0.5 * omegaQuat * body->m_rotation;
        body->m_rotation = glm::normalize(body->m_rotation);

        // 更新世界空间惯性张量
        body->updateInertiaTensorWorld();

        // 睡眠检测
        double linVelSq = glm::dot(body->m_linearVelocity, body->m_linearVelocity);
        double angVelSq = glm::dot(body->m_angularVelocity, body->m_angularVelocity);
        if (linVelSq < 0.0001 && angVelSq < 0.0025) {
            body->m_sleepTime += dt;
            if (body->m_sleepTime > 0.5) {
                body->setAwake(false);
                continue;
            }
        } else {
            body->m_sleepTime = 0.0;
        }

        // 清除力和力矩
        body->m_accumulatedForce = glm::dvec3(0.0);
        body->m_accumulatedTorque = glm::dvec3(0.0);

        // 更新运动状态
        body->m_motionState.setCurrentState(body->m_position, body->m_rotation);
    }
}

void PhysicsSystem::stepTriggers(double dt) {
    // 收集所有刚体的 AABB 用于触发检测
    std::vector<std::pair<uint32_t, Physics::AABB>> entityAABBs;
    entityAABBs.reserve(m_bodies.size());

    for (uint32_t idx = 0; idx < static_cast<uint32_t>(m_bodies.size()); ++idx) {
        auto& body = m_bodies[idx];
        if (!body->isActive()) continue;
        entityAABBs.emplace_back(idx, body->getWorldAABB());
    }

    // 更新触发管理器
    m_triggerManager.update(dt, entityAABBs);
}

} // namespace Prisma
