#pragma once
#include "ManagerBase.h"
#include "Scene.h"
#include <memory>

namespace Prisma {

class ENGINE_API SceneManager : public ManagerBase<SceneManager> {
public:
    static std::shared_ptr<SceneManager> Get();

    static constexpr const char* GetStaticName() { return "SceneManager"; }
    void Shutdown() override;
    void Update(Timestep ts) override;
    Scene* GetCurrentScene() const;
    int Initialize() override;
    SceneManager() = default;
    ~SceneManager() override = default;

private:
    std::shared_ptr<Scene> m_currentScene;
};
}  // namespace Prisma
