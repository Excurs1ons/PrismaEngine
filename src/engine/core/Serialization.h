#pragma once

#include "ECS.h"
#include "math/MathTypes.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <compare> // 确保 Glaze 依赖的 C++20 特性全局可见

namespace Prisma {
    class Scene; 

namespace Core {

enum class SerializationFormat { JSON, Binary, XML };

class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual std::string ToString() const = 0;
    virtual bool FromString(const std::string& data) = 0;
    virtual bool SaveToFile(const std::string& filePath) const = 0;
    virtual bool LoadFromFile(const std::string& filePath) = 0;
};

class JsonSerializer : public ISerializer {
public:
    JsonSerializer();
    ~JsonSerializer() override; 

    void BeginObject(const std::string& name = "");
    void EndObject();
    void BeginArray(const std::string& name = "");
    void EndArray();

    void Serialize(const std::string& key, bool value);
    void Serialize(const std::string& key, int32_t value);
    void Serialize(const std::string& key, uint32_t value);
    void Serialize(const std::string& key, float value);
    void Serialize(const std::string& key, double value);
    void Serialize(const std::string& key, const std::string& value);
    void Serialize(const std::string& key, const PrismaMath::vec3& value);
    void Serialize(const std::string& key, const PrismaMath::vec4& value);

    std::string ToString() const override { return ""; }
    bool FromString(const std::string& data) override { return true; }
    bool SaveToFile(const std::string& filePath) const override { return true; }
    bool LoadFromFile(const std::string& filePath) override { return true; }

private:
    struct JsonDocumentImpl; 
    std::unique_ptr<JsonDocumentImpl> m_pImpl;
    void* m_currentNode = nullptr;
};

class SceneSerializer {
public:
    SceneSerializer(Scene& scene);
    SceneSerializer(ECS::World& world);

    bool SaveScene(const std::string& filePath, SerializationFormat format = SerializationFormat::JSON);
    bool LoadScene(const std::string& filePath, SerializationFormat format = SerializationFormat::JSON);

    template<typename T>
    void RegisterComponentSerializer() {
        uint32_t typeID = (uint32_t)typeid(T).hash_code();
        m_componentSerializers[typeID] = [this](ECS::EntityID entity, JsonSerializer& serializer) {
            if (m_world) {
                if (auto* component = m_world->GetComponent<T>(entity)) {
                    serializer.BeginObject(typeid(T).name());
                    serializer.EndObject();
                }
            }
        };
        m_componentDeserializers[typeID] = [this](ECS::EntityID entity, JsonSerializer& serializer) {
            if (m_world) m_world->AddComponent<T>(entity);
        };
    }

private:
    Scene* m_scene = nullptr;
    ECS::World* m_world = nullptr;

    void SerializeSceneGameObject(JsonSerializer& serializer);
    void DeserializeSceneGameObject(JsonSerializer& serializer);
    void SerializeSceneECS(JsonSerializer& serializer);
    void DeserializeSceneECS(JsonSerializer& serializer);

    std::unordered_map<uint32_t, std::function<void(ECS::EntityID, JsonSerializer&)>> m_componentSerializers;
    std::unordered_map<uint32_t, std::function<void(ECS::EntityID, JsonSerializer&)>> m_componentDeserializers;

    void RegisterComponentSerializers();
};

} // namespace Core
} // namespace Prisma
