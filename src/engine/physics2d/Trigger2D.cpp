#include "Trigger2D.h"

namespace Prisma::Physics2D {

TriggerSystem::TriggerSystem()
    : m_nextId(1)
{
}

uint32_t TriggerSystem::RegisterTrigger(const AABB2D& bounds, TriggerCallback callback, void* userData)
{
    uint32_t id = m_nextId++;

    TriggerEntry entry;
    entry.volume.id = id;
    entry.volume.bounds = bounds;
    entry.volume.isActive = true;
    entry.volume.userData = userData;
    entry.callback = std::move(callback);

    m_triggers[id] = std::move(entry);
    return id;
}

void TriggerSystem::UnregisterTrigger(uint32_t triggerId)
{
    m_triggers.erase(triggerId);
}

void TriggerSystem::UpdateTriggerBounds(uint32_t triggerId, const AABB2D& newBounds)
{
    auto it = m_triggers.find(triggerId);
    if (it != m_triggers.end()) {
        it->second.volume.bounds = newBounds;
    }
}

void TriggerSystem::Update(const std::vector<std::pair<uint32_t, AABB2D>>& entityAABBs)
{
    for (auto& [triggerId, entry] : m_triggers) {
        if (!entry.volume.isActive) continue;

        std::unordered_set<uint32_t> currentFrameEntities;

        for (const auto& [entityId, entityAABB] : entityAABBs) {
            if (entry.volume.bounds.Intersects(entityAABB)) {
                currentFrameEntities.insert(entityId);
            }
        }

        for (uint32_t entityId : currentFrameEntities) {
            if (entry.entitiesInside.find(entityId) == entry.entitiesInside.end()) {
                if (entry.callback) {
                    entry.callback(triggerId, entityId, TriggerEvent::Enter);
                }
            } else {
                if (entry.callback) {
                    entry.callback(triggerId, entityId, TriggerEvent::Stay);
                }
            }
        }

        for (uint32_t entityId : entry.entitiesInside) {
            if (currentFrameEntities.find(entityId) == currentFrameEntities.end()) {
                if (entry.callback) {
                    entry.callback(triggerId, entityId, TriggerEvent::Exit);
                }
            }
        }

        entry.entitiesInside.swap(currentFrameEntities);
    }
}

void TriggerSystem::Clear()
{
    m_triggers.clear();
}

} // namespace Prisma::Physics2D
