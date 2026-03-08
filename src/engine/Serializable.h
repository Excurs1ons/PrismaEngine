#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
#include "resource/Archive.h"

namespace PrismaEngine {
    namespace Serialization {

        /// <summary>
        /// 可序列化对象接口
        /// </summary>
        class ENGINE_API Serializable {
        public:
            virtual ~Serializable() = default;
            virtual void Serialize(OutputArchive& archive) const = 0;
            virtual void Deserialize(InputArchive& archive) = 0;
        };

        // --- 基础数学类型的序列化实现 ---

        template <> inline void OutputArchive::SerializeValue(const std::string& key, const DirectX::XMFLOAT3& value) {
            SetCurrent(key);
            uint32_t size = 3;
            BeginArray(key, size);
            WriteFloat(value.x);
            WriteFloat(value.y);
            WriteFloat(value.z);
            EndArray();
        }

        template <> inline void InputArchive::DeserializeValue(const std::string& key, DirectX::XMFLOAT3& value) {
            uint32_t size = 0;
            BeginArray(key, size);
            if (size >= 3) {
                value.x = ReadFloat();
                value.y = ReadFloat();
                value.z = ReadFloat();
            }
            EndArray();
        }

        template <> inline void OutputArchive::SerializeValue(const std::string& key, const DirectX::XMFLOAT4& value) {
            SetCurrent(key);
            uint32_t size = 4;
            BeginArray(key, size);
            WriteFloat(value.x);
            WriteFloat(value.y);
            WriteFloat(value.z);
            WriteFloat(value.w);
            EndArray();
        }

        template <> inline void InputArchive::DeserializeValue(const std::string& key, DirectX::XMFLOAT4& value) {
            uint32_t size = 0;
            BeginArray(key, size);
            if (size >= 4) {
                value.x = ReadFloat();
                value.y = ReadFloat();
                value.z = ReadFloat();
                value.w = ReadFloat();
            }
            EndArray();
        }

        // --- 泛型序列化分发逻辑 (必须在 Serializable 之后) ---

        template<typename T>
        inline void OutputArchive::SerializeValue(const std::string& key, const T& value) {
            BeginObject(key);
            value.Serialize(*this);
            EndObject();
        }

        template<typename T>
        inline void InputArchive::DeserializeValue(const std::string& key, T& value) {
            BeginObject(key);
            value.Deserialize(*this);
            EndObject();
        }

    } // namespace Serialization
} // namespace PrismaEngine
