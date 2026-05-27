#pragma once

#include "Export.h"
#include "NavAgent.h"
#include <memory>

namespace Prisma {
namespace Navigation {

/**
 * @brief ECS 导航智能体组件
 *
 * 持有 NavAgent 实例，可附加到场景实体上。
 * 由 NavigationSystem 每帧更新。
 */
struct ENGINE_API NavAgentComponent {
    std::unique_ptr<NavAgent> agent;
    bool enabled = true;
    bool autoPathOnLoad = false;

    NavAgentComponent() = default;

    explicit NavAgentComponent(std::unique_ptr<NavAgent> navAgent)
        : agent(std::move(navAgent)) {}

    explicit NavAgentComponent(const NavAgentParams& params)
        : agent(std::make_unique<NavAgent>(params)) {}

    NavAgentComponent(NavAgentComponent&&) = default;
    NavAgentComponent& operator=(NavAgentComponent&&) = default;

    // 禁止拷贝
    NavAgentComponent(const NavAgentComponent&) = delete;
    NavAgentComponent& operator=(const NavAgentComponent&) = delete;

    /** 检查组件是否可用 */
    bool IsValid() const { return agent != nullptr && enabled; }

    /** 获取智能体引用 */
    NavAgent& GetAgent() {
        return *agent;
    }

    const NavAgent& GetAgent() const {
        return *agent;
    }

    /** 创建默认参数的智能体组件 */
    static NavAgentComponent CreateDefault() {
        NavAgentParams params;
        return NavAgentComponent(std::make_unique<NavAgent>(params));
    }

    /** 创建指定参数的智能体组件 */
    static NavAgentComponent CreateWithParams(const NavAgentParams& params) {
        return NavAgentComponent(std::make_unique<NavAgent>(params));
    }
};

} // namespace Navigation
} // namespace Prisma
