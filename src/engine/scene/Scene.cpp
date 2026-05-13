#include "pch.h"
#include "Scene.h"
#include "Camera.h"
#include "Logger.h"
#include "core/EntityManager.h"
#include "graphic/OrthographicCamera.h"
#include <glaze/glaze.hpp>
#include <array>
#include <vector>
#include <optional>

namespace Prisma {

// ── 场景文件实体-组件结构 ──
/// SpriteRenderer 组件数据（JSON 序列化用 std::array，再写入 SoA）
struct SpriteRendererData {
    std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA
    std::array<float, 2> size = {100.0f, 100.0f};            // width, height
};

/// 实体变换数据
struct EntityTransformData {
    std::array<float, 2> position = {0.0f, 0.0f};
    float rotation = 0.0f;
};

/// 实体数据（含可选组件）
struct EntityData {
    std::string name;
    EntityTransformData transform;
    std::optional<SpriteRendererData> spriteRenderer;
};

struct SceneCameraData {
    std::array<float, 4> projection = {0.0f, 1920.0f, 0.0f, 1080.0f};
};

struct SceneFileData {
    std::string name;
    SceneCameraData camera;
    std::vector<EntityData> entities;
};

} // namespace Prisma

template <>
struct glz::meta<Prisma::SpriteRendererData> {
    static constexpr auto value = glz::object(
        "color", &Prisma::SpriteRendererData::color,
        "size",  &Prisma::SpriteRendererData::size
    );
};

template <>
struct glz::meta<Prisma::EntityTransformData> {
    static constexpr auto value = glz::object(
        "position", &Prisma::EntityTransformData::position,
        "rotation", &Prisma::EntityTransformData::rotation
    );
};

template <>
struct glz::meta<Prisma::EntityData> {
    static constexpr auto value = glz::object(
        "name",           &Prisma::EntityData::name,
        "transform",      &Prisma::EntityData::transform,
        "spriteRenderer", &Prisma::EntityData::spriteRenderer
    );
};

template <>
struct glz::meta<Prisma::SceneCameraData> {
    static constexpr auto value = glz::object("projection", &Prisma::SceneCameraData::projection);
};

template <>
struct glz::meta<Prisma::SceneFileData> {
    static constexpr auto value = glz::object(
        "name",     &Prisma::SceneFileData::name,
        "camera",   &Prisma::SceneFileData::camera,
        "entities", &Prisma::SceneFileData::entities
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

    // 创建 Entity 并恢复数据
    for (auto& ed : sfd.entities) {
        Node node = CreateNode(ed.name);
        node.SetPosition({ed.transform.position[0], ed.transform.position[1]});
        node.SetRotation(ed.transform.rotation);

        // 加载 SpriteRenderer 组件（颜色 + 大小写入 SoA）
        if (ed.spriteRenderer) {
            auto& sr = *ed.spriteRenderer;
            auto& em = EntityManager::Get();
            auto* rb = em.GetRenderBuffer();
            uint32_t idx = node.GetIndex();
            rb->colorR[idx] = sr.color[0];
            rb->colorG[idx] = sr.color[1];
            rb->colorB[idx] = sr.color[2];
            rb->colorA[idx] = sr.color[3];
            rb->sizeW[idx] = sr.size[0];
            rb->sizeH[idx] = sr.size[1];
        }
    }

    LOG_DEBUG("Scene", "场景已加载: {0} ({1} 个 Entity)", sfd.name, m_nodes.size());
    return true;
}

bool Scene::Serialize(const std::string& path) const {
    SceneFileData sfd;
    sfd.name = m_Name;

    // TODO: 相机投影保存

    // Entity 数据
    for (auto node : m_nodes) {
        EntityData ed;
        ed.name = "Node"; // TODO: 实际名称
        ed.transform.position = { node.GetX(), node.GetY() };
        ed.transform.rotation = node.GetRotation();

        // TODO: 从 SoA 读取 spriteRenderer 数据
        // auto& em = EntityManager::Get();
        // auto* rb = em.GetRenderBuffer();
        // uint32_t idx = node.GetIndex();
        // SpriteRendererData sr;
        // sr.color = { rb->colorR[idx], rb->colorG[idx], rb->colorB[idx], rb->colorA[idx] };
        // sr.size  = { rb->sizeW[idx], rb->sizeH[idx] };
        // ed.spriteRenderer = sr;

        sfd.entities.push_back(ed);
    }

    auto error = glz::write_file_json(sfd, path, std::string{});
    if (error) {
        LOG_ERROR("Scene", "场景序列化失败: {0}", glz::format_error(error, ""));
        return false;
    }

    LOG_INFO("Scene", "场景已保存: {0} ({1} 个 Entity)", path, m_nodes.size());
    return true;
}

} // namespace Prisma
