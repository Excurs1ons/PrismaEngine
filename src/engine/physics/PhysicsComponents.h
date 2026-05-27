#pragma once
#include "Component.h"
#include "math/MathTypes.h"

namespace Prisma {

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
    
    void SetVelocity(const Vector3& v) { m_Velocity = v; }

private:
    RigidBodyDesc m_Desc;
    Vector3 m_Velocity{0.0f};
    Vector3 m_AccumulatedForce{0.0f};
    
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

private:
    Vector3 m_Size;
};

} // namespace Prisma
