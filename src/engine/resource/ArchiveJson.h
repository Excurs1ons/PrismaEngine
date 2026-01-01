#pragma once
#include "SerializationVersion.h"
#include "Archive.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace PrismaEngine {
    namespace Serialization {

        // JSON输出存档
        class JsonOutputArchive : public OutputArchive {
        public:
            explicit JsonOutputArchive() : m_json(json::object()) {}

            void WriteBool(bool value) override {
                m_currentValue = value;
            }

            void WriteInt32(int32_t value) override {
                m_currentValue = value;
            }

            void WriteUInt32(uint32_t value) override {
                m_currentValue = value;
            }

            void WriteFloat(float value) override {
                m_currentValue = value;
            }

            void WriteDouble(double value) override {
                m_currentValue = value;
            }

            void WriteString(const std::string& value) override {
                m_currentValue = value;
            }

            void BeginArray(size_t size) override {
                m_currentValue = json::array();
                m_arrayStack.push_back(std::ref(m_currentValue));
            }

            void EndArray() override {
                if (!m_arrayStack.empty()) {
                    m_arrayStack.pop_back();
                }
            }

            void BeginObject(size_t fieldCount = 0) override {
                m_currentValue = json::object();
                m_objectStack.push_back(std::ref(m_currentValue));
            }

            void EndObject() override {
                if (!m_objectStack.empty()) {
                    m_objectStack.pop_back();
                }
            }

            void SetCurrent(const std::string& key) override {
                m_currentKey = key;
            }

            // 获取最终的JSON对象
            const json& GetJson() const { return m_json; }

            // 将当前值添加到当前对象或数组中
            void CommitValue();

        private:
            json m_json;
            json m_currentValue;
            std::string m_currentKey;
            std::vector<std::reference_wrapper<json>> m_objectStack;
            std::vector<std::reference_wrapper<json>> m_arrayStack;
        };

        // JSON输入存档
        class JsonInputArchive : public InputArchive {
        public:
            explicit JsonInputArchive(const json& jsonData) : m_json(jsonData) {
                m_iteratorStack.push_back(m_json.begin());
                m_endIteratorStack.push_back(m_json.end());
            }

            bool ReadBool() override {
                return m_iteratorStack.back().value().get<bool>();
            }

            int32_t ReadInt32() override {
                return m_iteratorStack.back().value().get<int32_t>();
            }

            uint32_t ReadUInt32() override {
                return m_iteratorStack.back().value().get<uint32_t>();
            }

            float ReadFloat() override {
                return m_iteratorStack.back().value().get<float>();
            }

            double ReadDouble() override {
                return m_iteratorStack.back().value().get<double>();
            }

            std::string ReadString() override {
                return m_iteratorStack.back().value().get<std::string>();
            }

            size_t BeginArray() override {
                const auto& arrayValue = m_iteratorStack.back().value();
                if (!arrayValue.is_array()) {
                    throw SerializationException("Expected JSON array");
                }

                m_iteratorStack.push_back(arrayValue.begin());
                m_endIteratorStack.push_back(arrayValue.end());
                return arrayValue.size();
            }

            void EndArray() override {
                m_iteratorStack.pop_back();
                m_endIteratorStack.pop_back();
            }

            size_t BeginObject() override {
                const auto& objectValue = m_iteratorStack.back().value();
                if (!objectValue.is_object()) {
                    throw SerializationException("Expected JSON object");
                }

                m_iteratorStack.push_back(objectValue.begin());
                m_endIteratorStack.push_back(objectValue.end());
                return objectValue.size();
            }

            void EndObject() override {
                m_iteratorStack.pop_back();
                m_endIteratorStack.pop_back();
            }

            bool HasNextField(const std::string& expectedField = "") override {
                auto& it = m_iteratorStack.back();
                auto& endIt = m_endIteratorStack.back();

                if (it == endIt) {
                    return false;
                }

                if (!expectedField.empty()) {
                    // 查找特定字段
                    for (auto searchIt = it; searchIt != endIt; ++searchIt) {
                        if (searchIt.key() == expectedField) {
                            // 将迭代器移动到找到的字段
                            while (it != searchIt) {
                                ++it;
                            }
                            return true;
                        }
                    }
                    return false;
                }

                return true;
            }

            void SetCurrent(const std::string& key) override {
                // JSON中通过迭代器访问字段，此方法主要用于兼容性
            }

            std::string GetCurrentKey() const {
                return m_iteratorStack.back().key();
            }

        private:
            json m_json;
            std::vector<nlohmann::detail::iter_impl<const json>> m_iteratorStack;
            std::vector<nlohmann::detail::iter_impl<const json>> m_endIteratorStack;
        };
    }
}