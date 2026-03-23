#pragma once
#include "math/MathTypes.h"
#include <string>
#include <vector>
#include <filesystem>

#include "Export.h"

namespace Prisma {
namespace Serialization {

/// <summary>
/// 可序列化对象接口
/// </summary>
class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void Serialize(class OutputArchive& archive) const = 0;
    virtual void Deserialize(class InputArchive& archive) = 0;
};

/// <summary>
/// 输出存档基类
/// </summary>
class ENGINE_API OutputArchive {
public:
    virtual ~OutputArchive() = default;

    virtual void BeginObject(const std::string& name) = 0;
    virtual void EndObject() = 0;

    virtual void Write(const std::string& key, float value) = 0;
    virtual void Write(const std::string& key, int32_t value) = 0;
    virtual void Write(const std::string& key, uint32_t value) = 0;
    virtual void Write(const std::string& key, bool value) = 0;
    virtual void Write(const std::string& key, const std::string& value) = 0;
    virtual void Write(const std::string& key, const PrismaMath::vec2& value) = 0;
    virtual void Write(const std::string& key, const PrismaMath::vec3& value) = 0;
    virtual void Write(const std::string& key, const PrismaMath::vec4& value) = 0;
    virtual void Write(const std::string& key, const PrismaMath::quat& value) = 0;

    virtual void SetCurrent(const std::string& key) { m_currentKey = key; }

    template <typename T>
    void operator()(const std::string& key, const T& value) {
        SerializeValue(key, value);
    }

    template <typename T>
    void SerializeValue(const std::string& key, const T& value) {
        if constexpr (std::is_same_v<T, float>) Write(key, value);
        else if constexpr (std::is_same_v<T, int32_t>) Write(key, value);
        else if constexpr (std::is_same_v<T, uint32_t>) Write(key, value);
        else if constexpr (std::is_same_v<T, bool>) Write(key, value);
        else if constexpr (std::is_same_v<T, std::string>) Write(key, value);
        else if constexpr (std::is_same_v<T, std::filesystem::path>) Write(key, value.string());
        else if constexpr (std::is_same_v<T, PrismaMath::vec2>) Write(key, value);
        else if constexpr (std::is_same_v<T, PrismaMath::vec3>) Write(key, value);
        else if constexpr (std::is_same_v<T, PrismaMath::vec4>) Write(key, value);
        else if constexpr (std::is_same_v<T, PrismaMath::quat>) Write(key, value);
        else {
            BeginObject(key);
            value.Serialize(*this);
            EndObject();
        }
    }

protected:
    std::string m_currentKey;
};

/// <summary>
/// 输入存档基类
/// </summary>
class ENGINE_API InputArchive {
public:
    virtual ~InputArchive() = default;

    virtual void BeginObject(const std::string& name) = 0;
    virtual void EndObject() = 0;

    virtual bool Read(const std::string& key, float& value) = 0;
    virtual bool Read(const std::string& key, int32_t& value) = 0;
    virtual bool Read(const std::string& key, uint32_t& value) = 0;
    virtual bool Read(const std::string& key, bool& value) = 0;
    virtual bool Read(const std::string& key, std::string& value) = 0;
    virtual bool Read(const std::string& key, PrismaMath::vec2& value) = 0;
    virtual bool Read(const std::string& key, PrismaMath::vec3& value) = 0;
    virtual bool Read(const std::string& key, PrismaMath::vec4& value) = 0;
    virtual bool Read(const std::string& key, PrismaMath::quat& value) = 0;

    virtual void SetCurrent(const std::string& key) { m_currentKey = key; }

    template <typename T>
    void operator()(const std::string& key, T& value) {
        DeserializeValue(key, value);
    }

    template <typename T>
    void DeserializeValue(const std::string& key, T& value) {
        if constexpr (std::is_same_v<T, float>) Read(key, value);
        else if constexpr (std::is_same_v<T, int32_t>) Read(key, value);
        else if constexpr (std::is_same_v<T, uint32_t>) Read(key, value);
        else if constexpr (std::is_same_v<T, bool>) Read(key, value);
        else if constexpr (std::is_same_v<T, std::string>) Read(key, value);
        else if constexpr (std::is_same_v<T, std::filesystem::path>) {
            std::string s;
            if (Read(key, s)) value = std::filesystem::path(s);
        }
        else if constexpr (std::is_same_v<T, PrismaMath::vec2>) Read(key, value);
        else if constexpr (std::is_same_v<T, PrismaMath::vec3>) Read(key, value);
        else if constexpr (std::is_same_v<T, PrismaMath::vec4>) Read(key, value);
        else if constexpr (std::is_same_v<T, PrismaMath::quat>) Read(key, value);
        else {
            BeginObject(key);
            value.Deserialize(*this);
            EndObject();
        }
    }

protected:
    std::string m_currentKey;
};

}  // namespace Serialization
}  // namespace Prisma
