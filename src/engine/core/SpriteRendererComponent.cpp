#include "SpriteRendererComponent.h"
#include "EntityManager.h"
#include "ComponentRegistry.h"
#include <glaze/glaze.hpp>

// ── Glaze 元数据 ──

template <>
struct glz::meta<Prisma::Core::SpriteRendererComponent::Data> {
    static constexpr auto value = glz::object(
        "color", &Prisma::Core::SpriteRendererComponent::Data::color,
        "size",  &Prisma::Core::SpriteRendererComponent::Data::size
    );
};

// ── ComponentRegistry 注册 ──

namespace {
    bool registered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::Core::SpriteRendererComponent>("SpriteRendererComponent");
        reg.RegisterSerializable("SpriteRendererComponent",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::Core::SpriteRendererComponent&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Core::SpriteRendererComponent&>(comp);
                Prisma::Core::SpriteRendererComponent::Data data;
                auto ec = glz::read<glz::opts{ .error_on_unknown_keys = false }>(data, json);
                // 同步数据到组件及渲染 SoA
                if (!ec) {
                    typed.SetData(data);
                }
            }
        );
        return true;
    }();
}

namespace Prisma {
namespace Core {

SpriteRendererComponent::Data SpriteRendererComponent::GetData() const {
    Data d;
    d.color = {color.r, color.g, color.b, color.a};
    d.size = {size.x, size.y};
    return d;
}

void SpriteRendererComponent::SetData(const Data& d) {
    color = {d.color[0], d.color[1], d.color[2], d.color[3]};
    size = {d.size[0], d.size[1]};
    // 同步到 SoA
    if (m_ownerNode.IsValid()) {
        WriteToSoA(m_ownerNode.GetIndex());
    }
}

void SpriteRendererComponent::WriteToSoA(uint32_t entityIndex) const {
    auto& em = EntityManager::Get();
    auto* rb = em.GetRenderData();
    rb->colorR[entityIndex] = color.r;
    rb->colorG[entityIndex] = color.g;
    rb->colorB[entityIndex] = color.b;
    rb->colorA[entityIndex] = color.a;
    rb->sizeW[entityIndex] = size.x;
    rb->sizeH[entityIndex] = size.y;
}

} // namespace Core
} // namespace Prisma
