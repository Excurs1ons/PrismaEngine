#include "ComponentRegistry.h"
#include "logger/Logger.h"

namespace Prisma {

ComponentRegistry& ComponentRegistry::Get() {
    static ComponentRegistry instance;
    return instance;
}

void ComponentRegistry::RegisterSerializable(const char* typeName,
                                               ComponentRegistry::SerializeFunc serialize,
                                               ComponentRegistry::DeserializeFunc deserialize) {
    m_Serializers[typeName] = std::move(serialize);
    m_Deserializers[typeName] = std::move(deserialize);
}

std::shared_ptr<Component> ComponentRegistry::Create(const std::string& typeName) const {
    auto it = m_Factories.find(typeName);
    if (it != m_Factories.end()) {
        return it->second();
    }
    LOG_ERROR("ComponentRegistry", "未知组件类型: {0}", typeName);
    return nullptr;
}

std::string_view ComponentRegistry::GetTypeName(const Component& component) const {
    auto it = m_TypeNames.find(std::type_index(typeid(component)));
    if (it != m_TypeNames.end()) {
        return it->second;
    }
    LOG_WARNING("ComponentRegistry", "未注册的组件类型: {0}", typeid(component).name());
    return {};
}

bool ComponentRegistry::CanSerialize(const std::string& typeName) const {
    return m_Serializers.find(typeName) != m_Serializers.end();
}

std::string ComponentRegistry::SerializeComponent(const Component& comp) const {
    auto typeName = GetTypeName(comp);
    if (typeName.empty()) return {};
    auto it = m_Serializers.find(std::string(typeName));
    if (it != m_Serializers.end()) {
        return it->second(comp);
    }
    return {};
}

bool ComponentRegistry::DeserializeComponent(Component& comp, const std::string& typeName, const std::string& json) const {
    auto it = m_Deserializers.find(typeName);
    if (it != m_Deserializers.end()) {
        it->second(comp, json);
        return true;
    }
    return false;
}

} // namespace Prisma
