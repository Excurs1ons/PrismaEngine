#include "PrimitiveComponent.h"
#include "core/ComponentRegistry.h"
#include <glaze/glaze.hpp>

// ── Glaze 元数据 ──
template <>
struct glz::meta<Prisma::Graphic::PrimitiveComponent::Data> {
    static constexpr auto value = glz::object(
        "shape",    &Prisma::Graphic::PrimitiveComponent::Data::shape,
        "material", &Prisma::Graphic::PrimitiveComponent::Data::material,
        "emissive", &Prisma::Graphic::PrimitiveComponent::Data::emissive
    );
};

// ── 注册 ──
namespace {
    bool registered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::Graphic::PrimitiveComponent>("PrimitiveComponent");
        reg.RegisterSerializable("PrimitiveComponent",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::Graphic::PrimitiveComponent&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Graphic::PrimitiveComponent&>(comp);
                Prisma::Graphic::PrimitiveComponent::Data data;
                auto ec = glz::read_json(data, json);
                if (!ec) typed.SetData(data);
            }
        );
        return true;
    }();
}

namespace Prisma::Graphic {

PrimitiveComponent::~PrimitiveComponent() = default;

PrimitiveComponent::Data PrimitiveComponent::GetData() const {
    Data d;
    d.shape = ShapeToString(m_shape);
    d.material = m_materialPath;
    d.emissive = {m_emissive.x, m_emissive.y, m_emissive.z};
    return d;
}

void PrimitiveComponent::SetData(const Data& d) {
    m_shape = ShapeFromString(d.shape);
    m_materialPath = d.material;
    m_emissive = {d.emissive[0], d.emissive[1], d.emissive[2]};
}

PrimitiveShape PrimitiveComponent::ShapeFromString(const std::string& s) {
    if (s == "Sphere") return PrimitiveShape::Sphere;
    if (s == "Box")    return PrimitiveShape::Box;
    if (s == "Cone")   return PrimitiveShape::Cone;
    if (s == "Plane")  return PrimitiveShape::Plane;
    return PrimitiveShape::Sphere;
}

const char* PrimitiveComponent::ShapeToString(PrimitiveShape s) {
    switch (s) {
        case PrimitiveShape::Sphere: return "Sphere";
        case PrimitiveShape::Box:    return "Box";
        case PrimitiveShape::Cone:   return "Cone";
        case PrimitiveShape::Plane:  return "Plane";
    }
    return "Sphere";
}

} // namespace Prisma::Graphic
