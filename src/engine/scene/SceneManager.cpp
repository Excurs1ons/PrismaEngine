#include "SceneManager.h"
#include "Scene.h"
#include "logger/Logger.h"
#include "core/AssetManager.h"
#include "app/Engine.h"
#include "transform/Camera.h"

namespace Prisma {

int SceneManager::Initialize() {
    //CreateNewScene();
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

    auto data = Platform::ReadBinaryFile(actualPath.c_str());
    if (data.empty()) {
        LOG_ERROR("SceneManager", "读取场景数据失败: {0}", path);
        return false;
    }

    auto newScene = std::make_shared<Scene>();
    std::string buffer(data.begin(), data.end());
    if (!newScene->DeserializeFromMemory(buffer)) {
        LOG_ERROR("SceneManager", "解析场景失败: {0}", path);
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

void SceneManager::RegisterScene(const std::string& name, Scene* scene) {
    if (!scene) return;
    m_scenes[name] = scene;
}

void SceneManager::UnregisterScene(const std::string& name) {
    auto it = m_scenes.find(name);
    if (it != m_scenes.end()) {
        m_scenes.erase(it);
    }
}

void SceneManager::SwitchScene(const std::string& name, float fadeMs) {
    (void)fadeMs;

    auto it = m_scenes.find(name);
    if (it == m_scenes.end() || !it->second) return;

    if (m_onSceneWillLoad) m_onSceneWillLoad(name);

    m_currentSceneName = name;

    if (m_onSceneLoaded) m_onSceneLoaded(name);
}

void SceneManager::SwitchSceneAsync(const std::string& name) {
    m_transitionPending = true;
    m_pendingSceneName = name;
}

Scene* SceneManager::GetScene(const std::string& name) const {
    auto it = m_scenes.find(name);
    return it != m_scenes.end() ? it->second : nullptr;
}

void SceneManager::SetTransitionData(const std::string& key, const std::string& value) {
    m_transitionData[key] = value;
}

std::string SceneManager::GetTransitionData(const std::string& key) const {
    auto it = m_transitionData.find(key);
    return it != m_transitionData.end() ? it->second : "";
}

bool SceneManager::HasTransitionData(const std::string& key) const {
    return m_transitionData.find(key) != m_transitionData.end();
}

void SceneManager::ClearTransitionData() {
    m_transitionData.clear();
}

}  // namespace Prisma
