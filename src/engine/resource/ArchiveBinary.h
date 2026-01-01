#pragma once
#include "Archive.h"
#include <istream>

namespace PrismaEngine {
    namespace Serialization {

        // 二进制输出存档
        class BinaryOutputArchive : public OutputArchive {
        public:
            explicit BinaryOutputArchive(std::ostream& stream) : m_stream(stream) {}

            void WriteBool(bool value) override {
                m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }

            void WriteInt32(int32_t value) override {
                m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }

            void WriteUInt32(uint32_t value) override {
                m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }

            void WriteFloat(float value) override {
                m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }

            void WriteDouble(double value) override {
                m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }

            void WriteString(const std::string& value) override {
                uint32_t length = static_cast<uint32_t>(value.length());
                WriteUInt32(length);
                m_stream.write(value.c_str(), length);
            }

            void BeginArray(size_t size) override {
                WriteUInt32(static_cast<uint32_t>(size));
            }

            void EndArray() override {
                // 二进制格式中数组结束不需要特殊标记
            }

            void BeginObject(size_t fieldCount = 0) override {
                WriteUInt32(static_cast<uint32_t>(fieldCount));
            }

            void EndObject() override {
                // 二进制格式中对象结束不需要特殊标记
            }

        private:
            std::ostream& m_stream;
        };

        // 二进制输入存档
        class BinaryInputArchive : public InputArchive {
        public:
            explicit BinaryInputArchive(std::istream& stream) : m_stream(stream) {}

            bool ReadBool() override {
                bool value;
                m_stream.read(reinterpret_cast<char*>(&value), sizeof(value));
                return value;
            }

            int32_t ReadInt32() override {
                int32_t value;
                m_stream.read(reinterpret_cast<char*>(&value), sizeof(value));
                return value;
            }

            uint32_t ReadUInt32() override {
                uint32_t value;
                m_stream.read(reinterpret_cast<char*>(&value), sizeof(value));
                return value;
            }

            float ReadFloat() override {
                float value;
                m_stream.read(reinterpret_cast<char*>(&value), sizeof(value));
                return value;
            }

            double ReadDouble() override {
                double value;
                m_stream.read(reinterpret_cast<char*>(&value), sizeof(value));
                return value;
            }

            std::string ReadString() override {
                uint32_t length = ReadUInt32();
                std::string value(length, '\0');
                m_stream.read(&value[0], length);
                return value;
            }

            size_t BeginArray() override {
                return static_cast<size_t>(ReadUInt32());
            }

            void EndArray() override {
                // 二进制格式中数组结束不需要特殊标记
            }

            size_t BeginObject() override {
                return static_cast<size_t>(ReadUInt32());
            }

            void EndObject() override {
                // 二进制格式中对象结束不需要特殊标记
            }

            bool HasNextField(const std::string& expectedField = "") override {
                // 二进制格式中字段顺序是固定的，不支持动态字段检查
                return true;
            }

        private:
            std::istream& m_stream;
        };
    }
}