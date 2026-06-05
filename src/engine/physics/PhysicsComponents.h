#pragma once
#include "Component.h"
#include "math/MathTypes.h"
#include <array>

namespace Prisma {
namespace Physics { class RigidBody; }

struct RigidBodyDesc {
    float mass = 1.0f;
    float linearDamping = 0.01f;
    float angularDamping = 0.05f;
    bool useGravity = true;
    bool isStatic = false;
};

class RigidBodyComponent : public Component {
public:
    RigidBodyComponent(const RigidBodyDesc& desc = RigidBodyDesc()) : m_Desc(desc) {}

    void Update(Timestep /*ts*/) override {}
    void Initialize() override {}
    ComponentId GetComponentId() const override { return GetComponentTypeId<RigidBodyComponent>(); }
    const char* GetComponentTypeName() const override { return "RigidBodyComponent"; }

    void ApplyForce(const Vector3& force) { m_AccumulatedForce += force; }
    void ApplyImpulse(const Vector3& impulse) { m_Velocity += impulse * (1.0f / m_Desc.mass); }

    Vector3& GetVelocity() { return m_Velocity; }
    const RigidBodyDesc& GetDesc() const { return m_Desc; }
    RigidBodyDesc& GetDescMutable() { return m_Desc; }

    void SetVelocity(const Vector3& v) { m_Velocity = v; }

    // 底层物理刚体（由 PhysicsSystem 创建和管理）
    Physics::RigidBody* GetPhysicsBody() const { return m_physicsBody; }
    void SetPhysicsBody(Physics::RigidBody* body) { m_physicsBody = body; }

    // 序列化数据结构
    struct Data {
        float mass = 1.0f;
        float linearDamping = 0.01f;
        float angularDamping = 0.05f;
        bool useGravity = true;
        bool isStatic = false;
    };

    Data GetData() const;
    void SetData(const Data& d);

private:
    RigidBodyDesc m_Desc;
    Vector3 m_Velocity{0.0f};
    Vector3 m_AccumulatedForce{0.0f};
    Physics::RigidBody* m_physicsBody = nullptr;  // 非 owning，由 PhysicsSystem 管理

    friend class PhysicsSystem;
};

class BoxColliderComponent : public Component {
public:
    BoxColliderComponent(const Vector3& size = {1.0f, 1.0f, 1.0f}) : m_Size(size) {}

    void Update(Timestep /*ts*/) override {}
    void Initialize() override {}
    ComponentId GetComponentId() const override { return GetComponentTypeId<BoxColliderComponent>(); }
    const char* GetComponentTypeName() const override { return "BoxColliderComponent"; }

    const Vector3& GetSize() const { return m_Size; }
    void SetSize(const Vector3& s) { m_Size = s; }

    // 序列化数据结构
    struct Data {
        std::array<float, 3> size = {1.0f, 1.0f, 1.0f};
    };

    Data GetData() const;
    void SetData(const Data& d);

private:
    Vector3 m_Size;
};

} // namespace Prisma