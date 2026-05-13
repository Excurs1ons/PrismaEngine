#include "pch.h"
#include "Scene.h"
#include "Camera.h"
#include "Logger.h"
#include "core/EntityManager.h"
#include "graphic/OrthographicCamera.h"
#include <glaze/glaze.hpp>
#include <array>
#include <vector>

namespace Prisma {

// ── 场景文件顶层 JSON 结构 (SoA 兼容版) ──
struct NodeData {
    std::string name;
    std::array<float, 2> position = {0.0f, 0.0f};
    float rotation = 0.0f;
    std::array<float, 2> scale = {1.0f, 1.0f};
};

struct SceneCameraData {
    std::array<float, 4> projection = {0.0f, 1920.0f, 0.0f, 1080.0f};
};

struct SceneFileData {
    std::string name;
    SceneCameraData camera;
    std::vector<NodeData> nodes;
};

} // namespace Prisma

template <>
struct glz::meta<Prisma::NodeData> {
    static constexpr auto value = glz::object(
        "name", &Prisma::NodeData::name,
        "position", &Prisma::NodeData::position,
        "rotation", &Prisma::NodeData::rotation,
        "scale", &Prisma::NodeData::scale
    );
};

template <>
struct glz::meta<Prisma::SceneCameraData> {
    static constexpr auto value = glz::object("projection", &Prisma::SceneCameraData::projection);
};

template <>
struct glz::meta<Prisma::SceneFileData> {
    static constexpr auto value = glz::object(
        "name", &Prisma::SceneFileData::name,
        "camera", &Prisma::SceneFileData::camera,
        "nodes", &Prisma::SceneFileData::nodes
    );
};

namespace Prisma {

Scene::Scene() {}

Scene::~Scene() {
    for (auto node : m_nodes) {
        node.Destroy();
    }
}

Node Scene::CreateNode(const std::string& name) {
    Node node = EntityManager::Get().CreateNode();
    // TODO: 存储名称到 SoA 或额外的名称表
    m_nodes.push_back(node);
    m_IsDirty = true;
    return node;
}

void Scene::RemoveNode(Node node) {
    auto it = std::find(m_nodes.begin(), m_nodes.end(), node);
    if (it != m_nodes.end()) {
        node.Destroy();
        m_nodes.erase(it);
        m_IsDirty = true;
    }
}

void Scene::Update(Timestep /*ts*/) {
    // 逻辑更新现在主要由 ScriptEngine 或 System 处理
    // Scene 仅负责维护 Node 列表的有效性
}

std::shared_ptr<Prisma::Graphic::ICamera> Scene::GetMainCamera() {
    return m_mainCamera;
}

void Scene::SetMainCamera(std::shared_ptr<Prisma::Graphic::ICamera> camera) {
    m_mainCamera = std::move(camera);
    LOG_DEBUG("Scene", "主相机已设置为 {0}", m_mainCamera ? "有效相机" : "nullptr");
}

bool Scene::Deserialize(const std::string& path) {
    SceneFileData sfd;
    auto error = glz::read_file_jsonc(sfd, path, std::string{});
    if (error) {
        LOG_ERROR("Scene", "解析场景文件失败: {0}", glz::format_error(error, ""));
        return false;
    }

    SetName(sfd.name);

    // 创建正交相机并设为主相机
    auto& p = sfd.camera.projection;
    auto camera = std::make_shared<Graphic::OrthographicCamera>();
    camera->SetProjection(p[0], p[1], p[2], p[3]);
    SetMainCamera(camera);

    // 创建 Node 并恢复 SoA 数据
    for (auto& nd : sfd.nodes) {
        Node node = CreateNode(nd.name);
        node.SetPosition({nd.position[0], nd.position[1]});
        node.SetRotation(nd.rotation);
        node.SetScale({nd.scale[0], nd.scale[1]});
    }

    LOG_DEBUG("Scene", "场景已加载: {0} ({1} 个 Node)", sfd.name, m_nodes.size());
    return true;
}

bool Scene::Serialize(const std::string& path) const {
    SceneFileData sfd;
    sfd.name = m_Name;

    // TODO: 相机投影保存

    // Node 数据
    for (auto node : m_nodes) {
        NodeData nd;
        nd.name = "Node"; // TODO: 实际名称
        nd.position = { node.GetX(), node.GetY() };
        nd.rotation = node.GetRotation();
        nd.scale = { node.GetScale().x, node.GetScale().y };
        sfd.nodes.push_back(nd);
    }

    auto error = glz::write_file_json(sfd, path, std::string{});
    if (error) {
        LOG_ERROR("Scene", "场景序列化失败: {0}", glz::format_error(error, ""));
        return false;
    }

    LOG_INFO("Scene", "场景已保存: {0} ({1} 个 Node)", path, m_nodes.size());
    return true;
}

} // namespace Prisma
