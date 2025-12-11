#pragma once
#include <cstdint>
#include <string>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
// 移除了对Asset.h的循环引用
#include "MetaData.h"
#include "resource/Archive.h"

namespace Engine {

class Asset;  // 前向声明

namespace Serialization {
class Serializable {
public:
    virtual ~Serializable()                              = default;
    virtual void Serialize(OutputArchive& archive) const = 0;
    virtual void Deserialize(InputArchive& archive)      = 0;
};

// 模板特化实现
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

template <> inline void OutputArchive::SerializeValue(const std::string& key, const Metadata& value) {
    BeginObject(1);
    SetCurrent(key);
    value.Serialize(*this);
    EndObject();
}

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

template <> inline void InputArchive::DeserializeValue(const std::string& key, Metadata& value) {
    BeginObject();
    EnterField(key);
    value.Deserialize(*this);
    EndObject();
}
}  // namespace Serialization
}  // namespace Engine