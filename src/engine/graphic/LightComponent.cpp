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
        "Spot", Spot,
        "Ambient", Ambient
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
                glz::json_t root;
                auto ec = glz::read_json(root, glz::write_json(data).value_or("{}"));
                if (!ec && root.is_object()) {
                    auto& obj = root.get_object();
                    obj["enabled"] = typed.IsEnabled();
                    return glz::write_json(root).value_or("");
                }
                // fallback: 只写 data
                std::string json;
                auto e = glz::write_json(data, json);
                if (e) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Graphic::LightComponent&>(comp);
                
                // 用 json_t 解析，提取 data 字段和 enabled
                glz::json_t root;
                auto ec = glz::read_json(root, json);
                if (ec) return;
                if (!root.is_object()) return;
                auto& obj = root.get_object();

                // 1. 提取并移除 enabled，避免干扰 Data 的反序列化（某些严格模式下的 Glaze 可能报错）
                auto it = obj.find("enabled");
                if (it != obj.end() && it->second.is_boolean()) {
                    typed.SetEnabled(it->second.get_boolean());
                    obj.erase(it);
                }

                // 2. 反序列化剩余字段到 Data
                Prisma::Graphic::LightComponent::Data data;
                auto de = glz::read<glz::opts{ .error_on_unknown_keys = false }>(data, glz::write_json(root).value_or("{}"));
                if (!de) {
                    typed.SetData(data);
                } else {
                    LOG_WARN("LightComponent", "反序列化 Light Data 失败，错误码: {}", static_cast<int>(de.ec));
                }
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
        if (m_Data.type == LightType::Ambient) {
            // 环境光没有方向
            light.direction = PrismaMath::vec4(0, 0, 0, static_cast<float>(m_Data.type));
        } else {
            PrismaMath::vec3 forward = transform->GetForward();
            light.direction = PrismaMath::vec4(forward.x, forward.y, forward.z, static_cast<float>(m_Data.type));
        }
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
