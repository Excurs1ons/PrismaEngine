#pragma once

#include "app/Application.h"
#include "graphic/interfaces/IPipeline.h"
#include <memory>
#include <string>

namespace Prisma {

class Scene;
class StatsOverlay;
class DeferredPipelineAdapter;

class Deferred3DApp : public Application {
public:
    Deferred3DApp();
    ~Deferred3DApp() override;

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    void LoadScene(const std::string& path);

    std::shared_ptr<DeferredPipelineAdapter> m_deferredAdapter;

    Scene* m_scene = nullptr;
    std::unique_ptr<StatsOverlay> m_statsOverlay;

    std::string m_scenePath;
    std::vector<std::string> m_sceneList;
    int m_currentSceneIndex = -1;

    bool m_showGBuffer = false;
    int m_gBufferTarget = 0;
};

} // namespace Prisma
