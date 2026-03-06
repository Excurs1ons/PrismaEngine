#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include "../../../Core/BlockPos.h"
#include "../../../Core/BlockState.h"
#include "../../../Core/ChunkPos.h"
#include <memory>
#include <array>
#include <vector>

namespace PrismaCraft {

class Level;
class BlockState;

/**
 * Palette container for block storage
 * Corresponds to: net.minecraft.world.level.chunk.PalettedContainer
 */
class PRISMACRAFT_API PalettedContainer {
public:
    static constexpr int SECTION_SIZE = 16 * 16 * 16; // 4096 blocks
    static constexpr int BITS_PER_BLOCK = 4; // Up to 16 block states

    PalettedContainer();
    explicit PalettedContainer(const class BlockState& defaultState);

    // Block access
    const class BlockState& get(int x, int y, int z) const;
    void set(int x, int y, int z, const class BlockState& state);

    // Bulk operations
    void fill(const class BlockState& state);
    void read(class NBT::CompoundTag* tag);
    std::unique_ptr<class NBT::CompoundTag> write() const;

    // Palette info
    size_t getPaletteSize() const { return palette.size(); }
    bool hasSinglePalette() const { return palette.size() == 1; }

private:
    // Index calculation
    static int getIndex(int x, int y, int z) {
        return y << 8 | z << 4 | x; // y * 256 + z * 16 + x
    }

    // Storage
    std::vector<uint8_t> data; // Packed indices
    std::vector<const class BlockState*> palette; // Palette of states
    const class BlockState* defaultState;
};

/**
 * Chunk section (16x16x16 blocks)
 * Corresponds to: net.minecraft.world.level.chunk.ChunkSection
 */
class PRISMACRAFT_API ChunkSection {
public:
    static constexpr int SECTION_HEIGHT = 16;
    static constexpr int SECTION_WIDTH = 16;
    static constexpr int SECTION_DEPTH = 16;

    ChunkSection(int yIndex);
    ~ChunkSection();

    // Block access
    const class BlockState& getBlockState(int x, int y, int z) const;
    void setBlockState(int x, int y, int z, const class BlockState& state);

    // Section properties
    int getYIndex() const { return yIndex; }
    bool isEmpty() const { return empty; }
    bool is possiblyFullyAir() const { return empty; }

    // Recalculate empty status
    void recalculate();

    // Serialization
    void read(class NBT::CompoundTag* tag);
    std::unique_ptr<class NBT::CompoundTag> write() const;

    // Non-copyable
    ChunkSection(const ChunkSection&) = delete;
    ChunkSection& operator=(const ChunkSection&) = delete;

private:
    int yIndex;
    PalettedContainer blocks;
    bool empty;

    // Non-owning reference to level (for block state lookup)
    Level* level;
};

/**
 * Level chunk (16x256x16 blocks, multiple sections)
 * Corresponds to: net.minecraft.world.level.chunk.LevelChunk
 */
class PRISMACRAFT_API LevelChunk {
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 256; // Can be adjusted for different world heights
    static constexpr int DEPTH = 16;
    static constexpr int SECTION_COUNT = HEIGHT / SECTION_HEIGHT; // 16 sections

    LevelChunk(Level* level, const ChunkPos& pos);
    ~LevelChunk();

    // Chunk position
    const ChunkPos& getPos() const { return pos; }
    int getX() const { return pos.x; }
    int getZ() const { return pos.z; }

    // Block access
    const class BlockState& getBlockState(const BlockPos& pos) const;
    const class BlockState& getBlockState(int x, int y, int z) const;
    void setBlockState(const BlockPos& pos, const class BlockState& state);

    // Section access
    ChunkSection* getSection(int yIndex);
    const ChunkSection* getSection(int yIndex) const;
    int getSectionIndex(int y) const { return y >> 4; }

    // Height map
    int getHeight(int x, int z) const;
    void setHeight(int x, int z, int height);

    // Chunk status
    bool isLoaded() const { return loaded; }
    void setLoaded(bool value) { loaded = value; }

    // Tick scheduling
    void markUnsaved();
    void markSaved();

    // Serialization
    void read(class NBT::CompoundTag* tag);
    std::unique_ptr<class NBT::CompoundTag> write() const;

    // Entity access
    std::vector<class Entity*>& getEntities() { return entities; }
    const std::vector<class Entity*>& getEntities() const { return entities; }

    // Block entity access
    std::vector<class BlockEntity*>& getBlockEntities() { return blockEntities; }
    const std::vector<class BlockEntity*>& getBlockEntities() const { return blockEntities; }

    // Non-copyable
    LevelChunk(const LevelChunk&) = delete;
    LevelChunk& operator=(const LevelChunk&) = delete;

private:
    Level* level;
    ChunkPos pos;
    std::array<std::unique_ptr<ChunkSection>, SECTION_COUNT> sections;
    std::array<uint16_t, WIDTH * DEPTH> heightMap; // Height map for each column
    bool loaded;

    std::vector<class Entity*> entities;
    std::vector<class BlockEntity*> blockEntities;

    void init();
};

} // namespace PrismaCraft
