#pragma once
#include <cstdint>
#include <string>
#include <filesystem>
#include <vector>
#include <map>
#include "../Export.h"

namespace PrismaEngine {
    namespace Serialization {

        class OutputArchive;
        class InputArchive;

        /// <summary>
        /// 可序列化对象接口 (前置声明)
        /// </summary>
        class ENGINE_API Serializable;

        /// <summary>
        /// 输出存档接口
        /// </summary>
        class ENGINE_API OutputArchive {
        public:
            virtual ~OutputArchive() = default;

            template<typename T>
            void operator()(const std::string& key, const T& value) {
                SerializeValue(key, value);
            }

            // 基础类型特化由 cpp 提供，自定义类型通过此模板分发
            template<typename T>
            void SerializeValue(const std::string& key, const T& value);
            
            virtual void WriteBool(bool value) = 0;
            virtual void WriteInt32(int32_t value) = 0;
            virtual void WriteUInt32(uint32_t value) = 0;
            virtual void WriteFloat(float value) = 0;
            virtual void WriteDouble(double value) = 0;
            virtual void WriteString(const std::string& value) = 0;
            virtual void SetCurrent(const std::string& key) = 0;

            virtual void BeginObject(const std::string& key) = 0;
            virtual void BeginArray(const std::string& key, uint32_t& size) = 0;
            virtual void BeginObject(size_t fieldCount = 0) = 0;
            virtual void BeginArray(uint32_t size) = 0;
            
            virtual void EndObject() = 0;
            virtual void EndArray() = 0;
            virtual void EnterField(const std::string& key) = 0;
        };

        /// <summary>
        /// 输入存档接口
        /// </summary>
        class ENGINE_API InputArchive {
        public:
            virtual ~InputArchive() = default;

            template<typename T>
            void operator()(const std::string& key, T& value) {
                DeserializeValue(key, value);
            }

            template<typename T>
            void DeserializeValue(const std::string& key, T& value);

            virtual bool ReadBool() = 0;
            virtual int32_t ReadInt32() = 0;
            virtual uint32_t ReadUInt32() = 0;
            virtual float ReadFloat() = 0;
            virtual double ReadDouble() = 0;
            virtual std::string ReadString() = 0;
            virtual void SetCurrent(const std::string& key) = 0;

            virtual void BeginObject(const std::string& key) = 0;
            virtual void BeginArray(const std::string& key, uint32_t& size) = 0;
            virtual size_t BeginObject() = 0;
            virtual size_t BeginArray() = 0;

            virtual void EndObject() = 0;
            virtual void EndArray() = 0;
            virtual bool HasNextField() = 0;
            virtual bool HasNextField(const std::string& expectedField) = 0;
            virtual void EnterField(const std::string& key) = 0;
        };

        // 基础类型特化声明
        template<> ENGINE_API void OutputArchive::SerializeValue<bool>(const std::string& key, const bool& value);
        template<> ENGINE_API void OutputArchive::SerializeValue<int32_t>(const std::string& key, const int32_t& value);
        template<> ENGINE_API void OutputArchive::SerializeValue<uint32_t>(const std::string& key, const uint32_t& value);
        template<> ENGINE_API void OutputArchive::SerializeValue<float>(const std::string& key, const float& value);
        template<> ENGINE_API void OutputArchive::SerializeValue<double>(const std::string& key, const double& value);
        template<> ENGINE_API void OutputArchive::SerializeValue<std::string>(const std::string& key, const std::string& value);
        template<> ENGINE_API void OutputArchive::SerializeValue<std::filesystem::path>(const std::string& key, const std::filesystem::path& value);

        template<> ENGINE_API void InputArchive::DeserializeValue<bool>(const std::string& key, bool& value);
        template<> ENGINE_API void InputArchive::DeserializeValue<int32_t>(const std::string& key, int32_t& value);
        template<> ENGINE_API void InputArchive::DeserializeValue<uint32_t>(const std::string& key, uint32_t& value);
        template<> ENGINE_API void InputArchive::DeserializeValue<float>(const std::string& key, float& value);
        template<> ENGINE_API void InputArchive::DeserializeValue<double>(const std::string& key, double& value);
        template<> ENGINE_API void InputArchive::DeserializeValue<std::string>(const std::string& key, std::string& value);
        template<> ENGINE_API void InputArchive::DeserializeValue<std::filesystem::path>(const std::string& key, std::filesystem::path& value);

    } // namespace Serialization
} // namespace PrismaEngine
