#pragma once
#include "SerializationVersion.h"
#include "Archive.h"
#include <vector>
#include <fstream>

namespace PrismaEngine {
    namespace Serialization {

        // 二进制输出存档
        class BinaryOutputArchive : public OutputArchive {
        public:
            explicit BinaryOutputArchive(std::ostream& stream) : m_stream(stream) {}

            void WriteBool(bool value) override { m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value)); }
            void WriteInt32(int32_t value) override { m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value)); }
            void WriteUInt32(uint32_t value) override { m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value)); }
            void WriteFloat(float value) override { m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value)); }
            void WriteDouble(double value) override { m_stream.write(reinterpret_cast<const char*>(&value), sizeof(value)); }
            void WriteString(const std::string& value) override {
                uint32_t size = static_cast<uint32_t>(value.size());
                WriteUInt32(size);
                m_stream.write(value.data(), size);
            }

            // 带 Key 版本
            void BeginArray(const std::string& /*key*/, uint32_t& size) override { WriteUInt32(size); }
            void BeginObject(const std::string& /*key*/) override {}

            // 兼容旧代码版本
            void BeginArray(uint32_t size) override { WriteUInt32(size); }
            void BeginObject(size_t /*fieldCount*/ = 0) override {}

            void EndArray() override {}
            void EndObject() override {}
            void SetCurrent(const std::string& /*key*/) override {}
            void EnterField(const std::string& /*key*/) override {}

        private:
            std::ostream& m_stream;
        };

        // 二进制输入存档
        class BinaryInputArchive : public InputArchive {
        public:
            explicit BinaryInputArchive(std::istream& stream) : m_stream(stream) {}

            bool ReadBool() override { bool v; m_stream.read(reinterpret_cast<char*>(&v), sizeof(v)); return v; }
            int32_t ReadInt32() override { int32_t v; m_stream.read(reinterpret_cast<char*>(&v), sizeof(v)); return v; }
            uint32_t ReadUInt32() override { uint32_t v; m_stream.read(reinterpret_cast<char*>(&v), sizeof(v)); return v; }
            float ReadFloat() override { float v; m_stream.read(reinterpret_cast<char*>(&v), sizeof(v)); return v; }
            double ReadDouble() override { double v; m_stream.read(reinterpret_cast<char*>(&v), sizeof(v)); return v; }
            std::string ReadString() override {
                uint32_t size = ReadUInt32();
                std::string v(size, '\0');
                m_stream.read(&v[0], size);
                return v;
            }

            // 带 Key 版本
            void BeginArray(const std::string& /*key*/, uint32_t& size) override { size = ReadUInt32(); }
            void BeginObject(const std::string& /*key*/) override {}

            // 兼容旧代码版本
            size_t BeginArray() override { return static_cast<size_t>(ReadUInt32()); }
            size_t BeginObject() override { return 0; }

            void EndArray() override {}
            void EndObject() override {}
            bool HasNextField() override { return !m_stream.eof(); }
            bool HasNextField(const std::string& /*expectedField*/) override { return HasNextField(); }
            void SetCurrent(const std::string& /*key*/) override {}
            void EnterField(const std::string& /*key*/) override {}

        private:
            std::istream& m_stream;
        };
    }
}
