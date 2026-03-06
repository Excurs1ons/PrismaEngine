#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include "BlockState.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include <functional>

namespace PrismaCraft {

// Forward declarations
class Block;
class ItemType;
class EntityType;

/**
 * Resource location (namespaced identifier)
 * Corresponds to: net.minecraft.resources.ResourceLocation
 */
class PRISMACRAFT_API ResourceLocation {
public:
    ResourceLocation() = default;

    ResourceLocation(const std::string& namespace_, const std::string& path)
        : namespace_(namespace_), path(path) {
        if (namespace_.empty() || !isValidNamespace(namespace_)) {
            throw std::invalid_argument("Invalid namespace: " + namespace_);
        }
        if (path.empty() || !isValidPath(path)) {
            throw std::invalid_argument("Invalid path: " + path);
        }
    }

    explicit ResourceLocation(const std::string& combined) {
        size_t colon = combined.find(':');
        if (colon == std::string::npos) {
            namespace_ = "minecraft";
            path = combined;
        } else {
            namespace_ = combined.substr(0, colon);
            path = combined.substr(colon + 1);
        }
        if (!isValidNamespace(namespace_) || !isValidPath(path)) {
            throw std::invalid_argument("Invalid resource location: " + combined);
        }
    }

    const std::string& getNamespace() const { return namespace_; }
    const std::string& getPath() const { return path; }

    std::string toString() const {
        return namespace_ + ":" + path;
    }

    bool operator==(const ResourceLocation& other) const {
        return namespace_ == other.namespace_ && path == other.path;
    }

    bool operator!=(const ResourceLocation& other) const {
        return !(*this == other);
    }

    size_t hashCode() const {
        return std::hash<std::string>{}(toString());
    }

    struct Hash {
        size_t operator()(const ResourceLocation& loc) const {
            return loc.hashCode();
        }
    };

    static const std::string DEFAULT_NAMESPACE;

private:
    std::string namespace_;
    std::string path;

    static bool isValidNamespace(const std::string& ns) {
        if (ns.empty()) return false;
        for (char c : ns) {
            if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') {
                return false;
            }
        }
        return true;
    }

    static bool isValidPath(const std::string& p) {
        if (p.empty()) return false;
        for (char c : p) {
            if (!std::isalnum(c) && c != '_' && c != '-' && c != '.' && c != '/') {
                return false;
            }
        }
        return true;
    }
};

/**
 * Generic registry for game objects
 * Corresponds to: net.minecraft.core.Registry
 */
template<typename T>
class PRISMACRAFT_API Registry {
public:
    using IdType = int;

    Registry(const std::string& name) : name_(name) {}

    // Register an object and return its ID
    IdType registerObject(const ResourceLocation& key, std::unique_ptr<T> object) {
        IdType id = static_cast<IdType>(objects_.size());
        objects_.push_back(object.get());
        byId_[id] = object.get();
        byKey_[key] = object.get();
        keyToId_[key] = id;
        object.release(); // Registry owns the object
        return id;
    }

    // Get object by ID
    T* getById(IdType id) const {
        auto it = byId_.find(id);
        return it != byId_.end() ? it->second : nullptr;
    }

    // Get object by resource location
    T* get(const ResourceLocation& key) const {
        auto it = byKey_.find(key);
        return it != byKey_.end() ? it->second : nullptr;
    }

    // Get ID by resource location
    IdType getId(const ResourceLocation& key) const {
        auto it = keyToId_.find(key);
        return it != keyToId_.end() ? it->second : -1;
    }

    // Get resource location by ID
    ResourceLocation getKey(IdType id) const {
        for (const auto& entry : keyToId_) {
            if (entry.second == id) {
                return entry.first;
            }
        }
        return ResourceLocation();
    }

    // Iterate over all objects
    const std::vector<T*>& getObjects() const {
        return objects_;
    }

    size_t size() const { return objects_.size(); }
    const std::string& getName() const { return name_; }

private:
    std::string name_;
    std::vector<T*> objects_;
    std::unordered_map<IdType, T*> byId_;
    std::unordered_map<ResourceLocation, T*, ResourceLocation::Hash> byKey_;
    std::unordered_map<ResourceLocation, IdType, ResourceLocation::Hash> keyToId_;
};

/**
 * Built-in registry access
 * Corresponds to: net.minecraft.core.Registries
 */
class PRISMACRAFT_API Registries {
public:
    static Registry<Block>& BLOCK();
    static Registry<ItemType>& ITEM();
    static Registry<EntityType>& ENTITY_TYPE();

    // Initialize all registries
    static void init();

private:
    static bool initialized_;
};

/**
 * Vanilla blocks registry
 * Corresponds to: net.minecraft.core.Blocks
 */
class PRISMACRAFT_API Blocks {
public:
    static Block* AIR;
    static Block* STONE;
    static Block* GRASS_BLOCK;
    static Block* DIRT;
    static Block* COBBLESTONE;
    static Block* PLANKS_OAK;
    static Block* PLANKS_SPRUCE;
    static Block* PLANKS_BIRCH;
    static Block* PLANKS_JUNGLE;
    static Block* PLANKS_ACACIA;
    static Block* PLANKS_DARK_OAK;
    static Block* BEDROCK;
    static Block* WATER;
    static Block* LAVA;
    static Block* SAND;
    static Block* GRAVEL;
    static Block* GOLD_ORE;
    static Block* IRON_ORE;
    static Block* COAL_ORE;
    static Block* LOG_OAK;
    static Block* LOG_SPRUCE;
    static Block* LOG_BIRCH;
    static Block* LOG_JUNGLE;
    static Block* LOG_ACACIA;
    static Block* LOG_DARK_OAK;
    static Block* LEAVES_OAK;
    static Block* LEAVES_SPRUCE;
    static Block* LEAVES_BIRCH;
    static Block* LEAVES_JUNGLE;
    static Block* LEAVES_ACACIA;
    static Block* LEAVES_DARK_OAK;
    static Block* SPONGE;
    static Block* GLASS;
    static Block* LAPIS_ORE;
    static Block* LAPIS_BLOCK;
    static Block* DISPENSER;
    static Block* SANDSTONE;
    static Block* NOTE_BLOCK;
    static Block* BED_RED;
    static Block* POWERED_RAIL;
    static Block* DETECTOR_RAIL;
    static Block* STICKY_PISTON;
    static Block* COBWEB;
    static Block* GRASS;
    static Block* FERN;
    static Block* DEAD_BUSH;
    static Block* SEAGRASS;
    static Block* SEA_PICKLE;

    // Initialize all vanilla blocks
    static void init();

private:
    Blocks() = default;
};

} // namespace PrismaCraft
