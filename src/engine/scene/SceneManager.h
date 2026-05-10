#pragma once
#include "../core/ISubSystem.h"
#include "Scene.h"
#include <memory>

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

private:
    std::shared_ptr<Scene> m_currentScene;
};
}  // namespace Prisma
