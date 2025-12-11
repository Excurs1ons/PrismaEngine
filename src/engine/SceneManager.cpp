#include "SceneManager.h"
#include "TriangleExample.h"
#include <Logger.h>
namespace Engine {
void SceneManager::Shutdown() {}
void SceneManager::Update(float deltaTime) {
    if (m_currentScene) {
        m_currentScene->Update(deltaTime);
    }
}
Scene* SceneManager::GetCurrentScene() {
    return m_currentScene.get();
}

bool SceneManager::Initialize() {

    // 使用TriangleExample创建示例场景
    TriangleExample example;
    auto scene           = example.CreateExampleScene();
    this->m_currentScene = scene;  // store for later
    LOG_INFO("Application", "Example scene created with triangles and camera");

    return true;
}

}  // namespace Engine