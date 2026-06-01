#pragma once

#include "Export.h"
#include "ISubSystem.h"
#include "NavMesh.h"
#include "NavAgentComponent.h"
#include "PathFinder.h"
#include "PathSmoothing.h"
#include "core/Timestep.h"

#include <vector>
#include <memory>
#include <string>

namespace Prisma {
namespace Navigation {

/**
 * @brief 导航系统 - ISubSystem 实现
 *
 * 管理导航网格和所有导航智能体。
 * 每帧更新所有活跃智能体的路径跟随和避让行为。
 */
class ENGINE_API NavigationSystem : public ISubSystem {
public:
    NavigationSystem() = default;
    ~NavigationSystem() override = default;

    NavigationSystem(const NavigationSystem&) = delete;
    NavigationSystem& operator=(const NavigationSystem&) = delete;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "NavigationSystem"; }

    // ========== 导航网格 ==========

    /** 设置导航网格 */
    void SetNavMesh(std::unique_ptr<NavMesh> navMesh) {
        m_navMesh = std::move(navMesh);
    }

    /** 获取导航网格 */
    NavMesh* GetNavMesh() { return m_navMesh.get(); }
    const NavMesh* GetNavMesh() const { return m_navMesh.get(); }

    /** 是否已加载导航网格 */
    bool HasNavMesh() const { return m_navMesh != nullptr; }

    /** 从文件加载预烘焙的 NavMesh */
    bool LoadNavMeshFromFile(const std::string& filePath);

    /** 保存导航网格到二进制文件 */
    bool SaveNavMeshToFile(const std::string& filePath) const;

    // ========== 寻路器 ==========

    PathFinder& GetPathFinder() { return m_pathFinder; }
    const PathFinder& GetPathFinder() const { return m_pathFinder; }

    PathSmoother& GetPathSmoother() { return m_pathSmoother; }
    const PathSmoother& GetPathSmoother() const { return m_pathSmoother; }

    // ========== 智能体管理 ==========

    /** 注册智能体组件 */
    void RegisterAgent(NavAgentComponent* component);

    /** 注销智能体组件 */
    void UnregisterAgent(NavAgentComponent* component);

    /** 获取活跃智能体数量 */
    size_t GetActiveAgentCount() const { return m_agents.size(); }

    /** 使用 A* 在导航网格上寻路 */
    PathResult FindPath(const glm::dvec3& start,
                        const glm::dvec3& end) const {
        if (!m_navMesh) return PathResult{};
        PathResult result = m_pathFinder.FindPath(start, end, *m_navMesh);
        if (result.found) {
            result = m_pathSmoother.Smooth(result, *m_navMesh);
        }
        return result;
    }

private:
    std::unique_ptr<NavMesh> m_navMesh;
    PathFinder  m_pathFinder;
    PathSmoother m_pathSmoother;
    std::vector<NavAgentComponent*> m_agents;
};

} // namespace Navigation
} // namespace Prisma
