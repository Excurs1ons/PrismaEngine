#include "navigation/NavigationSystem.h"
#include "Logger.h"
#include <fstream>

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

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("NavigationSystem", "无法打开文件: {}", filePath);
        return false;
    }

    // 读取魔数 "NAVM"
    char magic[4];
    file.read(magic, 4);
    if (file.gcount() != 4 ||
        magic[0] != 'N' || magic[1] != 'A' ||
        magic[2] != 'V' || magic[3] != 'M') {
        LOG_ERROR("NavigationSystem", "无效的 NavMesh 文件格式: {}", filePath);
        return false;
    }

    // 读取版本号
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) {
        LOG_ERROR("NavigationSystem", "不支持的 NavMesh 版本: {}", version);
        return false;
    }

    // 读取多边形数量
    uint32_t polygonCount = 0;
    file.read(reinterpret_cast<char*>(&polygonCount), sizeof(polygonCount));

    std::vector<NavPolygon> polygons;
    polygons.reserve(polygonCount);

    for (uint32_t i = 0; i < polygonCount; ++i) {
        NavPolygon poly;

        // 读取三个顶点 (3 * glm::dvec3 = 9 个 double)
        file.read(reinterpret_cast<char*>(&poly.vertices[0].x), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[0].y), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[0].z), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[1].x), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[1].y), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[1].z), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[2].x), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[2].y), sizeof(double));
        file.read(reinterpret_cast<char*>(&poly.vertices[2].z), sizeof(double));

        // 读取三个邻居索引 (3 个 int32_t)
        file.read(reinterpret_cast<char*>(&poly.neighbors[0]), sizeof(int32_t));
        file.read(reinterpret_cast<char*>(&poly.neighbors[1]), sizeof(int32_t));
        file.read(reinterpret_cast<char*>(&poly.neighbors[2]), sizeof(int32_t));

        // 读取区域类型 (1 字节)
        uint8_t areaType = 0;
        file.read(reinterpret_cast<char*>(&areaType), sizeof(uint8_t));
        poly.areaType = static_cast<AreaType>(areaType);

        // 读取多边形 ID (4 字节)
        file.read(reinterpret_cast<char*>(&poly.id), sizeof(uint32_t));

        // 跳过 3 字节填充
        char padding[3];
        file.read(padding, 3);

        if (!file.good()) {
            LOG_ERROR("NavigationSystem", "读取多边形数据失败，索引: {}", i);
            return false;
        }

        polygons.push_back(poly);
    }

    file.close();

    auto navMesh = std::make_unique<NavMesh>(std::move(polygons));
    SetNavMesh(std::move(navMesh));

    LOG_INFO("NavigationSystem", "成功加载导航网格: {} 个多边形", polygonCount);
    return true;
}

bool NavigationSystem::SaveNavMeshToFile(const std::string& filePath) const {
    if (!m_navMesh) {
        LOG_ERROR("NavigationSystem", "没有导航网格可保存");
        return false;
    }

    LOG_INFO("NavigationSystem", "正在保存导航网格到文件: {}", filePath);

    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("NavigationSystem", "无法创建文件: {}", filePath);
        return false;
    }

    // 写入魔数 "NAVM"
    const char magic[4] = {'N', 'A', 'V', 'M'};
    file.write(magic, 4);

    // 写入版本号
    const uint32_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // 写入多边形数量
    const auto& polygons = m_navMesh->GetPolygons();
    const uint32_t polygonCount = static_cast<uint32_t>(polygons.size());
    file.write(reinterpret_cast<const char*>(&polygonCount), sizeof(polygonCount));

    // 写入每个多边形
    for (const auto& poly : polygons) {
        // 写入三个顶点 (3 * glm::dvec3 = 9 个 double)
        file.write(reinterpret_cast<const char*>(&poly.vertices[0].x), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[0].y), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[0].z), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[1].x), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[1].y), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[1].z), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[2].x), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[2].y), sizeof(double));
        file.write(reinterpret_cast<const char*>(&poly.vertices[2].z), sizeof(double));

        // 写入三个邻居索引 (3 个 int32_t)
        file.write(reinterpret_cast<const char*>(&poly.neighbors[0]), sizeof(int32_t));
        file.write(reinterpret_cast<const char*>(&poly.neighbors[1]), sizeof(int32_t));
        file.write(reinterpret_cast<const char*>(&poly.neighbors[2]), sizeof(int32_t));

        // 写入区域类型 (1 字节)
        const uint8_t areaType = static_cast<uint8_t>(poly.areaType);
        file.write(reinterpret_cast<const char*>(&areaType), sizeof(uint8_t));

        // 写入多边形 ID (4 字节)
        file.write(reinterpret_cast<const char*>(&poly.id), sizeof(uint32_t));

        // 写入 3 字节填充 (0)
        const char padding[3] = {0, 0, 0};
        file.write(padding, 3);
    }

    file.close();

    LOG_INFO("NavigationSystem", "成功保存导航网格: {} 个多边形", polygonCount);
    return true;
}

} // namespace Navigation
} // namespace Prisma
