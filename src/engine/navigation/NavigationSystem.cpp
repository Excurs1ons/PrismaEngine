#include "navigation/NavigationSystem.h"
#include "Logger.h"

namespace Prisma {
namespace Navigation {

int NavigationSystem::Initialize() {
    LOG_INFO("NavigationSystem", "正在初始化导航系统...");

    m_navMesh = nullptr;
    m_agents.clear();

    // 配置寻路器
    PathFinder::Config pfConfig;
    pfConfig.heuristicWeight = 1.0;
    pfConfig.maxSearchNodes  = 10000;
    pfConfig.useEdgeMidpoints = true;
    m_pathFinder.SetConfig(pfConfig);

    // 配置路径平滑器
    PathSmoother::Config psConfig;
    psConfig.enableSmoothing = true;
    psConfig.validateWithRaycast = false;
    m_pathSmoother.SetConfig(psConfig);

    LOG_INFO("NavigationSystem", "导航系统初始化完成");
    return 0;
}

void NavigationSystem::Shutdown() {
    LOG_INFO("NavigationSystem", "正在关闭导航系统...");

    m_agents.clear();
    m_navMesh.reset();

    LOG_INFO("NavigationSystem", "导航系统已关闭");
}

void NavigationSystem::Update(Timestep ts) {
    double deltaTime = static_cast<double>(ts);

    // 如果没有导航网格或没有智能体，跳过
    if (!m_navMesh || m_agents.empty()) return;

    // 收集邻近智能体列表（用于动态避让）
    // 注意：此处所有智能体互相感知，实际应用可按空间划分
    std::vector<NavAgent*> allAgentPtrs;
    allAgentPtrs.reserve(m_agents.size());
    for (auto* comp : m_agents) {
        if (comp && comp->IsValid()) {
            allAgentPtrs.push_back(comp->agent.get());
        }
    }

    // 更新每个智能体
    for (auto* comp : m_agents) {
        if (!comp || !comp->IsValid()) continue;

        NavAgent& agent = comp->GetAgent();
        if (agent.GetState() == NavAgentState::Idle
            || agent.GetState() == NavAgentState::Arrived
            || agent.GetState() == NavAgentState::Stuck) {
            continue;
        }

        // 在 Pathfinding 状态时需要计算路径
        if (agent.GetState() == NavAgentState::Pathfinding) {
            PathResult result = m_pathFinder.FindPath(
                agent.GetPosition(),
                agent.GetTarget(),
                *m_navMesh);

            if (result.found) {
                result = m_pathSmoother.Smooth(result, *m_navMesh);
                agent.SetPath(result);
            } else {
                LOG_WARNING("NavigationSystem",
                            "Agent pathfinding failed from ({:.2f}, {:.2f}, {:.2f}) "
                            "to ({:.2f}, {:.2f}, {:.2f})",
                            agent.GetPosition().x, agent.GetPosition().y, agent.GetPosition().z,
                            agent.GetTarget().x, agent.GetTarget().y, agent.GetTarget().z);
                agent.Stop();
                continue;
            }
        }

        // 更新智能体（传递其他智能体用于避让）
        agent.Update(deltaTime, m_navMesh.get(), &allAgentPtrs);
    }
}

void NavigationSystem::RegisterAgent(NavAgentComponent* component) {
    if (component) {
        m_agents.push_back(component);
        LOG_INFO("NavigationSystem", "智能体组件已注册 (总数: {})", m_agents.size());
    }
}

void NavigationSystem::UnregisterAgent(NavAgentComponent* component) {
    for (auto it = m_agents.begin(); it != m_agents.end(); ++it) {
        if (*it == component) {
            m_agents.erase(it);
            LOG_INFO("NavigationSystem", "智能体组件已注销 (剩余: {})", m_agents.size());
            return;
        }
    }
}

bool NavigationSystem::LoadNavMeshFromFile(const std::string& filePath) {
    LOG_INFO("NavigationSystem", "正在从文件加载导航网格: {}", filePath);
    // TODO: 实现 NavMesh 的二进制序列化加载
    // 当前仅作为桩函数
    LOG_WARNING("NavigationSystem", "NavMesh 文件加载功能尚未实现: {}", filePath);
    return false;
}

} // namespace Navigation
} // namespace Prisma
