#include "SceneManager.h"
#include "TriangleExample.h"
#include <Logger.h>
namespace PrismaEngine {
void SceneManager::Shutdown() {}
void SceneManager::Update(float delta_time) {
    if (m_currentScene) {
        m_currentScene->Update(delta_time);
    }
}
Scene* SceneManager::GetCurrentScene() const {
    return m_currentScene.get();
}

bool SceneManager::Initialize() {

    // 使用TriangleExample创建示例场景
    TriangleExample example;
    const auto SCENE           = example.CreateExampleScene();
    this->m_currentScene = SCENE;  // store for later
    LOG_INFO("Application", "Example scene created with triangles and camera");

    return true;
}

}  // namespace Engine