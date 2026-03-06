#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include <functional>
#include <cmath>

namespace PrismaCraft {

/**
 * immutable integer triple coordinates
 * Corresponds to: net.minecraft.core.Vec3i
 */
struct PRISMACRAFT_API Vec3i {
    int x;
    int y;
    int z;

    constexpr Vec3i(int x, int y, int z) : x(x), y(y), z(z) {}

    constexpr bool operator==(const Vec3i& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    constexpr bool operator!=(const Vec3i& other) const {
        return !(*this == other);
    }

    constexpr Vec3i offset(int dx, int dy, int dz) const {
        return Vec3i(x + dx, y + dy, z + dz);
    }

    // Helper methods
    constexpr Vec3i above() const { return Vec3i(x, y + 1, z); }
    constexpr Vec3i below() const { return Vec3i(x, y - 1, z); }
    constexpr Vec3i north() const { return Vec3i(x, y, z - 1); }
    constexpr Vec3i south() const { return Vec3i(x, y, z + 1); }
    constexpr Vec3i west() const { return Vec3i(x - 1, y, z); }
    constexpr Vec3i east() const { return Vec3i(x + 1, y, z); }
};

/**
 * Immutable block position
 * Corresponds to: net.minecraft.core.BlockPos
 */
class PRISMACRAFT_API BlockPos : public Vec3i {
public:
    static const BlockPos ZERO;
    static const int PACKED_X_LENGTH = 1 + (int)std::log2(30000000);
    static const int PACKED_Z_LENGTH = PACKED_X_LENGTH;
    static const int PACKED_Y_LENGTH = 64 - PACKED_X_LENGTH - PACKED_Z_LENGTH;
    static const long long PACKED_X_MASK = (1LL << PACKED_X_LENGTH) - 1LL;
    static const long long PACKED_Y_MASK = (1LL << PACKED_Y_LENGTH) - 1LL;
    static const long long PACKED_Z_MASK = (1LL << PACKED_Z_LENGTH) - 1LL;
    static const int MAX_Y = 1 << (PACKED_Y_LENGTH - 1);

    constexpr BlockPos() : Vec3i(0, 0, 0) {}
    constexpr BlockPos(int x, int y, int z) : Vec3i(x, y, z) {}
    constexpr BlockPos(const Vec3i& vec) : Vec3i(vec) {}

    // Pack/unpack coordinates into a single long
    static constexpr long long asLong(int x, int y, int z) {
        long long l = 0LL;
        l |= ((long long)x & PACKED_X_MASK) << PACKED_Z_LENGTH;
        l |= ((long long)y & PACKED_Y_MASK) << (PACKED_X_LENGTH + PACKED_Z_LENGTH);
        l |= ((long long)z & PACKED_Z_MASK);
        return l;
    }

    constexpr long long asLong() const {
        return asLong(x, y, z);
    }

    static constexpr BlockPos fromLong(long long packed) {
        int i = (int)(packed << (64 - PACKED_X_LENGTH - PACKED_Z_LENGTH) >> (64 - PACKED_X_LENGTH));
        int j = (int)(packed << (64 - PACKED_Y_LENGTH - PACKED_X_LENGTH - PACKED_Z_LENGTH) >> (64 - PACKED_Y_LENGTH));
        int k = (int)(packed << (64 - PACKED_Z_LENGTH) >> (64 - PACKED_Z_LENGTH));
        return BlockPos(i, j, k);
    }

    // Hash for use in containers
    struct Hash {
        size_t operator()(const BlockPos& pos) const {
            return std::hash<long long>{}(pos.asLong());
        }
    };
};

/**
 * Mutable block position for temporary use
 * Corresponds to: net.minecraft.core.BlockPos.MutableBlockPos
 */
class PRISMACRAFT_API MutableBlockPos : public Vec3i {
public:
    MutableBlockPos() : Vec3i(0, 0, 0) {}
    MutableBlockPos(int x, int y, int z) : Vec3i(x, y, z) {}
    explicit MutableBlockPos(const Vec3i& vec) : Vec3i(vec) {}

    MutableBlockPos& set(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
        return *this;
    }

    MutableBlockPos& set(int axis, int value) {
        if (axis == 0) x = value;
        else if (axis == 1) y = value;
        else if (axis == 2) z = value;
        return *this;
    }

    MutableBlockPos& offset(int dx, int dy, int dz) {
        x += dx;
        y += dy;
        z += dz;
        return *this;
    }

    MutableBlockPos& move(int dx, int dy, int dz) {
        return offset(dx, dy, dz);
    }
};

} // namespace PrismaCraft
