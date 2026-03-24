#include "PhysicsSystem.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Engine.h"
#include "GameObject.h"
#include "physics/PhysicsComponents.h"

namespace Prisma {

int PhysicsSystem::Initialize() {
    LOG_INFO("Physics", "Prisma Physics Subsystem Initializing...");
    return 0;
}

void PhysicsSystem::Shutdown() {
    LOG_INFO("Physics", "Physics system shutting down.");
}

void PhysicsSystem::Update(Timestep ts) {
    float dt = ts.GetSeconds();
    if (dt <= 0.0f) return;

    auto scene = Engine::Get().GetSceneManager()->GetCurrentScene();
    if (!scene) return;

    const Vector3 gravity{0.0f, -9.81f, 0.0f};

    // 1. 物理积分更新
    for (auto& obj : scene->GetGameObjects()) {
        auto rb = obj->GetComponent<RigidBodyComponent>();
        if (!rb || rb->m_Desc.isStatic) continue;

        auto transform = obj->GetTransform();
        
        // 积分速度
        Vector3 force = rb->m_AccumulatedForce;
        if (rb->m_Desc.useGravity) force += gravity * rb->m_Desc.mass;
        
        rb->m_Velocity += (force / rb->m_Desc.mass) * dt;
        rb->m_Velocity *= (1.0f - rb->m_Desc.linearDamping); // 阻尼

        // 更新位置
        Vector3 pos = transform->GetPosition();
        pos += rb->m_Velocity * dt;
        
        // 极简碰撞：简单的地面检测 (y=0)
        if (pos.y < 0.0f) {
            pos.y = 0.0f;
            rb->m_Velocity.y = -rb->m_Velocity.y * 0.5f; // 简单的弹力
        }

        transform->SetPosition(pos);
        rb->m_AccumulatedForce = Vector3(0.0f); // 重置力
    }
}

} // namespace Prisma
