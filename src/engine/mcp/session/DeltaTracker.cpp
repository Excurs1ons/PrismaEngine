#include "DeltaTracker.h"
#include <algorithm>
#include <sstream>

namespace Prisma {
namespace MCP {

void DeltaTracker::UpdateEntityHash(uint32_t entityId, std::string_view serialized) {
    m_PreviousEntityHashes[entityId] = m_EntityHashes[entityId];
    m_EntityHashes[entityId] = ComputeHash(serialized);
    m_RootHashDirty = true;
}

void DeltaTracker::UpdateSceneHash(const std::string& entitiesJson) {
    m_SceneHash = ComputeHash(entitiesJson);
    m_RootHashDirty = true;
}

void DeltaTracker::UpdateAssetHash(const std::string& assetsJson) {
    m_AssetHash = ComputeHash(assetsJson);
    m_RootHashDirty = true;
}

void DeltaTracker::RemoveEntityHash(uint32_t entityId) {
    m_PreviousEntityHashes[entityId] = m_EntityHashes[entityId];
    m_EntityHashes.erase(entityId);
    m_RootHashDirty = true;
}

Hash64 DeltaTracker::GetEntityHash(uint32_t entityId) const {
    auto it = m_EntityHashes.find(entityId);
    return (it != m_EntityHashes.end()) ? it->second : 0;
}

Hash64 DeltaTracker::GetRootHash() const {
    if (m_RootHashDirty) recomputeRootHash();
    return m_CachedRootHash;
}

std::string DeltaTracker::GetRootHashString() const {
    auto h = GetRootHash();
    std::stringstream ss;
    ss << "0x" << std::hex << h;
    return ss.str();
}

void DeltaTracker::recomputeRootHash() const {
    std::vector<std::pair<std::string, Hash64>> entries = {
        {"asset", m_AssetHash},
        {"scene", m_SceneHash}
    };

    // Include entity hashes in root hash calculation
    for (const auto& [entityId, hash] : m_EntityHashes) {
        std::string key = "entity_" + std::to_string(entityId);
        entries.emplace_back(key, hash);
    }

    std::sort(entries.begin(), entries.end());

    std::string combined;
    for (const auto& [label, hash] : entries) {
        combined += label;
        combined.append(reinterpret_cast<const char*>(&hash), sizeof(hash));
    }

    m_CachedRootHash = ComputeHash(combined);
    m_RootHashDirty = false;
}

DeltaResult DeltaTracker::ComputeDelta(Hash64 knownRootHash, const std::string& /*toolName*/,
                                       const nlohmann::json& /*args*/) const {
    DeltaResult result;
    result.newHash = GetRootHash();

    if (knownRootHash == result.newHash) {
        result.hasChanges = false;
        result.delta = {{"_unchanged", true}};
        return result;
    }

    size_t changedCount = 0;
    size_t deletedCount = 0;
    size_t totalCount = std::max(m_EntityHashes.size(), m_PreviousEntityHashes.size());

    for (const auto& [id, hash] : m_EntityHashes) {
        auto prevIt = m_PreviousEntityHashes.find(id);
        // Only count as "changed" if we have a meaningful previous hash (not 0)
        if (prevIt != m_PreviousEntityHashes.end() && prevIt->second != 0 && prevIt->second != hash) {
            changedCount++;
        }
    }
    for (const auto& [id, hash] : m_PreviousEntityHashes) {
        if (m_EntityHashes.find(id) == m_EntityHashes.end()) {
            deletedCount++;
        }
    }

    result.changeRatio = (totalCount > 0)
        ? static_cast<float>(changedCount + deletedCount) / totalCount
        : 0.0f;

    if (result.changeRatio > kDeltaThreshold && totalCount > 5) {
        result.hasChanges = true;
        result.snapshot = {
            {"_full_snapshot", true},
            {"reason", "change_ratio_exceeded"},
            {"ratio", result.changeRatio}
        };
    } else {
        result.hasChanges = (changedCount > 0 || deletedCount > 0);
        if (result.hasChanges) {
            result.delta = {
                {"_delta_version", result.newHash},
                {"changed_count", changedCount},
                {"deleted_count", deletedCount}
            };
        }
    }

    return result;
}

} // namespace MCP
} // namespace Prisma
