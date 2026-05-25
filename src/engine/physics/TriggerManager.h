#pragma once

#include "physics/TriggerVolume.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace Prisma {
namespace Physics {

// 触发管理器 - 管理所有 TriggerVolume 并检测实体进出
class TriggerManager {
public:
    TriggerManager() = default;
    ~TriggerManager() = default;

    // 添加触发体积
    uint32_t addTrigger(const TriggerVolume& trigger) {
        uint32_t id = m_nextTriggerId++;
        m_triggers.push_back({ id, trigger });
        return id;
    }

    // 移除触发体积
    void removeTrigger(uint32_t id) {
        auto it = std::find_if(m_triggers.begin(), m_triggers.end(),
            [id](const auto& pair) { return pair.first == id; });
        if (it != m_triggers.end()) {
            m_triggers.erase(it);
        }
    }

    // 获取触发体积
    TriggerVolume* getTrigger(uint32_t id) {
        auto it = std::find_if(m_triggers.begin(), m_triggers.end(),
            [id](const auto& pair) { return pair.first == id; });
        return it != m_triggers.end() ? &it->second : nullptr;
    }

    // 清除所有触发体积
    void clear() { m_triggers.clear(); }

    // 更新所有触发体积 - 检测实体 AABB 与触发区的重叠
    // entityAABBs: map of entityId -> world-space AABB
    void update(double dt, const std::vector<std::pair<uint32_t, AABB>>& entityAABBs) {
        for (auto& [id, trigger] : m_triggers) {
            if (!trigger.enabled) continue;

            std::unordered_set<uint32_t> currentOverlaps;

            for (const auto& [entityId, entityAABB] : entityAABBs) {
                if (trigger.overlaps(entityAABB)) {
                    currentOverlaps.insert(entityId);
                }
            }

            // 检测新的进入
            for (uint32_t entityId : currentOverlaps) {
                if (trigger.previousOverlaps.find(entityId) == trigger.previousOverlaps.end()) {
                    // 新进入
                    if (trigger.onEnter) {
                        trigger.onEnter(entityId);
                    }
                } else {
                    // 停留
                    if (trigger.onStay) {
                        trigger.onStay(entityId);
                    }
                }
            }

            // 检测离开
            for (uint32_t entityId : trigger.previousOverlaps) {
                if (currentOverlaps.find(entityId) == currentOverlaps.end()) {
                    if (trigger.onExit) {
                        trigger.onExit(entityId);
                    }
                }
            }

            // 更新上一帧状态
            trigger.previousOverlaps = std::move(currentOverlaps);
        }
    }

    // 获取触发器数量
    size_t getTriggerCount() const { return m_triggers.size(); }

private:
    std::vector<std::pair<uint32_t, TriggerVolume>> m_triggers;
    uint32_t m_nextTriggerId = 1;
};

} // namespace Physics
} // namespace Prisma
