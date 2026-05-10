#include "SceneManager.h"
#include "Scene.h"
#include "logger/Logger.h"
#include "graphic/OrthographicCamera.h"

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

    // 创建默认 2D 正交相机
    auto camera = std::make_shared<Graphic::OrthographicCamera>();
    camera->SetProjection(0.0f, 1600.0f, 0.0f, 900.0f);
    m_currentScene->SetMainCamera(camera);

    m_currentScene->SetDirty(false);
}

Scene* SceneManager::GetCurrentScene() const {
    return m_currentScene.get();
}

bool SceneManager::LoadFromFile(const std::string& path) {
    // 让 Scene::Deserialize 创建场景对象
    auto newScene = std::make_shared<Scene>();
    if (!newScene->Deserialize(path)) {
        LOG_ERROR("SceneManager", "从文件加载场景失败: {0}", path);
        return false;
    }

    // 确保场景有主相机（fallback）
    if (!newScene->GetMainCamera()) {
        auto camera = std::make_shared<Graphic::OrthographicCamera>();
        camera->SetProjection(0.0f, 1600.0f, 0.0f, 900.0f);
        newScene->SetMainCamera(camera);
    }

    m_currentScene = std::move(newScene);
    LOG_INFO("SceneManager", "场景已加载: {0} ({1})", path, m_currentScene->GetName());
    return true;
}

}  // namespace Prisma
