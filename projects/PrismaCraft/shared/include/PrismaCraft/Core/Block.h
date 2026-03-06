#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include <string>
#include <memory>

namespace PrismaCraft {

class BlockState;

/**
 * Base block class
 * Corresponds to: net.minecraft.world.level.block.Block
 */
class PRISMACRAFT_API Block {
public:
    virtual ~Block() = default;

    // Block properties
    virtual const std::string& getName() const = 0;
    virtual int getId() const = 0;

    // State handling
    virtual const BlockState& defaultBlockState() const = 0;

    // Block behavior
    virtual float getDestroySpeed(const BlockState& state) const { return 1.0f; }
    virtual float getExplosionResistance() const { return 0.0f; }

    // Light
    virtual int getLightEmission(const BlockState& state) const { return 0; }
    virtual int getLightBlock(const BlockState& state) const { return 0; }

    // Collision
    virtual bool isSolid(const BlockState& state) const { return true; }
    virtual bool isAir(const BlockState& state) const { return false; }

protected:
    Block() = default;
};

} // namespace PrismaCraft
