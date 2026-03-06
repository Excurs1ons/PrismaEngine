#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include "../../Core/BlockPos.h"
#include "../../Core/BlockState.h"
#include "../../Core/ChunkPos.h"
#include <memory>
#include <vector>

namespace PrismaCraft {

class BlockGetter;
class LevelWriter;
class LevelChunk;
class Entity;

/**
 * Level interface - world level access
 * Corresponds to: net.minecraft.world.level.Level
 */
class PRISMACRAFT_API Level {
public:
    virtual ~Level() = default;

    // Chunk access
    virtual LevelChunk* getChunk(int x, int z) const = 0;
    virtual bool isLoaded(int x, int z) const = 0;

    // Block access
    virtual const class BlockState& getBlockState(const BlockPos& pos) const = 0;
    virtual void setBlock(const BlockPos& pos, const class BlockState& state) = 0;

    // Light
    virtual int getBrightness(const BlockPos& pos) const = 0;
    virtual int getRawBrightness(const BlockPos& pos, int amount) const = 0;

    // Time
    virtual int64_t getDayTime() const = 0;
    virtual void setDayTime(int64_t time) = 0;

    // Dimension
    virtual const std::string& getDimension() const = 0;

    // Entity management
    virtual void addEntity(std::unique_ptr<Entity> entity) = 0;
    virtual void removeEntity(Entity* entity) = 0;
    virtual Entity* getEntity(int id) const = 0;

    // Height map
    virtual int getHeight(int x, int z) const = 0;
    virtual void setHeight(int x, int z, int height) = 0;

    // Game rules
    virtual bool getGameRuleBoolean(const std::string& rule) const = 0;
    virtual int getGameRuleInt(const std::string& rule) const = 0;

    // Block update
    virtual void blockChanged(const BlockPos& pos) = 0;

protected:
    Level() = default;
};

/**
 * Block getter interface
 * Corresponds to: net.minecraft.world.level.BlockGetter
 */
class PRISMACRAFT_API BlockGetter {
public:
    virtual ~BlockGetter() = default;

    virtual const class BlockState& getBlockState(const BlockPos& pos) const = 0;

    // Convenience methods
    virtual float getBlockBrightness(const BlockPos& pos) const = 0;
    virtual bool isBlockInLine(class AABB& box, const BlockPos& from, const BlockPos& to) const = 0;

protected:
    BlockGetter() = default;
};

/**
 * Level writer interface
 * Corresponds to: net.minecraft.world.level.LevelWriter
 */
class PRISMACRAFT_API LevelWriter {
public:
    virtual ~LevelWriter() = default;

    virtual void setBlock(const BlockPos& pos, const class BlockState& state) = 0;
    virtual void removeBlock(const BlockPos& pos) = 0;

protected:
    LevelWriter() = default;
};

/**
 * Level entity access
 * Corresponds to: net.minecraft.world.level.EntityGetter
 */
class PRISMACRAFT_API EntityGetter {
public:
    virtual ~EntityGetter() = default;

    virtual Entity* getEntity(int id) const = 0;
    virtual std::vector<Entity*> getEntities(const class AABB& box) const = 0;
    virtual std::vector<Entity*> getEntitiesOfClass(const class AABB& box, const std::type_info& type) const = 0;

protected:
    EntityGetter() = default;
};

} // namespace PrismaCraft
