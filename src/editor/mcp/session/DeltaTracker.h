#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <xxhash.h>

namespace Prisma {
namespace MCP {

using Hash64 = uint64_t;

inline Hash64 ComputeHash(std::string_view data) {
    return XXH3_64bits(data.data(), data.size());
}

struct DeltaResult {
    bool hasChanges = false;
    glz::json_t delta;
    glz::json_t snapshot;
    float changeRatio = 0.0f;
    Hash64 newHash = 0;
};

class DeltaTracker {
public:
    void UpdateEntityHash(uint32_t entityId, std::string_view serialized);
    void UpdateSceneHash(const std::string& entitiesJson);
    void UpdateAssetHash(const std::string& assetsJson);
    void RemoveEntityHash(uint32_t entityId);

    Hash64 GetEntityHash(uint32_t entityId) const;
    Hash64 GetSceneHash() const { return m_SceneHash; }
    Hash64 GetAssetHash() const { return m_AssetHash; }
    Hash64 GetRootHash() const;
    std::string GetRootHashString() const;

    DeltaResult ComputeDelta(Hash64 knownRootHash, const std::string& toolName,
                             const glz::json_t& args) const;

    static constexpr float kDeltaThreshold = 0.3f;

private:
    std::unordered_map<uint32_t, Hash64> m_EntityHashes;
    std::unordered_map<uint32_t, Hash64> m_PreviousEntityHashes;

    Hash64 m_SceneHash = 0;
    Hash64 m_AssetHash = 0;

    mutable bool m_RootHashDirty = true;
    mutable Hash64 m_CachedRootHash = 0;

    void recomputeRootHash() const;
};

} // namespace MCP
} // namespace Prisma
