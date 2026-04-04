#include "pch.h"
#include "Scene.h"
#include "Camera.h"
#include "Logger.h"

namespace Prisma {

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
    m_gameObjects.push_back(gameObject);
    m_IsDirty = true;
}

void Scene::RemoveGameObject(GameObject* gameObject)
{
    m_gameObjects.erase(
        std::remove_if(m_gameObjects.begin(), m_gameObjects.end(),
            [gameObject](const std::shared_ptr<GameObject>& obj) {
                return obj.get() == gameObject;
            }),
        m_gameObjects.end()
    );
    m_IsDirty = true;
}

void Scene::Update(Timestep ts)
{
    for (auto& obj : m_gameObjects) {
        obj->Update(ts);
    }
}


const std::vector<std::shared_ptr<GameObject>>& Scene::GetGameObjects() const
{
    return m_gameObjects;
}

std::shared_ptr<Prisma::Graphic::ICamera> Scene::GetMainCamera()
{
    return m_mainCamera;
}

void Scene::SetMainCamera(std::shared_ptr<Prisma::Graphic::ICamera> camera)
{
    m_mainCamera = camera;
    LOG_INFO("Scene", "主相机已设置为 {0}", camera ? "有效相机" : "nullptr");
}

} // namespace Prisma
