#pragma once
#include "Serializable.h"
#include <vector>
#include <fstream>

namespace Prisma {
namespace Serialization {

/**
 * @brief 二进制输出存档
 */
class ENGINE_API BinaryOutputArchive : public OutputArchive {
public:
    void BeginObject(const std::string& name) override {}
    void EndObject() override {}

    void Write(const std::string& key, float value) override { WriteRaw(value); }
    void Write(const std::string& key, int32_t value) override { WriteRaw(value); }
    void Write(const std::string& key, uint32_t value) override { WriteRaw(value); }
    void Write(const std::string& key, bool value) override { WriteRaw(value); }
    void Write(const std::string& key, const std::string& value) override {
        uint32_t size = static_cast<uint32_t>(value.size());
        WriteRaw(size);
        m_data.insert(m_data.end(), value.begin(), value.end());
    }
    
    void Write(const std::string& key, const PrismaMath::vec2& value) override { WriteRaw(value); }
    void Write(const std::string& key, const PrismaMath::vec3& value) override { WriteRaw(value); }
    void Write(const std::string& key, const PrismaMath::vec4& value) override { WriteRaw(value); }
    void Write(const std::string& key, const PrismaMath::quat& value) override { WriteRaw(value); }

    const std::vector<uint8_t>& GetData() const { return m_data; }

private:
    template<typename T>
    void WriteRaw(const T& value) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
        m_data.insert(m_data.end(), p, p + sizeof(T));
    }

    std::vector<uint8_t> m_data;
};

/**
 * @brief 二进制输入存档
 */
class ENGINE_API BinaryInputArchive : public InputArchive {
public:
    explicit BinaryInputArchive(const std::vector<uint8_t>& data) : m_data(data), m_offset(0) {}

    void BeginObject(const std::string& name) override {}
    void EndObject() override {}

    bool Read(const std::string& key, float& value) override { return ReadRaw(value); }
    bool Read(const std::string& key, int32_t& value) override { return ReadRaw(value); }
    bool Read(const std::string& key, uint32_t& value) override { return ReadRaw(value); }
    bool Read(const std::string& key, bool& value) override { return ReadRaw(value); }
    bool Read(const std::string& key, std::string& value) override {
        uint32_t size = 0;
        if (!ReadRaw(size)) return false;
        if (m_offset + size > m_data.size()) return false;
        value.assign(reinterpret_cast<const char*>(&m_data[m_offset]), size);
        m_offset += size;
        return true;
    }
    
    bool Read(const std::string& key, PrismaMath::vec2& value) override { return ReadRaw(value); }
    bool Read(const std::string& key, PrismaMath::vec3& value) override { return ReadRaw(value); }
    bool Read(const std::string& key, PrismaMath::vec4& value) override { return ReadRaw(value); }
    bool Read(const std::string& key, PrismaMath::quat& value) override { return ReadRaw(value); }

private:
    template<typename T>
    bool ReadRaw(T& value) {
        if (m_offset + sizeof(T) > m_data.size()) return false;
        std::memcpy(&value, &m_data[m_offset], sizeof(T));
        m_offset += sizeof(T);
        return true;
    }

    const std::vector<uint8_t>& m_data;
    size_t m_offset;
};

} // namespace Serialization
} // namespace Prisma
