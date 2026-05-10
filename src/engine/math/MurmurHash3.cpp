#include "MurmurHash3.h"
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace Prisma::Math {

// --- MurmurHash3 Core (x64_128) ---

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

Hash128 MurmurHash3::Hash(const void* key, int len, uint32_t seed) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks   = len / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const uint64_t c1 = 0x87c37b91114253d5ULL;
    const uint64_t c2 = 0x4cf5ad432745937fULL;

    // --- body ---
    const uint64_t* blocks = (const uint64_t*)(data);

    for (int i = 0; i < nblocks; i++) {
        uint64_t k1 = blocks[i * 2 + 0];
        uint64_t k2 = blocks[i * 2 + 1];

        k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
        h1 = rotl64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52dce729;

        k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;
        h2 = rotl64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
    }

    // --- tail ---
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);
    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (len & 15) {
        case 15: k2 ^= (uint64_t)(tail[14]) << 48;
        case 14: k2 ^= (uint64_t)(tail[13]) << 40;
        case 13: k2 ^= (uint64_t)(tail[12]) << 32;
        case 12: k2 ^= (uint64_t)(tail[11]) << 24;
        case 11: k2 ^= (uint64_t)(tail[10]) << 16;
        case 10: k2 ^= (uint64_t)(tail[9]) << 8;
        case  9: k2 ^= (uint64_t)(tail[8]) << 0;
                 k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;

        case  8: k1 ^= (uint64_t)(tail[7]) << 56;
        case  7: k1 ^= (uint64_t)(tail[6]) << 48;
        case  6: k1 ^= (uint64_t)(tail[5]) << 40;
        case  5: k1 ^= (uint64_t)(tail[4]) << 32;
        case  4: k1 ^= (uint64_t)(tail[3]) << 24;
        case  3: k1 ^= (uint64_t)(tail[2]) << 16;
        case  2: k1 ^= (uint64_t)(tail[1]) << 8;
        case  1: k1 ^= (uint64_t)(tail[0]) << 0;
                 k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
    };

    // --- finalization ---
    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    return {h1, h2};
}

Hash128 MurmurHash3::HashString(const std::string& str, uint32_t seed) {
    return Hash(str.data(), (int)str.length(), seed);
}

Hash128 MurmurHash3::HashFile(const std::string& path, uint32_t seed) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {0, 0};

    // 针对大文件，由于 mmh3 的 x64_128 是基于块的，
    // 这里采用简单方案：对于小文件直接读入内存，对于大文件后续可以扩展流式 API。
    // 暂时采用读取全量文件以简化实现（如果文件超过数百 MB 会慢，但比 md5 快得多）。
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return Hash(buffer.data(), (int)buffer.size(), seed);
}

std::string Hash128::ToString() const {
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << low 
       << std::hex << std::setw(16) << std::setfill('0') << high;
    return ss.str();
}

Hash128 Hash128::FromString(const std::string& str) {
    if (str.length() != 32) return {0, 0};
    Hash128 h;
    std::string lowPart  = str.substr(0, 16);
    std::string highPart = str.substr(16, 16);
    h.low  = std::stoull(lowPart, nullptr, 16);
    h.high = std::stoull(highPart, nullptr, 16);
    return h;
}

} // namespace Prisma::Math
