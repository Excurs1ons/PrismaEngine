#pragma once

#include "AABB2D.h"
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Prisma::Physics2D {

enum class TriggerEvent { Enter, Exit, Stay };

using TriggerCallback = std::function<void(uint32_t triggerId, uint32_t entityId, TriggerEvent event)>;

struct TriggerVolume {
    uint32_t id;
    AABB2D bounds;
    bool isActive;
    void* userData;
};

class TriggerSystem {
public:
    TriggerSystem();
    ~TriggerSystem() = default;

    uint32_t RegisterTrigger(const AABB2D& bounds, TriggerCallback callback, void* userData = nullptr);
    void UnregisterTrigger(uint32_t triggerId);
    void UpdateTriggerBounds(uint32_t triggerId, const AABB2D& newBounds);

    void Update(const std::vector<std::pair<uint32_t, AABB2D>>& entityAABBs);

    void Clear();

private:
    struct TriggerEntry {
        TriggerVolume volume;
        TriggerCallback callback;
        std::unordered_set<uint32_t> entitiesInside;
    };

    std::unordered_map<uint32_t, TriggerEntry> m_triggers;
    uint32_t m_nextId = 1;
};

} // namespace Prisma::Physics2D
