#pragma once

#include "Export.h"
#include "Component.h"
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <typeindex>
#include <functional>

namespace Prisma {

/**
 * @brief 组件类型注册表
 * 以字符串标识符注册 Component 子类的工厂函数，
 * 支持通过类型名创建组件实例和反向查询。
 */
class ENGINE_API ComponentRegistry {
public:
    static ComponentRegistry& Get();

    // 基础注册：仅创建
    template<typename T>
    void Register(const char* typeName) {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        m_Factories[typeName] = []() { return std::make_shared<T>(); };
        m_TypeNames[std::type_index(typeid(T))] = typeName;
    }

    // 类型别名（必须放在使用之前）
    using SerializeFunc = std::function<std::string(const Component&)>;
    using DeserializeFunc = std::function<void(Component&, const std::string&)>;

    // 可序列化注册：创建 + 数据序列化回调
    // 调用方提供 serialize/deserialize lambda（需包含 <glaze/glaze.hpp>）
    void RegisterSerializable(const char* typeName,
                              SerializeFunc serialize,
                              DeserializeFunc deserialize);

    std::shared_ptr<Component> Create(const std::string& typeName) const;
    std::string_view GetTypeName(const Component& component) const;

    // 序列化支持
    bool CanSerialize(const std::string& typeName) const;
    std::string SerializeComponent(const Component& comp) const;
    bool DeserializeComponent(Component& comp, const std::string& typeName, const std::string& json) const;

private:
    ComponentRegistry() = default;

    using FactoryFunc = std::function<std::shared_ptr<Component>()>;

    std::unordered_map<std::string, FactoryFunc> m_Factories;
    std::unordered_map<std::type_index, std::string> m_TypeNames;
    std::unordered_map<std::string, SerializeFunc> m_Serializers;
    std::unordered_map<std::string, DeserializeFunc> m_Deserializers;
};

} // namespace Prisma
