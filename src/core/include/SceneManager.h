#pragma once
#include "Scene.h"
#include "ManagerBase.h"
#include <memory>

namespace Engine {

class SceneManager : public ManagerBase<SceneManager> {
    friend class ManagerBase<SceneManager>;

public:
    static constexpr std::string GetName() { return R"(SceneManager)"; }
    void Shutdown() override;
    void Update(float deltaTime) override;
    Scene* GetCurrentScene();
    bool Initialize() override;

private:
    std::shared_ptr<Scene> m_currentScene;
};
}  // namespace Engine
