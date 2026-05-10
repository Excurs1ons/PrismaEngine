#include "Serialization.h"
#include "scene/Scene.h"
#include "graphic/CameraComponent.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace Prisma::Core {

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
    
    // 这里应该是真正的解析逻辑，调用 serializer 的接口读取数据
    // 由于 JsonSerializer 目前是存根，我们先记录意图
    
    // 1. 读取场景名称
    // std::string sceneName = serializer.ReadString("Name");
    // m_scene->SetName(sceneName);

    // 2. 遍历 GameObjects 数组
    // ...
}

void SceneSerializer::SerializeSceneECS(JsonSerializer& serializer)
{
    LOG_INFO("Serialization", "正在序列化 ECS 世界...");
}

void SceneSerializer::DeserializeSceneECS(JsonSerializer& serializer)
{
    LOG_INFO("Serialization", "正在从 JSON 反序列化 ECS 场景...");
}

// ========== JsonSerializer 存根实现 (为了让编译通过) ==========

JsonSerializer::JsonSerializer() {}
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
void JsonSerializer::Serialize(const std::string& key, const DirectX::XMFLOAT3& value) {}
void JsonSerializer::Serialize(const std::string& key, const DirectX::XMFLOAT4& value) {}
std::string JsonSerializer::ToString() const { return ""; }
bool JsonSerializer::FromString(const std::string& data) { return true; }
bool JsonSerializer::SaveToFile(const std::string& filePath) const { return true; }
bool JsonSerializer::LoadFromFile(const std::string& filePath) { return true; }

} // namespace Prisma::Core
