#include "SceneManager.h"
#include "TriangleExample.h"
#include "Logger.h"

namespace PrismaEngine {

std::shared_ptr<SceneManager> SceneManager::GetInstance() {
    static std::shared_ptr<SceneManager> instance = std::make_shared<SceneManager>();
    return instance;
}

int SceneManager::Initialize() {
    TriangleExample example;
    m_currentScene = example.CreateExampleScene();
    LOG_INFO("Scene", "Example scene created.");
    return true;
}

void SceneManager::Shutdown() {
    m_currentScene.reset();
}

void SceneManager::Update(float deltaTime) {
    if (m_currentScene) {
        m_currentScene->Update(deltaTime);
    }
}

Scene* SceneManager::GetCurrentScene() const {
    return m_currentScene.get();
}

}  // namespace PrismaEngine
