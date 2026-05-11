#include "Serialization.h"
#include "scene/Scene.h"
#include "graphic/CameraComponent.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Prisma::Core {

struct JsonSerializer::JsonDocumentImpl {};

JsonSerializer::JsonSerializer() : m_pImpl(std::make_unique<JsonDocumentImpl>()) {}
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

SceneSerializer::SceneSerializer(Prisma::Scene& scene) : m_scene(&scene) {}
SceneSerializer::SceneSerializer(ECS::World& world) : m_world(&world) {}

bool SceneSerializer::SaveScene(const std::string& filePath, SerializationFormat format) {
    JsonSerializer s;
    if (m_scene) SerializeSceneGameObject(s);
    else if (m_world) SerializeSceneECS(s);
    return true;
}

bool SceneSerializer::LoadScene(const std::string& filePath, SerializationFormat format) {
    JsonSerializer s;
    if (m_scene) DeserializeSceneGameObject(s);
    else if (m_world) DeserializeSceneECS(s);
    return true;
}

void SceneSerializer::SerializeSceneGameObject(JsonSerializer& s) {
    s.BeginObject("Scene");
    if (m_scene) s.Serialize("Name", m_scene->GetName());
    s.EndObject();
}

void SceneSerializer::DeserializeSceneGameObject(JsonSerializer& s) {
    LOG_DEBUG("Serialization", "正在反序列化 GameObject 场景...");
}

void SceneSerializer::SerializeSceneECS(JsonSerializer& s) {
    LOG_DEBUG("Serialization", "正在序列化 ECS 世界...");
}

void SceneSerializer::DeserializeSceneECS(JsonSerializer& s) {
    LOG_DEBUG("Serialization", "正在反序列化 ECS 场景...");
}

void SceneSerializer::RegisterComponentSerializers() {}

} // namespace Prisma::Core
