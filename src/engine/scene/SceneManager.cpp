#include "SceneManager.h"
#include "Scene.h"
#include "logger/Logger.h"
#include "core/AssetManager.h"
#include "app/Engine.h"
#include "transform/Camera.h"

namespace Prisma {

int SceneManager::Initialize() {
    CreateNewScene();
    LOG_DEBUG("Scene", "场景管理器已初始化。");
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
    LOG_DEBUG("Scene", "正在创建新的空场景...");
    m_currentScene = std::make_shared<Scene>();
    m_currentScene->SetName("未命名场景");

    // 创建默认透视相机节点
    auto cameraNode = m_currentScene->CreateNode("Main Camera");
    m_currentScene->AddComponent<Graphic::Camera>(cameraNode);

    m_currentScene->SetDirty(false);
}

Scene* SceneManager::GetCurrentScene() const {
    return m_currentScene.get();
}

bool SceneManager::LoadFromFile(const std::string& path) {
    // 通过 AssetManager 解析实际文件路径
    std::string actualPath = path;
    auto assetMgr = Engine::Get().GetAssetManager();
    if (assetMgr) {
        auto found = assetMgr->FindResource(path);
        if (found) actualPath = found->string();
    }

    auto newScene = std::make_shared<Scene>();
    if (!newScene->Deserialize(actualPath)) {
        LOG_ERROR("SceneManager", "从文件加载场景失败: {0}", path);
        return false;
    }

    // 确保场景有主相机（fallback）
    if (!newScene->GetMainCamera()) {
        LOG_WARN("SceneManager", "场景 '{0}' 没有相机节点，创建默认相机", newScene->GetName());
        auto cameraNode = newScene->CreateNode("Main Camera");
        newScene->AddComponent<Graphic::Camera>(cameraNode);
    }

    m_currentScene = std::move(newScene);
    LOG_INFO("SceneManager", "场景已加载: {0} ({1})", path, m_currentScene->GetName());
    return true;
}

}  // namespace Prisma
