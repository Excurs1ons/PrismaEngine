#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Prisma::Math {

// MurmurHash3 128位版本 (针对 x64 优化)
struct Hash128 {
    uint64_t low;
    uint64_t high;

    bool operator==(const Hash128& other) const {
        return low == other.low && high == other.high;
    }

    bool operator!=(const Hash128& other) const {
        return !(*this == other);
    }

    std::string ToString() const;
    static Hash128 FromString(const std::string& str);
};

class MurmurHash3 {
public:
    static Hash128 Hash(const void* key, int len, uint32_t seed = 42);
    static Hash128 HashString(const std::string& str, uint32_t seed = 42);
    static Hash128 HashFile(const std::string& path, uint32_t seed = 42);
};

} // namespace Prisma::Math
