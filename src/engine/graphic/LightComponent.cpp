#include "LightComponent.h"
#include "ComponentRegistry.h"
#include "transform/Transform.h"
#include "core/Node.h"
#include <glaze/glaze.hpp>

// ── Glaze 元数据 ──

template <>
struct glz::meta<Prisma::Graphic::LightComponent::LightType> {
    using enum Prisma::Graphic::LightComponent::LightType;
    static constexpr auto value = glz::enumerate(
        "Directional", Directional,
        "Point", Point,
        "Spot", Spot
    );
};

template <>
struct glz::meta<Prisma::Graphic::LightComponent::Data> {
    static constexpr auto value = glz::object(
        "type",       &Prisma::Graphic::LightComponent::Data::type,
        "color",      &Prisma::Graphic::LightComponent::Data::color,
        "intensity",  &Prisma::Graphic::LightComponent::Data::intensity,
        "range",      &Prisma::Graphic::LightComponent::Data::range,
        "spotAngle",  &Prisma::Graphic::LightComponent::Data::spotAngle
    );
};

// ── 注册 ──
namespace {
    bool registered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::Graphic::LightComponent>("Light");
        reg.RegisterSerializable("Light",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::Graphic::LightComponent&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Graphic::LightComponent&>(comp);
                Prisma::Graphic::LightComponent::Data data;
                auto ec = glz::read_json(data, json);
                if (!ec) typed.SetData(data);
            }
        );
        return true;
    }();
}

namespace Prisma::Graphic {

LightComponent::LightComponent() : Component() {}

Light LightComponent::GetLightData() const {
    Light light{};
    
    // 1. Position (w 为填充)
    auto transform = GetTransform();
    if (transform) {
        PrismaMath::vec3 pos = transform->GetPosition();
        light.position = PrismaMath::vec4(pos.x, pos.y, pos.z, 0.0f);
        
        // 3. Direction (w 为类型)
        PrismaMath::vec3 forward = transform->GetForward();
        light.direction = PrismaMath::vec4(forward.x, forward.y, forward.z, static_cast<float>(m_Data.type));
    } else {
        light.position = PrismaMath::vec4(0, 0, 0, 0);
        light.direction = PrismaMath::vec4(0, 0, 1, static_cast<float>(m_Data.type));
    }
    
    // 2. Color (rgb 为带强度的颜色, w 为范围)
    light.color = PrismaMath::vec4(
        m_Data.color[0] * m_Data.intensity, 
        m_Data.color[1] * m_Data.intensity, 
        m_Data.color[2] * m_Data.intensity, 
        m_Data.range
    );

    return light;
}

} // namespace Prisma::Graphic
