#pragma once

#include "app/Application.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma {

class Scene;
class StatsOverlay;

namespace Graphic {
    class ClusteredForwardPipeline;
}

class ClusteredApp : public Application {
public:
    ClusteredApp();
    ~ClusteredApp() override;

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    void LoadScene(const std::string& path);

    std::shared_ptr<Graphic::ClusteredForwardPipeline> m_pipeline;
    Scene* m_scene = nullptr;
    std::unique_ptr<StatsOverlay> m_statsOverlay;

    std::string m_scenePath;
    std::vector<std::string> m_sceneList;
    int m_currentSceneIndex = -1;
};

} // namespace Prisma
