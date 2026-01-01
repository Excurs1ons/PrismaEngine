#include "AssetSerializer.h"
#include "Logger.h"
#include <filesystem>

namespace PrismaEngine {
    namespace Serialization {

        // OutputArchive模板特化实现
        template<>
        void OutputArchive::SerializeValue<bool>(const std::string& key, const bool& value) {
            SetCurrent(key);
            WriteBool(value);
        }

        template<>
        void OutputArchive::SerializeValue<int32_t>(const std::string& key, const int32_t& value) {
            SetCurrent(key);
            WriteInt32(value);
        }

        template<>
        void OutputArchive::SerializeValue<uint32_t>(const std::string& key, const uint32_t& value) {
            SetCurrent(key);
            WriteUInt32(value);
        }

        template<>
        void OutputArchive::SerializeValue<float>(const std::string& key, const float& value) {
            SetCurrent(key);
            WriteFloat(value);
        }

        template<>
        void OutputArchive::SerializeValue<double>(const std::string& key, const double& value) {
            SetCurrent(key);
            WriteDouble(value);
        }

        template<>
        void OutputArchive::SerializeValue<std::string>(const std::string& key, const std::string& value) {
            SetCurrent(key);
            WriteString(value);
        }

        template<>
        void OutputArchive::SerializeValue<std::filesystem::path>(const std::string& key, const std::filesystem::path& value) {
            SetCurrent(key);
            WriteString(value.string());
        }

        // InputArchive模板特化实现
        template<>
        void InputArchive::DeserializeValue<bool>(const std::string& key, bool& value) {
            SetCurrent(key);
            value = ReadBool();
        }

        template<>
        void InputArchive::DeserializeValue<int32_t>(const std::string& key, int32_t& value) {
            SetCurrent(key);
            value = ReadInt32();
        }

        template<>
        void InputArchive::DeserializeValue<uint32_t>(const std::string& key, uint32_t& value) {
            SetCurrent(key);
            value = ReadUInt32();
        }

        template<>
        void InputArchive::DeserializeValue<float>(const std::string& key, float& value) {
            SetCurrent(key);
            value = ReadFloat();
        }

        template<>
        void InputArchive::DeserializeValue<double>(const std::string& key, double& value) {
            SetCurrent(key);
            value = ReadDouble();
        }

        template<>
        void InputArchive::DeserializeValue<std::string>(const std::string& key, std::string& value) {
            SetCurrent(key);
            value = ReadString();
        }

        template<>
        void InputArchive::DeserializeValue<std::filesystem::path>(const std::string& key, std::filesystem::path& value) {
            SetCurrent(key);
            value = std::filesystem::path(ReadString());
        }

        // JsonOutputArchive的CommitValue方法实现
        void JsonOutputArchive::CommitValue() {
            if (!m_objectStack.empty()) {
                m_objectStack.back().get()[m_currentKey] = m_currentValue;
            } else if (!m_arrayStack.empty()) {
                m_arrayStack.back().get().push_back(m_currentValue);
            } else {
                m_json = m_currentValue;
            }
        }

    } // namespace Serialization
} // namespace Engine