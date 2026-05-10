#pragma once

#include "ECS.h"
#include "math/MathTypes.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace Prisma {
    class Scene; // 前置声明 GameObject 模式的 Scene

namespace Core {

// 序列化格式
enum class SerializationFormat {
    JSON,
    Binary,
    XML
};

// 序列化器接口
class ISerializer {
public:
    virtual ~ISerializer() = default;

    // 序列化到字符串
    virtual std::string ToString() const = 0;

    // 从字符串反序列化
    virtual bool FromString(const std::string& data) = 0;

    // 保存到文件
    virtual bool SaveToFile(const std::string& filePath) const = 0;

    // 从文件加载
    virtual bool LoadFromFile(const std::string& filePath) = 0;
};

// JSON序列化器
class JsonSerializer : public ISerializer {
public:
    JsonSerializer();

    // 基础类型序列化
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

    // ISerializer 实现
    std::string ToString() const override;
    bool FromString(const std::string& data) override;
    bool SaveToFile(const std::string& filePath) const override;
    bool LoadFromFile(const std::string& filePath) override;

private:
    std::unique_ptr<class JsonDocument> m_document;
    void* m_currentNode;
};

// 二进制序列化器
class BinarySerializer : public ISerializer {
public:
    BinarySerializer();

    // 基础类型序列化
    void Write(const void* data, size_t size);
    void Read(void* data, size_t size);

    // 模板方法
    template<typename T>
    void Serialize(const T& value) {
        Write(&value, sizeof(T));
    }

    template<typename T>
    void Deserialize(T& value) {
        Read(&value, sizeof(T));
    }

    // ISerializer 实现
    std::string ToString() const override;
    bool FromString(const std::string& data) override;
    bool SaveToFile(const std::string& filePath) const override;
    bool LoadFromFile(const std::string& filePath) override;

private:
    std::vector<uint8_t> m_buffer;
    size_t m_readPosition;
};

// 场景序列化器 - 支持 GameObject 模式和 ECS 模式
class SceneSerializer {
public:
    SceneSerializer(Scene& scene);           // GameObject 模式
    SceneSerializer(ECS::World& world);      // ECS 模式

    // 保存场景
    bool SaveScene(const std::string& filePath, SerializationFormat format = SerializationFormat::JSON);

    // 加载场景
    bool LoadScene(const std::string& filePath, SerializationFormat format = SerializationFormat::JSON);

    // 注册具体组件的序列化函数 (用于 ECS 模式)
    template<typename T>
    void RegisterComponentSerializer();

private:
    Scene* m_scene = nullptr;
    ECS::World* m_world = nullptr;

    // GameObject 模式序列化逻辑
    void SerializeSceneGameObject(JsonSerializer& serializer);
    void DeserializeSceneGameObject(JsonSerializer& serializer);

    // ECS 模式序列化逻辑
    void SerializeSceneECS(JsonSerializer& serializer);
    void DeserializeSceneECS(JsonSerializer& serializer);

    // 组件序列化函数映射 (用于 ECS 模式)
    std::unordered_map<uint32_t, std::function<void(ECS::EntityID, JsonSerializer&)>> m_componentSerializers;
    std::unordered_map<uint32_t, std::function<void(ECS::EntityID, JsonSerializer&)>> m_componentDeserializers;

    void RegisterComponentSerializers();
};

// 资源序列化器
class ResourceSerializer {
public:
    // 保存资源
    template<typename T>
    static bool SaveResource(const T& resource, const std::string& filePath);

    // 加载资源
    template<typename T>
    static std::shared_ptr<T> LoadResource(const std::string& filePath);

private:
    // 资源类型注册
    static std::unordered_map<std::string, std::function<std::shared_ptr<void>(const std::string&)>> m_loaders;
    static std::unordered_map<std::string, std::function<bool(const void*, const std::string&)>> m_savers;
};

// 模板实现
template<typename T>
void SceneSerializer::RegisterComponentSerializer() {
    // 假设 T::TYPE_ID 存在，或者使用 typeid
    uint32_t typeID = (uint32_t)typeid(T).hash_code();
    const char* typeName = typeid(T).name();

    // 序列化函数
    m_componentSerializers[typeID] = [this, typeName](ECS::EntityID entity, JsonSerializer& serializer) {
        if (auto* component = m_world->GetComponent<T>(entity)) {
            serializer.BeginObject(typeName);
            // 假设组件有 enabled 属性
            // serializer.Serialize("enabled", true); 
            serializer.EndObject();
        }
    };

    // 反序列化函数
    m_componentDeserializers[typeID] = [this](ECS::EntityID entity, JsonSerializer& serializer) {
        m_world->AddComponent<T>(entity);
    };
}

} // namespace Core
} // namespace Prisma
