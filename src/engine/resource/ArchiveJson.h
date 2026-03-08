#pragma once
#include "SerializationVersion.h"
#include "Archive.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace PrismaEngine {
    namespace Serialization {

        // JSON输出存档
        class ENGINE_API JsonOutputArchive : public OutputArchive {
        public:
            explicit JsonOutputArchive() : m_json(json::object()) {}

            void WriteBool(bool value) override { m_currentValue = value; CommitValue(); }
            void WriteInt32(int32_t value) override { m_currentValue = value; CommitValue(); }
            void WriteUInt32(uint32_t value) override { m_currentValue = value; CommitValue(); }
            void WriteFloat(float value) override { m_currentValue = value; CommitValue(); }
            void WriteDouble(double value) override { m_currentValue = value; CommitValue(); }
            void WriteString(const std::string& value) override { m_currentValue = value; CommitValue(); }

            // 带 Key 版本
            void BeginArray(const std::string& key, uint32_t& size) override {
                (void)size;
                m_currentKey = key;
                m_currentValue = json::array();
                m_arrayStack.push_back(std::ref(m_currentValue));
            }
            void BeginObject(const std::string& key) override {
                m_currentKey = key;
                m_currentValue = json::object();
                m_objectStack.push_back(std::ref(m_currentValue));
            }

            // 兼容旧代码版本
            void BeginArray(uint32_t size) override {
                (void)size;
                m_currentValue = json::array();
                m_arrayStack.push_back(std::ref(m_currentValue));
            }
            void BeginObject(size_t fieldCount = 0) override {
                (void)fieldCount;
                m_currentValue = json::object();
                m_objectStack.push_back(std::ref(m_currentValue));
            }

            void EndArray() override { if (!m_arrayStack.empty()) m_arrayStack.pop_back(); }
            void EndObject() override { if (!m_objectStack.empty()) m_objectStack.pop_back(); }

            void SetCurrent(const std::string& key) override { m_currentKey = key; }
            void EnterField(const std::string& key) override { m_currentKey = key; }

            const json& GetJson() const { return m_json; }
            void CommitValue();

        private:
            json m_json;
            json m_currentValue;
            std::string m_currentKey;
            std::vector<std::reference_wrapper<json>> m_objectStack;
            std::vector<std::reference_wrapper<json>> m_arrayStack;
        };

        // JSON输入存档
        class ENGINE_API JsonInputArchive : public InputArchive {
        public:
            explicit JsonInputArchive(const json& jsonData) : m_json(jsonData) {
                m_iteratorStack.push_back(m_json.begin());
                m_endIteratorStack.push_back(m_json.end());
            }

            bool ReadBool() override { return m_iteratorStack.back().value().get<bool>(); }
            int32_t ReadInt32() override { return m_iteratorStack.back().value().get<int32_t>(); }
            uint32_t ReadUInt32() override { return m_iteratorStack.back().value().get<uint32_t>(); }
            float ReadFloat() override { return m_iteratorStack.back().value().get<float>(); }
            double ReadDouble() override { return m_iteratorStack.back().value().get<double>(); }
            std::string ReadString() override { return m_iteratorStack.back().value().get<std::string>(); }

            // 带 Key 版本
            void BeginArray(const std::string& key, uint32_t& size) override {
                const auto& val = m_iteratorStack.back().value()[key];
                size = static_cast<uint32_t>(val.size());
                m_iteratorStack.push_back(val.begin());
                m_endIteratorStack.push_back(val.end());
            }
            void BeginObject(const std::string& key) override {
                const auto& val = m_iteratorStack.back().value()[key];
                m_iteratorStack.push_back(val.begin());
                m_endIteratorStack.push_back(val.end());
            }

            // 兼容旧代码版本
            size_t BeginArray() override {
                const auto& val = m_iteratorStack.back().value();
                m_iteratorStack.push_back(val.begin());
                m_endIteratorStack.push_back(val.end());
                return val.size();
            }
            size_t BeginObject() override {
                const auto& val = m_iteratorStack.back().value();
                m_iteratorStack.push_back(val.begin());
                m_endIteratorStack.push_back(val.end());
                return val.size();
            }

            void EndArray() override { m_iteratorStack.pop_back(); m_endIteratorStack.pop_back(); }
            void EndObject() override { m_iteratorStack.pop_back(); m_endIteratorStack.pop_back(); }

            bool HasNextField() override { return m_iteratorStack.back() != m_endIteratorStack.back(); }
            bool HasNextField(const std::string& expectedField) override {
                (void)expectedField;
                return HasNextField();
            }
            void SetCurrent(const std::string& key) override { (void)key; }
            void EnterField(const std::string& key) override { (void)key; }

        private:
            json m_json;
            std::vector<nlohmann::detail::iter_impl<const json>> m_iteratorStack;
            std::vector<nlohmann::detail::iter_impl<const json>> m_endIteratorStack;
        };
    }
}
