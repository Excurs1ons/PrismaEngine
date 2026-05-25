#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Prisma {
namespace Core {

/* 编译期/运行期 FNV-1a 字符串哈希 */
class StringHash {
public:
    using HashType = uint32_t;

    // FNV-1a 32-bit constants
    static constexpr HashType FNV_OFFSET_BASIS = 2166136261U;
    static constexpr HashType FNV_PRIME = 16777619U;

    // 编译期哈希函数
    static constexpr HashType HashCompileTime(const char* str) {
        HashType hash = FNV_OFFSET_BASIS;
        while (*str) {
            hash ^= static_cast<HashType>(*str++);
            hash *= FNV_PRIME;
        }
        return hash;
    }

    // 运行期哈希函数
    static HashType Hash(std::string_view str) {
        HashType hash = FNV_OFFSET_BASIS;
        for (char c : str) {
            hash ^= static_cast<HashType>(c);
            hash *= FNV_PRIME;
        }
        return hash;
    }

    StringHash() : m_hash(0) {}
    StringHash(const char* str) : m_hash(Hash(str)) {}
    StringHash(const std::string& str) : m_hash(Hash(str)) {}
    StringHash(std::string_view str) : m_hash(Hash(str)) {}

    operator HashType() const { return m_hash; }
    HashType GetHash() const { return m_hash; }

    bool operator==(const StringHash& other) const { return m_hash == other.m_hash; }
    bool operator!=(const StringHash& other) const { return m_hash != other.m_hash; }

private:
    HashType m_hash;
};

} // namespace Core
} // namespace Prisma

// 字面量支持: "path/to/asset"_hash
constexpr Prisma::Core::StringHash::HashType operator""_hash(const char* str, size_t) {
    return Prisma::Core::StringHash::HashCompileTime(str);
}
