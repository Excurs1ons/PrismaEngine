#include "SceneManager.h"
#include "logger/Logger.h"

namespace Prisma {

int SceneManager::Initialize() {
    CreateNewScene();
    LOG_INFO("Scene", "场景管理器已初始化。");
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

}  // namespace Prisma
