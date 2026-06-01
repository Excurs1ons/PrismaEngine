#include "ComponentSerializer.h"
#include "core/ComponentRegistry.h"
#include "transform/Transform.h"
#include "logger/Logger.h"

namespace Prisma {
namespace MCP {

glz::json_t ComponentSerializer::Serialize(void* componentData,
                                            const std::string& typeName,
                                            const std::vector<std::string>& fields,
                                            bool omitDefaults) {
    glz::json_t result;

    if (typeName == "Transform") {
        auto* transform = static_cast<Transform*>(componentData);
        if (!transform) {
            LOG_WARNING("ComponentSerializer", "Transform 组件数据为空");
            return glz::json_t::object_t{};
        }

        auto pos = transform->GetPosition();
        auto rot = transform->GetRotation();
        auto scale = transform->GetScale();

        result = glz::json_t::object_t{
            {"position", glz::json_t::array_t{static_cast<double>(pos.x), static_cast<double>(pos.y), static_cast<double>(pos.z)}},
            {"rotation", glz::json_t::array_t{static_cast<double>(rot.x), static_cast<double>(rot.y), static_cast<double>(rot.z), static_cast<double>(rot.w)}},
            {"scale", glz::json_t::array_t{static_cast<double>(scale.x), static_cast<double>(scale.y), static_cast<double>(scale.z)}}
        };

        if (omitDefaults) {
            auto& obj = result.get_object();
            auto posIt = obj.find("position");
            if (posIt != obj.end() && posIt->second.get_array().size() >= 3) {
                auto& arr = posIt->second.get_array();
                if (arr[0].get_number() == 0.0 && arr[1].get_number() == 0.0 && arr[2].get_number() == 0.0)
                    obj.erase("position");
            }
            auto rotIt = obj.find("rotation");
            if (rotIt != obj.end() && rotIt->second.get_array().size() >= 4) {
                auto& arr = rotIt->second.get_array();
                if (arr[0].get_number() == 0.0 && arr[1].get_number() == 0.0 &&
                    arr[2].get_number() == 0.0 && arr[3].get_number() == 1.0)
                    obj.erase("rotation");
            }
            auto scaleIt = obj.find("scale");
            if (scaleIt != obj.end() && scaleIt->second.get_array().size() >= 3) {
                auto& arr = scaleIt->second.get_array();
                if (arr[0].get_number() == 1.0 && arr[1].get_number() == 1.0 && arr[2].get_number() == 1.0)
                    obj.erase("scale");
            }
        }
    } else {
        auto& registry = ComponentRegistry::Get();
        if (registry.CanSerialize(typeName)) {
            auto* comp = static_cast<Component*>(componentData);
            if (!comp) {
                LOG_WARNING("ComponentSerializer", "组件数据为空: {}", typeName);
                return glz::json_t::object_t{};
            }

            auto jsonStr = registry.SerializeComponent(*comp);
            auto ec = glz::read_json(result, jsonStr);
            if (ec) {
                LOG_WARNING("ComponentSerializer", "解析序列化 JSON 失败: {}", typeName);
                return glz::json_t::object_t{};
            }
        } else {
            LOG_WARNING("ComponentSerializer", "未注册的组件类型: {}", typeName);
            return glz::json_t::object_t{};
        }
    }

    if (!fields.empty()) {
        glz::json_t filtered = glz::json_t::object_t{};
        for (const auto& fieldName : fields) {
            auto it = result.get_object().find(fieldName);
            if (it != result.get_object().end()) {
                filtered.get_object()[fieldName] = it->second;
            }
        }
        return filtered;
    }

    return result;
}

bool ComponentSerializer::Deserialize(void* componentData,
                                        const std::string& typeName,
                                        const glz::json_t& data) {
    if (typeName == "Transform") {
        auto* transform = static_cast<Transform*>(componentData);
        if (!transform) {
            LOG_WARNING("ComponentSerializer", "Transform 组件数据为空");
            return false;
        }

        glz::json_t localData = data;
        auto& obj = localData.get_object();
        auto posIt = obj.find("position");
        if (posIt != obj.end()) {
            auto& p = posIt->second.get_array();
            transform->SetPosition(Vector3(
                static_cast<float>(p[0].get_number()),
                static_cast<float>(p[1].get_number()),
                static_cast<float>(p[2].get_number())
            ));
        }
        auto rotIt = obj.find("rotation");
        if (rotIt != obj.end()) {
            auto& r = rotIt->second.get_array();
            transform->SetRotation(Quaternion(
                static_cast<float>(r[3].get_number()), // w (GLM quat 构造: w,x,y,z)
                static_cast<float>(r[0].get_number()), // x
                static_cast<float>(r[1].get_number()), // y
                static_cast<float>(r[2].get_number())  // z
            ));
        }
        auto scaleIt = obj.find("scale");
        if (scaleIt != obj.end()) {
            auto& s = scaleIt->second.get_array();
            transform->SetScale(Vector3(
                static_cast<float>(s[0].get_number()),
                static_cast<float>(s[1].get_number()),
                static_cast<float>(s[2].get_number())
            ));
        }
        return true;
    }

    auto& registry = ComponentRegistry::Get();
    if (registry.CanSerialize(typeName)) {
        auto* comp = static_cast<Component*>(componentData);
        if (!comp) {
            LOG_WARNING("ComponentSerializer", "反序列化时组件数据为空: {}", typeName);
            return false;
        }

        std::string jsonStr;
        auto ec = glz::write_json(data, jsonStr);
        if (ec) {
            LOG_WARNING("ComponentSerializer", "序列化 JSON 数据失败: {}", typeName);
            return false;
        }

        return registry.DeserializeComponent(*comp, typeName, jsonStr);
    }

    LOG_WARNING("ComponentSerializer", "反序列化未注册的组件类型: {}", typeName);
    return false;
}

glz::json_t ComponentSerializer::GetFieldSchema(const std::string& typeName) {
    if (typeName == "Transform") {
        return glz::json_t::object_t{
            {"name", "Transform"},
            {"fields", glz::json_t::array_t{
                glz::json_t::object_t{{"name", "position"}, {"type", "float3"}, {"default", glz::json_t::array_t{0.0, 0.0, 0.0}}},
                glz::json_t::object_t{{"name", "rotation"}, {"type", "float4"}, {"default", glz::json_t::array_t{0.0, 0.0, 0.0, 1.0}}},
                glz::json_t::object_t{{"name", "scale"},    {"type", "float3"}, {"default", glz::json_t::array_t{1.0, 1.0, 1.0}}}
            }}
        };
    }

    auto& registry = ComponentRegistry::Get();
    if (registry.CanSerialize(typeName)) {
        return glz::json_t::object_t{
            {"name", typeName},
            {"fields", glz::json_t::array_t{
                glz::json_t::object_t{{"name", "data"}, {"type", "json"}, {"default", glz::json_t::object_t{}}}
            }}
        };
    }

    return glz::json_t::object_t{};
}

} // namespace MCP
} // namespace Prisma
