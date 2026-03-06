#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include "Block.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <stdexcept>

namespace PrismaCraft {

class BlockProperty;

/**
 * Block property base class
 * Corresponds to: net.minecraft.world.level.block.state.properties.Property
 */
class PRISMACRAFT_API BlockProperty {
public:
    virtual ~BlockProperty() = default;

    virtual std::string getName() const = 0;
    virtual std::vector<std::string> getPossibleValues() const = 0;
    virtual size_t getValueIndex(const std::string& value) const = 0;
    virtual std::string getValueName(size_t index) const = 0;

protected:
    BlockProperty() = default;
};

/**
 * Boolean property (true/false)
 * Corresponds to: net.minecraft.world.level.block.state.properties.BooleanProperty
 */
class PRISMACRAFT_API BooleanProperty : public BlockProperty {
public:
    static BooleanProperty* create(const std::string& name);

    std::string getName() const override { return name; }
    std::vector<std::string> getPossibleValues() const override {
        return {"true", "false"};
    }

    size_t getValueIndex(const std::string& value) const override {
        return (value == "true") ? 0 : 1;
    }

    std::string getValueName(size_t index) const override {
        return (index == 0) ? "true" : "false";
    }

private:
    std::string name;
    explicit BooleanProperty(const std::string& n) : name(n) {}
};

/**
 * Integer property (min to max)
 * Corresponds to: net.minecraft.world.level.block.state.properties.IntegerProperty
 */
class PRISMACRAFT_API IntegerProperty : public BlockProperty {
public:
    static IntegerProperty* create(const std::string& name, int min, int max);

    std::string getName() const override { return name; }
    std::vector<std::string> getPossibleValues() const override;

    size_t getValueIndex(const std::string& value) const override;
    std::string getValueName(size_t index) const override {
        return std::to_string(minValue + (int)index);
    }

    int getMin() const { return minValue; }
    int getMax() const { return maxValue; }

private:
    std::string name;
    int minValue;
    int maxValue;

    IntegerProperty(const std::string& n, int min, int max)
        : name(n), minValue(min), maxValue(max) {}
};

/**
 * Enum property (string values)
 * Corresponds to: net.minecraft.world.level.block.state.properties.EnumProperty
 */
class PRISMACRAFT_API EnumProperty : public BlockProperty {
public:
    static EnumProperty* create(const std::string& name, const std::vector<std::string>& values);

    std::string getName() const override { return name; }
    std::vector<std::string> getPossibleValues() const override { return values; }

    size_t getValueIndex(const std::string& value) const override;
    std::string getValueName(size_t index) const override {
        return values.at(index);
    }

private:
    std::string name;
    std::vector<std::string> values;

    EnumProperty(const std::string& n, const std::vector<std::string>& v)
        : name(n), values(v) {}
};

/**
 * Block state identifier
 * Corresponds to: net.minecraft.world.level.block.state.BlockStateBase
 */
class PRISMACRAFT_API BlockStateBase {
public:
    virtual ~BlockStateBase() = default;

    virtual const Block& getBlock() const = 0;
    virtual size_t getStateId() const = 0;
    virtual bool hasProperty(const BlockProperty& property) const = 0;
    virtual std::string getValue(const BlockProperty& property) const = 0;

protected:
    BlockStateBase() = default;
};

/**
 * Block state with property values
 * Corresponds to: net.minecraft.world.level.block.state.BlockState
 */
class PRISMACRAFT_API BlockState : public BlockStateBase {
public:
    BlockState(const Block& block, size_t stateId)
        : block(block), stateId(stateId) {}

    const Block& getBlock() const override { return block; }
    size_t getStateId() const override { return stateId; }

    bool hasProperty(const BlockProperty& property) const override;
    std::string getValue(const BlockProperty& property) const override;

    // Convenience methods
    bool is(const Block& otherBlock) const { return &block == &otherBlock; }
    bool isAir() const;

    // Property accessors
    bool getBoolean(const BooleanProperty& property) const;
    int getInteger(const IntegerProperty& property) const;
    std::string getEnum(const EnumProperty& property) const;

    // Generate state map key for serialization
    std::map<std::string, std::string> getValues() const;

    // Hash for container storage
    struct Hash {
        size_t operator()(const BlockState& state) const {
            return std::hash<size_t>{}(state.stateId);
        }
    };

    bool operator==(const BlockState& other) const {
        return stateId == other.stateId;
    }

private:
    const Block& block;
    size_t stateId;
};

} // namespace PrismaCraft
