#include "ai/AISystem.h"
#include "ai/BuiltInTasks.h"
#include "Logger.h"

namespace Prisma {
namespace AI {

int AISystem::Initialize() {
    m_components.clear();
    m_globalEnabled = true;

    LOG_INFO("AI", "AI 系统已初始化 (内置任务已注册)");
    return 0;
}

void AISystem::Shutdown() {
    m_components.clear();
    m_globalEnabled = false;

    LOG_INFO("AI", "AI 系统已关闭");
}

void AISystem::Update(Timestep ts) {
    if (!m_globalEnabled) return;
    if (m_components.empty()) return;

    double dt = static_cast<double>(ts);
    double currentTime = ts.GetSeconds();

    for (auto* component : m_components) {
        if (!component || !component->enabled) continue;

        // 更新间隔控制
        if (component->updateInterval > 0.0) {
            double timeSinceLastUpdate = currentTime - component->m_lastUpdateTime;
            if (timeSinceLastUpdate < component->updateInterval) {
                continue; // 未到更新间隔
            }
            component->m_lastUpdateTime = currentTime;
        }

        // 更新行为树
        if (component->behaviorTree && component->blackboard) {
            BTNode::Status status =
                component->behaviorTree->Execute(*component->blackboard, dt);

            // Running 状态表示节点正在执行中，无需额外处理
            // Success/Failure 由行为树内部处理
            (void)status;
        }

        // 更新有限状态机
        if (component->stateMachine) {
            component->stateMachine->Update(dt);
        }
    }
}

void AISystem::RegisterComponent(AIComponent* component) {
    if (component) {
        m_components.push_back(component);
        LOG_DEBUG("AI", "AI 组件已注册 (总数: {})", m_components.size());
    }
}

void AISystem::UnregisterComponent(AIComponent* component) {
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        if (*it == component) {
            m_components.erase(it);
            LOG_DEBUG("AI", "AI 组件已注销 (剩余: {})", m_components.size());
            return;
        }
    }
}

} // namespace AI
} // namespace Prisma
