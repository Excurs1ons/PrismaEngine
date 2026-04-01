#include "SceneManager.h"
#include "TriangleExample.h"
#include "Logger.h"

namespace Prisma {

int SceneManager::Initialize() {
    TriangleExample example;
    m_currentScene = example.CreateExampleScene();
    LOG_INFO("Scene", "示例场景已创建。");
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
    LOG_INFO("Scene", "正在创建新的空场景...");
    m_currentScene = std::make_shared<Scene>();
    m_currentScene->SetName("未命名场景");
    m_currentScene->SetDirty(false);
}

Scene* SceneManager::GetCurrentScene() const {
    return m_currentScene.get();
}

} // namespace Prisma
