#include "Scene.h"
#include "Camera.h"
#include "Logger.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
    m_gameObjects.push_back(gameObject);
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
}

void Scene::Update(float deltaTime)
{
    for (auto& obj : m_gameObjects) {
        obj->Update(deltaTime);
    }
}


const std::vector<std::shared_ptr<GameObject>>& Scene::GetGameObjects() const
{
    return m_gameObjects;
}

std::shared_ptr<PrismaEngine::Graphic::ICamera> Scene::GetMainCamera()
{
    return m_mainCamera;
}

void Scene::SetMainCamera(std::shared_ptr<PrismaEngine::Graphic::ICamera> camera)
{
    m_mainCamera = camera;
    LOG_INFO("Scene", "Main camera set to {0}", camera ? "valid camera" : "nullptr");
}