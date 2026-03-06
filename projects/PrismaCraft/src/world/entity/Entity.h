#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include "../../Core/BlockPos.h"
#include <string>
#include <memory>

namespace PrismaCraft {

class Level;
class EntityType;

/**
 * 3D vector for positions and velocities
 * Corresponds to: net.minecraft.world.phys.Vec3
 */
class PRISMACRAFT_API Vec3 {
public:
    double x;
    double y;
    double z;

    constexpr Vec3() : x(0), y(0), z(0) {}
    constexpr Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    // Vector operations
    Vec3 add(double dx, double dy, double dz) const {
        return Vec3(x + dx, y + dy, z + dz);
    }

    Vec3 add(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 subtract(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 scale(double scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }

    double dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vec3 cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    double length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    double lengthSquared() const {
        return x * x + y * y + z * z;
    }

    Vec3 normalize() const {
        double len = length();
        if (len > 0) {
            return scale(1.0 / len);
        }
        return Vec3();
    }

    double distanceTo(const Vec3& other) const {
        return subtract(other).length();
    }

    std::string toString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }
};

/**
 * Axis-aligned bounding box
 * Corresponds to: net.minecraft.world.phys.AABB
 */
class PRISMACRAFT_API AABB {
public:
    double minX;
    double minY;
    double minZ;
    double maxX;
    double maxY;
    double maxZ;

    AABB(double x1, double y1, double z1, double x2, double y2, double z2)
        : minX(x1), minY(y1), minZ(z1), maxX(x2), maxY(y2), maxZ(z2) {}

    AABB(const Vec3& min, const Vec3& max)
        : minX(min.x), minY(min.y), minZ(min.z),
          maxX(max.x), maxY(max.y), maxZ(max.z) {}

    // Properties
    double getXSize() const { return maxX - minX; }
    double getYSize() const { return maxY - minY; }
    double getZSize() const { return maxZ - minZ; }

    Vec3 getCenter() const {
        return Vec3(
            (minX + maxX) * 0.5,
            (minY + maxY) * 0.5,
            (minZ + maxZ) * 0.5
        );
    }

    // Collision tests
    bool contains(const Vec3& point) const {
        return point.x >= minX && point.x <= maxX &&
               point.y >= minY && point.y <= maxY &&
               point.z >= minZ && point.z <= maxZ;
    }

    bool intersects(const AABB& other) const {
        return minX < other.maxX && maxX > other.minX &&
               minY < other.maxY && maxY > other.minY &&
               minZ < other.maxZ && maxZ > other.minZ;
    }

    // Expansion and contraction
    AABB inflate(double dx, double dy, double dz) const {
        return AABB(
            minX - dx, minY - dy, minZ - dz,
            maxX + dx, maxY + dy, maxZ + dz
        );
    }

    AABB shrink(double dx, double dy, double dz) const {
        return inflate(-dx, -dy, -dz);
    }

    AABB move(double dx, double dy, double dz) const {
        return AABB(
            minX + dx, minY + dy, minZ + dz,
            maxX + dx, maxY + dy, maxZ + dz
        );
    }

    // Min/max operations
    AABB minmax(const Vec3& min, const Vec3& max) const {
        return AABB(
            std::min(minX, min.x), std::min(minY, min.y), std::min(minZ, min.z),
            std::max(maxX, max.x), std::max(maxY, max.y), std::max(maxZ, max.z)
        );
    }

    std::string toString() const {
        return "AABB[" + std::to_string(minX) + ", " + std::to_string(minY) + ", " +
               std::to_string(minZ) + " -> " + std::to_string(maxX) + ", " +
               std::to_string(maxY) + ", " + std::to_string(maxZ) + "]";
    }
};

/**
 * Entity base class
 * Corresponds to: net.minecraft.world.entity.Entity
 */
class PRISMACRAFT_API Entity {
public:
    virtual ~Entity() = default;

    // Entity identification
    int getId() const { return id; }
    const std::string& getUUID() const { return uuid; }
    const std::string& getName() const { return name; }

    // Type
    virtual const EntityType& getType() const = 0;

    // Position
    const Vec3& getPosition() const { return pos; }
    void setPosition(const Vec3& pos) { this->pos = pos; }
    void setPosition(double x, double y, double z) {
        pos = Vec3(x, y, z);
    }

    BlockPos getBlockPos() const {
        return BlockPos(
            static_cast<int>(std::floor(pos.x)),
            static_cast<int>(std::floor(pos.y)),
            static_cast<int>(std::floor(pos.z))
        );
    }

    // Rotation
    float getYRot() const { return yRot; }
    float getXRot() const { return xRot; }
    void setYRot(float rot) { yRot = rot; }
    void setXRot(float rot) { xRot = rot; }

    // Velocity
    const Vec3& getDeltaMovement() const { return delta; }
    void setDeltaMovement(const Vec3& delta) { this->delta = delta; }
    void setDeltaMovement(double dx, double dy, double dz) {
        delta = Vec3(dx, dy, dz);
    }

    // Bounding box
    const AABB& getBoundingBox() const { return boundingBox; }
    void setBoundingBox(const AABB& box) { boundingBox = box; }

    // Dimensions (width, height)
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    void setDimensions(float w, float h) { width = w; height = h; }

    // Level
    Level* getLevel() const { return level; }
    void setLevel(Level* level) { this->level = level; }

    // Entity flags
    bool isOnGround() const { return onGround; }
    void setOnGround(bool value) { onGround = value; }

    bool isRemoved() const { return removed; }
    void remove() { removed = true; }

    // Tick
    virtual void tick();
    virtual void baseTick();

    // Update methods
    virtual void move(double dx, double dy, double dz);
    virtual void teleport(double x, double y, double z);

protected:
    Entity(EntityType* type, Level* level);

    int id;
    std::string uuid;
    std::string name;

    Vec3 pos;
    Vec3 delta;
    float yRot = 0.0f;
    float xRot = 0.0f;

    AABB boundingBox;
    float width;
    float height;

    Level* level;
    bool onGround = false;
    bool removed = false;

    static int nextId();
};

/**
 * Entity type registry
 * Corresponds to: net.minecraft.world.entity.EntityType
 */
class PRISMACRAFT_API EntityType {
public:
    static constexpr int PLAYER = 1;
    static constexpr int ZOMBIE = 2;
    static constexpr int SKELETON = 3;
    static constexpr int CREEPER = 4;
    static constexpr int SPIDER = 5;
    static constexpr int ENDERMAN = 6;
    static constexpr int PIG = 7;
    static constexpr int COW = 8;
    static constexpr int CHICKEN = 9;
    static constexpr int SHEEP = 10;
    static constexpr int VILLAGER = 11;
    static constexpr int ITEM = 12;
    static constexpr int EXPERIENCE_ORB = 13;
    static constexpr int PAINTING = 14;
    static constexpr int FALLING_BLOCK = 15;
    static constexpr int ARROW = 16;
    static constexpr int SNOWBALL = 17;
    static constexpr int FIREBALL = 18;
    static constexpr int THROWN_EGG = 19;
    static constexpr int TNT = 20;

    EntityType(int id, const std::string& name)
        : id(id), name(name) {}

    int getId() const { return id; }
    const std::string& getName() const { return name; }

    // Create entity instance
    virtual Entity* create(Level* level) const = 0;

    // Entity properties
    virtual float getWidth() const { return 0.6f; }
    virtual float getHeight() const { return 1.8f; }

protected:
    int id;
    std::string name;
};

// Forward declaration for BlockEntity
class BlockEntity;

} // namespace PrismaCraft
