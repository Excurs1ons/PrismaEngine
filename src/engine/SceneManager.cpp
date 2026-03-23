#include "SceneManager.h"
#include "TriangleExample.h"
#include "Logger.h"

namespace Prisma {

int SceneManager::Initialize() {
    TriangleExample example;
    m_currentScene = example.CreateExampleScene();
    LOG_INFO("Scene", "Example scene created.");
    return 0;
}

void SceneManager::Shutdown() {
    m_currentScene.reset();
}

void SceneManager::Update(Timestep ts) {
    if (m_currentScene) {
        m_currentScene->Update(ts);
    }
}

void SceneManager::CreateNewScene() {
    LOG_INFO("Scene", "Creating new empty scene...");
    m_currentScene = std::make_shared<Scene>();
}

Scene* SceneManager::GetCurrentScene() const {
    return m_currentScene.get();
}

} // namespace Prisma
