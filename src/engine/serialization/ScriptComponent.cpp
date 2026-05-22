#include "ScriptComponent.h"
#include "core/ComponentRegistry.h"
#include "Logger.h"
#include <glaze/glaze.hpp>

// ── Glaze 元数据 ──
template <>
struct glz::meta<Prisma::Scripting::ScriptComponent::Data> {
    static constexpr auto value = glz::object(
        "scriptClass", &Prisma::Scripting::ScriptComponent::Data::scriptClass,
        "fields",      &Prisma::Scripting::ScriptComponent::Data::fields
    );
};

// ── ComponentRegistry 注册 ──
namespace {
    bool registered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::Scripting::ScriptComponent>("Script");
        reg.RegisterSerializable("Script",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::Scripting::ScriptComponent&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Scripting::ScriptComponent&>(comp);
                Prisma::Scripting::ScriptComponent::Data data;
                auto ec = glz::read_json(data, json);
                if (!ec) typed.SetData(data);
            }
        );
        return true;
    }();
}

namespace Prisma::Scripting {

ScriptComponent::~ScriptComponent() = default;

ScriptComponent::Data ScriptComponent::GetData() const {
    Data d;
    d.scriptClass  = m_scriptClass;
    d.fields       = m_fields;
    return d;
}

void ScriptComponent::SetData(const Data& d) {
    m_scriptClass  = d.scriptClass;
    m_fields       = d.fields;
}

} // namespace Prisma::Scripting
