#include "Serialization.h"
#include "scene/Scene.h"
#include "graphic/CameraComponent.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Prisma::Core {

class JsonDocument {}; // JsonSerializer 使用的存根定义

// ========== JsonSerializer 实现 ==========

JsonSerializer::JsonSerializer() {}
JsonSerializer::~JsonSerializer() = default;

void JsonSerializer::BeginObject(const std::string& name) {}
void JsonSerializer::EndObject() {}
void JsonSerializer::BeginArray(const std::string& name) {}
void JsonSerializer::EndArray() {}
void JsonSerializer::Serialize(const std::string& key, bool value) {}
void JsonSerializer::Serialize(const std::string& key, int32_t value) {}
void JsonSerializer::Serialize(const std::string& key, uint32_t value) {}
void JsonSerializer::Serialize(const std::string& key, float value) {}
void JsonSerializer::Serialize(const std::string& key, double value) {}
void JsonSerializer::Serialize(const std::string& key, const std::string& value) {}
void JsonSerializer::Serialize(const std::string& key, const PrismaMath::vec3& value) {}
void JsonSerializer::Serialize(const std::string& key, const PrismaMath::vec4& value) {}
std::string JsonSerializer::ToString() const { return ""; }
bool JsonSerializer::FromString(const std::string& data) { return true; }
bool JsonSerializer::SaveToFile(const std::string& filePath) const { return true; }
bool JsonSerializer::LoadFromFile(const std::string& filePath) { return true; }

// ========== SceneSerializer 实现 ==========

SceneSerializer::SceneSerializer(Prisma::Scene& scene)
    : m_scene(&scene)
{
}

SceneSerializer::SceneSerializer(ECS::World& world)
    : m_world(&world)
{
}

bool SceneSerializer::SaveScene(const std::string& filePath, SerializationFormat format)
{
    if (format != SerializationFormat::JSON) {
        LOG_ERROR("Serialization", "目前仅支持 JSON 场景序列化");
        return false;
    }

    JsonSerializer serializer;
    if (m_scene) {
        SerializeSceneGameObject(serializer);
    } else if (m_world) {
        SerializeSceneECS(serializer);
    }

    return serializer.SaveToFile(filePath);
}

bool SceneSerializer::LoadScene(const std::string& filePath, SerializationFormat format)
{
    if (format != SerializationFormat::JSON) {
        LOG_ERROR("Serialization", "目前仅支持 JSON 场景序列化");
        return false;
    }

    JsonSerializer serializer;
    if (!serializer.LoadFromFile(filePath)) {
        return false;
    }

    if (m_scene) {
        DeserializeSceneGameObject(serializer);
    } else if (m_world) {
        DeserializeSceneECS(serializer);
    }

    return true;
}

void SceneSerializer::SerializeSceneGameObject(JsonSerializer& serializer)
{
    serializer.BeginObject("Scene");
    serializer.Serialize("Name", m_scene->GetName());

    serializer.BeginArray("GameObjects");
    for (auto& gameObject : m_scene->GetGameObjects()) {
        serializer.BeginObject();
        serializer.Serialize("Name", gameObject->name);
        
        // TODO: 序列化组件 (如 CameraComponent, SpriteRenderer)
        
        serializer.EndObject();
    }
    serializer.EndArray();
    serializer.EndObject();
}

void SceneSerializer::DeserializeSceneGameObject(JsonSerializer& serializer)
{
    // 假设 serializer 已经加载了文件内容
    LOG_INFO("Serialization", "正在从 JSON 反序列化 GameObject 场景...");
}

void SceneSerializer::SerializeSceneECS(JsonSerializer& serializer)
{
    LOG_INFO("Serialization", "正在序列化 ECS 世界...");
}

void SceneSerializer::DeserializeSceneECS(JsonSerializer& serializer)
{
    LOG_INFO("Serialization", "正在从 JSON 反序列化 ECS 场景...");
}

void SceneSerializer::RegisterComponentSerializers()
{
    // 这里注册所有需要支持序列化的 ECS 组件
}

} // namespace Prisma::Core
