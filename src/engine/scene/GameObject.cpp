#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "ComponentRegistry.h"
#include "Logger.h"
#include <glaze/glaze.hpp>

// ── Glaze meta（必须在全局命名空间）──
template <>
struct glz::meta<Prisma::GameObject::ComponentEntry> {
    static constexpr auto value = glz::object(
        "type", &Prisma::GameObject::ComponentEntry::type,
        "data", &Prisma::GameObject::ComponentEntry::data
    );
};

template <>
struct glz::meta<Prisma::GameObject::Data> {
    static constexpr auto value = glz::object(
        "name", &Prisma::GameObject::Data::name,
        "transform", &Prisma::GameObject::Data::transform,
        "components", &Prisma::GameObject::Data::components
    );
};

namespace Prisma {

// ── 生命周期 ──

GameObject::GameObject() : name("GameObject") {
    m_Transform = std::make_shared<Transform>();
}

GameObject::GameObject(std::string name) : name(std::move(name)) {
    m_Transform = std::make_shared<Transform>();
}

GameObject::~GameObject() {
    Shutdown();
}

void GameObject::Initialize() {
    if (m_Transform) {
        m_Transform->SetOwner(this);
        m_Transform->Initialize();
    }
}

void GameObject::Update(Timestep ts) {
    if (m_Transform) {
        m_Transform->Update(ts);
    }
    for (auto& comp : m_Components) {
        comp->Update(ts);
    }
}

void GameObject::Shutdown() {
    for (auto& comp : m_Components) {
        comp->Shutdown();
    }
    m_Components.clear();
    if (m_Transform) {
        m_Transform->Shutdown();
    }
}

// ── 序列化 ──

GameObject::Data GameObject::GetData() const {
    Data d;
    d.name = name;
    d.transform = m_Transform->GetData();

    auto& reg = ComponentRegistry::Get();
    for (auto& comp : m_Components) {
        auto typeName = reg.GetTypeName(*comp);
        if (typeName.empty()) continue;

        ComponentEntry entry;
        entry.type = std::string(typeName);
        auto json = reg.SerializeComponent(*comp);
        if (!json.empty()) {
            auto ec = glz::read_json(entry.data, json);
            if (ec) {
                LOG_WARN("GameObject", "反序列化组件数据失败，保留原始数据");
            }
        }
        d.components.push_back(std::move(entry));
    }
    return d;
}

void GameObject::SetData(const Data& d) {
    name = d.name;
    m_Transform->SetData(d.transform);

    auto& reg = ComponentRegistry::Get();
    for (const auto& entry : d.components) {
        auto comp = reg.Create(entry.type);
        if (!comp) continue;

        comp->SetOwner(this);
        comp->Initialize();

        auto json = entry.data.dump();
        if (json) {
            reg.DeserializeComponent(*comp, entry.type, *json);
        }

        m_Components.push_back(comp);
    }
}

} // namespace Prisma
