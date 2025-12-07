#pragma once
#include <cstdint>
#include <string>
#include <filesystem>

namespace Engine {
    namespace Serialization {
        class InputArchive {
        public:
            virtual ~InputArchive() = default;

            // 基础类型反序列化
            virtual bool ReadBool() = 0;
            virtual int32_t ReadInt32() = 0;
            virtual uint32_t ReadUInt32() = 0;
            virtual float ReadFloat() = 0;
            virtual double ReadDouble() = 0;
            virtual std::string ReadString() = 0;

            // 容器反序列化
            virtual size_t BeginArray() = 0;  // 返回数组大小
            virtual void EndArray() = 0;
            virtual size_t BeginObject() = 0; // 返回字段数量
            virtual void EndObject() = 0;

            // 检查下一个字段
            virtual bool HasNextField(const std::string& expectedField = "") = 0;
            // 进入字段
            void EnterField(const std::string& field);
            // 设置当前字段（用于JSON等格式）
            virtual void SetCurrent(const std::string& key);

            template<typename T>
            void operator()(const std::string& key, T& value) {
                DeserializeValue(key, value);
            }

        private:
            template<typename T>
            void DeserializeValue(const std::string& key, T& value);
        };
        
        // 特化声明
        template<>
        void InputArchive::DeserializeValue<bool>(const std::string& key, bool& value);
        template<>
        void InputArchive::DeserializeValue<int32_t>(const std::string& key, int32_t& value);
        template<>
        void InputArchive::DeserializeValue<uint32_t>(const std::string& key, uint32_t& value);
        template<>
        void InputArchive::DeserializeValue<float>(const std::string& key, float& value);
        template<>
        void InputArchive::DeserializeValue<double>(const std::string& key, double& value);
        template<>
        void InputArchive::DeserializeValue<std::string>(const std::string& key, std::string& value);
        template<>
        void InputArchive::DeserializeValue<std::filesystem::path>(const std::string& key, std::filesystem::path& value);

        // 序列化接口
        class OutputArchive {
        public:
            virtual ~OutputArchive() = default;

            // 基础类型序列化
            virtual void WriteBool(bool value) = 0;
            virtual void WriteInt32(int32_t value) = 0;
            virtual void WriteUInt32(uint32_t value) = 0;
            virtual void WriteFloat(float value) = 0;
            virtual void WriteDouble(double value) = 0;
            virtual void WriteString(const std::string& value) = 0;

            // 容器序列化
            virtual void BeginArray(size_t size) = 0;
            virtual void EndArray() = 0;
            virtual void BeginObject(size_t fieldCount = 0) = 0;
            virtual void EndObject() = 0;
            // 设置当前字段（用于JSON等格式）
            virtual void SetCurrent(const std::string& key);
            // 便捷操作符
            template<typename T>
            void operator()(const std::string& key, const T& value) {
                // 在头文件中提供默认实现，特化在cpp文件中
                SerializeValue(key, value);
            }

        private:
            // 私有序列化方法，在cpp文件中特化
            template<typename T>
            void SerializeValue(const std::string& key, const T& value);
        };
        
        // 特化声明
        template<>
        void OutputArchive::SerializeValue<bool>(const std::string& key, const bool& value);
        template<>
        void OutputArchive::SerializeValue<int32_t>(const std::string& key, const int32_t& value);
        template<>
        void OutputArchive::SerializeValue<uint32_t>(const std::string& key, const uint32_t& value);
        template<>
        void OutputArchive::SerializeValue<float>(const std::string& key, const float& value);
        template<>
        void OutputArchive::SerializeValue<double>(const std::string& key, const double& value);
        template<>
        void OutputArchive::SerializeValue<std::string>(const std::string& key, const std::string& value);
        template<>
        void OutputArchive::SerializeValue<std::filesystem::path>(const std::string& key, const std::filesystem::path& value);
    }
}