#pragma once

#include "Export.h"
#include "ISubSystem.h"
#include "core/Timestep.h"
#include "ai/AIComponent.h"

#include <vector>
#include <memory>
#include <string>

namespace Prisma {
namespace AI {

/**
 * @brief AI 系统 — ISubSystem 实现
 *
 * 管理所有 AI 组件，每帧更新行为树和有限状态机。
 * 在 Initialize 阶段注册内置任务。
 */
class ENGINE_API AISystem : public ISubSystem {
public:
    AISystem() = default;
    ~AISystem() override = default;

    AISystem(const AISystem&) = delete;
    AISystem& operator=(const AISystem&) = delete;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "AISystem"; }

    // ========== AI 组件管理 ==========

    /** 注册 AI 组件 */
    void RegisterComponent(AIComponent* component);

    /** 注销 AI 组件 */
    void UnregisterComponent(AIComponent* component);

    /** 获取活跃 AI 组件数量 */
    size_t GetActiveComponentCount() const { return m_components.size(); }

    // ========== 全局控制 ==========

    /** 启用/禁用所有 AI */
    void SetGlobalEnabled(bool enabled) { m_globalEnabled = enabled; }

    /** 检查 AI 是否全局启用 */
    bool IsGlobalEnabled() const { return m_globalEnabled; }

private:
    std::vector<AIComponent*> m_components;
    bool m_globalEnabled = true;
};

} // namespace AI
} // namespace Prisma
