#pragma once
#include "ManagerBase.h"
#include "Scene.h"
#include <memory>

namespace PrismaEngine {

class SceneManager : public ManagerBase<SceneManager> {
    friend class ManagerBase<SceneManager>;

public:
    static constexpr std::string GetName() { return R"(SceneManager)"; }
    void Shutdown() override;
    void Update(float deltaTime) override;
    Scene* GetCurrentScene() const;
    bool Initialize() override;

private:
    std::shared_ptr<Scene> m_currentScene;
};
}  // namespace Engine
