#include "pch.h"
#include "Scene.h"
#include "Camera.h"
#include "Logger.h"
#include "graphic/OrthographicCamera.h"
#include <glaze/glaze.hpp>
#include <array>
#include <vector>

namespace Prisma {

// ── 场景文件顶层 JSON 结构 ──
struct SceneCameraData {
    std::array<float, 4> projection = {0.0f, 1920.0f, 0.0f, 1080.0f};
};
struct SceneFileData {
    std::string name;
    SceneCameraData camera;
    std::vector<GameObject::Data> gameObjects;
};

} // namespace Prisma

template <>
struct glz::meta<Prisma::SceneCameraData> {
    static constexpr auto value = glz::object("projection", &Prisma::SceneCameraData::projection);
};
template <>
struct glz::meta<Prisma::SceneFileData> {
    static constexpr auto value = glz::object(
        "name", &Prisma::SceneFileData::name,
        "camera", &Prisma::SceneFileData::camera,
        "gameObjects", &Prisma::SceneFileData::gameObjects
    );
};

namespace Prisma {

Scene::Scene() {}

Scene::~Scene() {}

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject) {
    m_gameObjects.push_back(std::move(gameObject));
    m_IsDirty = true;
}

void Scene::RemoveGameObject(GameObject* gameObject) {
    m_gameObjects.erase(
        std::remove_if(m_gameObjects.begin(), m_gameObjects.end(),
            [gameObject](const std::shared_ptr<GameObject>& obj) {
                return obj.get() == gameObject;
            }),
        m_gameObjects.end()
    );
    m_IsDirty = true;
}

void Scene::Update(Timestep ts) {
    for (auto& obj : m_gameObjects) {
        obj->Update(ts);
    }
}

const std::vector<std::shared_ptr<GameObject>>& Scene::GetGameObjects() const {
    return m_gameObjects;
}

std::shared_ptr<Prisma::Graphic::ICamera> Scene::GetMainCamera() {
    return m_mainCamera;
}

void Scene::SetMainCamera(std::shared_ptr<Prisma::Graphic::ICamera> camera) {
    m_mainCamera = std::move(camera);
    LOG_INFO("Scene", "主相机已设置为 {0}", m_mainCamera ? "有效相机" : "nullptr");
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

    // 创建 GameObject 并恢复数据
    for (auto& god : sfd.gameObjects) {
        auto go = std::make_shared<GameObject>(god.name);
        go->SetData(god);
        AddGameObject(go);
    }

    LOG_INFO("Scene", "场景已加载: {0} ({1} 个对象)", sfd.name, m_gameObjects.size());
    return true;
}

bool Scene::Serialize(const std::string& path) const {
    SceneFileData sfd;
    sfd.name = m_Name;

    // 相机投影
    if (m_mainCamera) {
        auto ortho = dynamic_cast<Graphic::OrthographicCamera*>(m_mainCamera.get());
        if (ortho) {
            // 通过读写字符串获取投影（OrthographicCamera 未提供 GetProjection）
            // 直接用当前投影数值构造
            // TODO: 当 OrthographicCamera 提供 GetProjection 时直接读取
        }
    }

    // GameObject 数据
    for (auto& go : m_gameObjects) {
        sfd.gameObjects.push_back(go->GetData());
    }

    auto error = glz::write_file_json(sfd, path, std::string{});
    if (error) {
        LOG_ERROR("Scene", "场景序列化失败: {0}", glz::format_error(error, ""));
        return false;
    }

    LOG_INFO("Scene", "场景已保存: {0} ({1} 个对象)", path, m_gameObjects.size());
    return true;
}

} // namespace Prisma
