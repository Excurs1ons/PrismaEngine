#include "SceneManager.h"
#include "TriangleExample.h"
#include "Logger.h"

namespace Prisma {

std::shared_ptr<SceneManager> SceneManager::Get() {
    static std::shared_ptr<SceneManager> instance = std::shared_ptr<SceneManager>(new SceneManager());
    return instance;
}

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

Scene* SceneManager::GetCurrentScene() const {
    return m_currentScene.get();
}

} // namespace Prisma
