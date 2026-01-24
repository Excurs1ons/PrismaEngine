#pragma once
#include <cstdint>
#include <string>
#include "math/MathTypes.h"
// 移除了对Asset.h的循环引用
#include "MetaData.h"
#include "resource/Archive.h"
#include "graphic/interfaces/RenderTypes.h"  // 需要 BoundingBox 完整定义

namespace PrismaEngine {

class Asset;  // 前向声明

namespace Serialization {
class Serializable {
public:
    virtual ~Serializable()                              = default;
    virtual void Serialize(OutputArchive& archive) const = 0;
    virtual void Deserialize(InputArchive& archive)      = 0;
};

// 模板特化实现
#if defined(PRISMA_PLATFORM_WINDOWS) && defined(PRISMA_USING_DIRECTXMATH)
template <> inline void OutputArchive::SerializeValue(const std::string& key, const DirectX::XMFLOAT4& value) {
    BeginObject(1);
    SetCurrent(key);
    BeginArray(4);
    WriteFloat(value.w);
    WriteFloat(value.x);
    WriteFloat(value.y);
    WriteFloat(value.z);
    EndArray();
    EndObject();
}

template <> inline void OutputArchive::SerializeValue(const std::string& key, const DirectX::XMVECTORF32& value) {
    BeginObject(1);
    SetCurrent(key);
    BeginArray(4);
    WriteFloat(value.f[0]);
    WriteFloat(value.f[1]);
    WriteFloat(value.f[2]);
    WriteFloat(value.f[3]);
    EndArray();
    EndObject();
}

template <> inline void OutputArchive::SerializeValue(const std::string& key, const DirectX::BoundingBox& value) {
    BeginObject(1);
    SetCurrent(key);
    BeginObject(2);

    // Center
    SetCurrent("center");
    BeginArray(3);
    WriteFloat(value.Center.x);
    WriteFloat(value.Center.y);
    WriteFloat(value.Center.z);
    EndArray();

    // Extents
    SetCurrent("extents");
    BeginArray(3);
    WriteFloat(value.Extents.x);
    WriteFloat(value.Extents.y);
    WriteFloat(value.Extents.z);
    EndArray();

    EndObject();
    EndObject();
}
#endif  // defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_PLATFORM_ANDROID)

// GLM 类型序列化（无平台限制）
template <> inline void OutputArchive::SerializeValue(const std::string& key, const glm::vec4& value) {
    BeginObject(1);
    SetCurrent(key);
    BeginArray(4);
    WriteFloat(value.x);
    WriteFloat(value.y);
    WriteFloat(value.z);
    WriteFloat(value.w);
    EndArray();
    EndObject();
}

template <> inline void OutputArchive::SerializeValue(const std::string& key, const glm::vec3& value) {
    BeginObject(1);
    SetCurrent(key);
    BeginArray(3);
    WriteFloat(value.x);
    WriteFloat(value.y);
    WriteFloat(value.z);
    EndArray();
    EndObject();
}

template <> inline void OutputArchive::SerializeValue(const std::string& key, const glm::vec2& value) {
    BeginObject(1);
    SetCurrent(key);
    BeginArray(2);
    WriteFloat(value.x);
    WriteFloat(value.y);
    EndArray();
    EndObject();
}

// PrismaEngine::Graphic::BoundingBox 序列化
template <> inline void OutputArchive::SerializeValue(const std::string& key, const PrismaEngine::Graphic::BoundingBox& value) {
    BeginObject(1);
    SetCurrent(key);
    BeginObject(2);

    // Min bounds
    SetCurrent("minBounds");
    BeginArray(3);
    WriteFloat(value.minBounds.x);
    WriteFloat(value.minBounds.y);
    WriteFloat(value.minBounds.z);
    EndArray();

    // Max bounds
    SetCurrent("maxBounds");
    BeginArray(3);
    WriteFloat(value.maxBounds.x);
    WriteFloat(value.maxBounds.y);
    WriteFloat(value.maxBounds.z);
    EndArray();

    EndObject();
    EndObject();
}

template <> inline void OutputArchive::SerializeValue(const std::string& key, const Metadata& value) {
    BeginObject(1);
    SetCurrent(key);
    value.Serialize(*this);
    EndObject();
}

#if defined(PRISMA_PLATFORM_WINDOWS) && defined(PRISMA_USING_DIRECTXMATH)
template <> inline void InputArchive::DeserializeValue(const std::string& key, DirectX::XMFLOAT4& value) {
    BeginObject();
    EnterField(key);
    size_t size = BeginArray();
    if (size >= 4) {
        value.w = ReadFloat();
        value.x = ReadFloat();
        value.y = ReadFloat();
        value.z = ReadFloat();
    }
    EndArray();
    EndObject();
}

template <> inline void InputArchive::DeserializeValue(const std::string& key, DirectX::XMVECTORF32& value) {
    BeginObject();
    EnterField(key);
    size_t size = BeginArray();
    if (size >= 4) {
        value.f[0] = ReadFloat();
        value.f[1] = ReadFloat();
        value.f[2] = ReadFloat();
        value.f[3] = ReadFloat();
    }
    EndArray();
    EndObject();
}

template <> inline void InputArchive::DeserializeValue(const std::string& key, DirectX::BoundingBox& value) {
    BeginObject();
    EnterField(key);
    size_t fieldCount = BeginObject();

    for (size_t i = 0; i < fieldCount; ++i) {
        if (HasNextField("center")) {
            size_t size = BeginArray();
            if (size >= 3) {
                value.Center.x = ReadFloat();
                value.Center.y = ReadFloat();
                value.Center.z = ReadFloat();
            }
            EndArray();
        } else if (HasNextField("extents")) {
            size_t size = BeginArray();
            if (size >= 3) {
                value.Extents.x = ReadFloat();
                value.Extents.y = ReadFloat();
                value.Extents.z = ReadFloat();
            }
            EndArray();
        }
    }

    EndObject();
    EndObject();
}
#endif  // defined(PRISMA_PLATFORM_WINDOWS) && !defined(PRISMA_PLATFORM_ANDROID)

// GLM 类型反序列化（无平台限制）
template <> inline void InputArchive::DeserializeValue(const std::string& key, glm::vec4& value) {
    BeginObject();
    EnterField(key);
    BeginArray();
    value.x = ReadFloat();
    value.y = ReadFloat();
    value.z = ReadFloat();
    value.w = ReadFloat();
    EndArray();
    EndObject();
}

template <> inline void InputArchive::DeserializeValue(const std::string& key, glm::vec3& value) {
    BeginObject();
    EnterField(key);
    size_t size = BeginArray();
    if (size >= 3) {
        value.x = ReadFloat();
        value.y = ReadFloat();
        value.z = ReadFloat();
    }
    EndArray();
    EndObject();
}

template <> inline void InputArchive::DeserializeValue(const std::string& key, glm::vec2& value) {
    BeginObject();
    EnterField(key);
    size_t size = BeginArray();
    if (size >= 2) {
        value.x = ReadFloat();
        value.y = ReadFloat();
    }
    EndArray();
    EndObject();
}

// PrismaEngine::Graphic::BoundingBox 反序列化
template <> inline void InputArchive::DeserializeValue(const std::string& key, PrismaEngine::Graphic::BoundingBox& value) {
    BeginObject();
    EnterField(key);
    size_t fieldCount = BeginObject();

    for (size_t i = 0; i < fieldCount; ++i) {
        if (HasNextField("minBounds")) {
            size_t size = BeginArray();
            if (size >= 3) {
                value.minBounds.x = ReadFloat();
                value.minBounds.y = ReadFloat();
                value.minBounds.z = ReadFloat();
            }
            EndArray();
        } else if (HasNextField("maxBounds")) {
            size_t size = BeginArray();
            if (size >= 3) {
                value.maxBounds.x = ReadFloat();
                value.maxBounds.y = ReadFloat();
                value.maxBounds.z = ReadFloat();
            }
            EndArray();
        }
    }

    EndObject();
    EndObject();
}

template <> inline void InputArchive::DeserializeValue(const std::string& key, Metadata& value) {
    BeginObject();
    EnterField(key);
    value.Deserialize(*this);
    EndObject();
}
}  // namespace Serialization
}  // namespace PrismaEngine
