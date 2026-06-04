#pragma once
#include "../core/ISubSystem.h"
#include "Scene.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace Prisma {

class ENGINE_API SceneManager : public ISubSystem {
public:
    SceneManager()           = default;
    ~SceneManager() override = default;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "SceneManager"; }

    Scene* GetCurrentScene() const;
    void CreateNewScene();
    bool LoadFromFile(const std::string& path);

    // Scene registry
    void RegisterScene(const std::string& name, Scene* scene);
    void UnregisterScene(const std::string& name);

    // Scene switching
    void SwitchScene(const std::string& name, float fadeMs = 500.0f);
    void SwitchSceneAsync(const std::string& name);

    // Query
    Scene* GetScene(const std::string& name) const;
    const std::string& GetCurrentSceneName() const { return m_currentSceneName; }
    bool IsTransitioning() const { return m_transitionPending; }

    // Transition data (for passing data between scenes)
    void SetTransitionData(const std::string& key, const std::string& value);
    std::string GetTransitionData(const std::string& key) const;
    bool HasTransitionData(const std::string& key) const;
    void ClearTransitionData();

    // Lifecycle events
    using SceneCallback = std::function<void(const std::string&)>;
    void SetOnSceneWillLoad(SceneCallback cb) { m_onSceneWillLoad = cb; }
    void SetOnSceneLoaded(SceneCallback cb) { m_onSceneLoaded = cb; }

private:
    std::shared_ptr<Scene> m_currentScene;
    std::unordered_map<std::string, Scene*> m_scenes;
    std::string m_currentSceneName;
    std::unordered_map<std::string, std::string> m_transitionData;
    SceneCallback m_onSceneWillLoad;
    SceneCallback m_onSceneLoaded;
    bool m_transitionPending = false;
    std::string m_pendingSceneName;
};
}  // namespace Prisma
