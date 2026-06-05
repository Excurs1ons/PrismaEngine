#include "PhysicsComponents.h"
#include "physics/RigidBody.h"
#include "core/ComponentRegistry.h"
#include <glaze/glaze.hpp>

// ── Glaze 元数据 ──

template <>
struct glz::meta<Prisma::RigidBodyComponent::Data> {
    static constexpr auto value = glz::object(
        "mass",           &Prisma::RigidBodyComponent::Data::mass,
        "linearDamping",  &Prisma::RigidBodyComponent::Data::linearDamping,
        "angularDamping", &Prisma::RigidBodyComponent::Data::angularDamping,
        "useGravity",     &Prisma::RigidBodyComponent::Data::useGravity,
        "isStatic",        &Prisma::RigidBodyComponent::Data::isStatic
    );
};

template <>
struct glz::meta<Prisma::BoxColliderComponent::Data> {
    static constexpr auto value = glz::object(
        "size", &Prisma::BoxColliderComponent::Data::size
    );
};

// ── ComponentRegistry 注册 ──

namespace {
    bool rbRegistered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::RigidBodyComponent>("RigidBodyComponent");
        reg.RegisterSerializable("RigidBodyComponent",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::RigidBodyComponent&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::RigidBodyComponent&>(comp);
                Prisma::RigidBodyComponent::Data data;
                auto ec = glz::read<glz::opts{ .error_on_unknown_keys = false }>(data, json);
                if (!ec) typed.SetData(data);
            }
        );
        return true;
    }();

    bool bcRegistered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::BoxColliderComponent>("BoxColliderComponent");
        reg.RegisterSerializable("BoxColliderComponent",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::BoxColliderComponent&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::BoxColliderComponent&>(comp);
                Prisma::BoxColliderComponent::Data data;
                auto ec = glz::read<glz::opts{ .error_on_unknown_keys = false }>(data, json);
                if (!ec) typed.SetData(data);
            }
        );
        return true;
    }();
}

namespace Prisma {

// ── RigidBodyComponent 序列化实现 ──

RigidBodyComponent::Data RigidBodyComponent::GetData() const {
    Data d;
    d.mass = m_Desc.mass;
    d.linearDamping = m_Desc.linearDamping;
    d.angularDamping = m_Desc.angularDamping;
    d.useGravity = m_Desc.useGravity;
    d.isStatic = m_Desc.isStatic;
    return d;
}

void RigidBodyComponent::SetData(const Data& d) {
    m_Desc.mass = d.mass;
    m_Desc.linearDamping = d.linearDamping;
    m_Desc.angularDamping = d.angularDamping;
    m_Desc.useGravity = d.useGravity;
    m_Desc.isStatic = d.isStatic;
}

// ── BoxColliderComponent 序列化实现 ──

BoxColliderComponent::Data BoxColliderComponent::GetData() const {
    Data d;
    d.size = {m_Size.x, m_Size.y, m_Size.z};
    return d;
}

void BoxColliderComponent::SetData(const Data& d) {
    m_Size = {d.size[0], d.size[1], d.size[2]};
}

} // namespace Prisma