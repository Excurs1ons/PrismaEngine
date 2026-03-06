#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include "BlockPos.h"
#include <functional>
#include <string>

namespace PrismaCraft {

/**
 * Chunk coordinate (x, z)
 * Corresponds to: net.minecraft.core.ChunkPos
 */
class PRISMACRAFT_API ChunkPos {
public:
    int x;
    int z;

    constexpr ChunkPos() : x(0), z(0) {}
    constexpr ChunkPos(int x, int z) : x(x), z(z) {}
    constexpr ChunkPos(const BlockPos& pos) : x(pos.x >> 4), z(pos.z >> 4) {}

    constexpr bool operator==(const ChunkPos& other) const {
        return x == other.x && z == other.z;
    }

    constexpr bool operator!=(const ChunkPos& other) const {
        return !(*this == other);
    }

    constexpr int hashCode() const {
        return (x & 0xFFFF) | ((z & 0xFFFF) << 16);
    }

    static constexpr long long asLong(int x, int z) {
        return (long long)x & 0xFFFFFFFFLL | ((long long)z & 0xFFFFFFFFLL) << 32;
    }

    constexpr long long asLong() const {
        return asLong(x, z);
    }

    static constexpr ChunkPos fromLong(long long packed) {
        return ChunkPos((int)packed, (int)(packed >> 32));
    }

    constexpr BlockPos getBlockAt(int x, int y, int z) const {
        return BlockPos((this->x << 4) + x, y, (this->z << 4) + z);
    }

    std::string toString() const {
        return "[" + std::to_string(x) + ", " + std::to_string(z) + "]";
    }

    struct Hash {
        size_t operator()(const ChunkPos& pos) const {
            return std::hash<long long>{}(pos.asLong());
        }
    };
};

/**
 * Section coordinate (for 16x16x16 chunk sections)
 * Corresponds to: net.minecraft.core.SectionPos
 */
class PRISMACRAFT_API SectionPos {
public:
    int x;
    int y;
    int z;

    constexpr SectionPos() : x(0), y(0), z(0) {}
    constexpr SectionPos(int x, int y, int z) : x(x), y(y), z(z) {}
    constexpr SectionPos(const ChunkPos& chunkPos, int y)
        : x(chunkPos.x), y(y), z(chunkPos.z) {}
    constexpr SectionPos(const BlockPos& blockPos)
        : x(blockPos.x >> 4), y(blockPos.y >> 4), z(blockPos.z >> 4) {}

    constexpr bool operator==(const SectionPos& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    static constexpr long long asLong(int x, int y, int z) {
        long long l = 0LL;
        l |= ((long long)x & 0xFFFFFLL) << 42;
        l |= ((long long)y & 0xFFFFFLL) << 20;
        l |= ((long long)z & 0xFFFFFLL);
        return l;
    }

    constexpr long long asLong() const {
        return asLong(x, y, z);
    }

    static constexpr SectionPos fromLong(long long packed) {
        return SectionPos(
            (int)(packed << 44 >> 44),
            (int)(packed << 24 >> 44),
            (int)(packed << 4 >> 44)
        );
    }

    static constexpr long long offset(long long packed, int x, int y, int z) {
        return asLong(
            (int)(packed << 44 >> 44) + x,
            (int)(packed << 24 >> 44) + y,
            (int)(packed << 4 >> 44) + z
        );
    }

    constexpr BlockPos origin() const {
        return BlockPos(x << 4, y << 4, z << 4);
    }

    constexpr BlockPos getBlockAt(int x, int y, int z) const {
        return BlockPos((this->x << 4) + x, (this->y << 4) + y, (this->z << 4) + z);
    }

    constexpr ChunkPos getChunkPos() const {
        return ChunkPos(x, z);
    }

    static SectionPos of(const BlockPos& pos) {
        return SectionPos(pos);
    }

    struct Hash {
        size_t operator()(const SectionPos& pos) const {
            return std::hash<long long>{}(pos.asLong());
        }
    };
};

} // namespace PrismaCraft
