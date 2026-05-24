#pragma once

#include "app/Application.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma {

class Scene;
class StatsOverlay;
class HeadlessRunner;

class PathTracing3DApp : public Application {
public:
    PathTracing3DApp();
    ~PathTracing3DApp() override;

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    // Hot-reload helper: load a scene file and rebuild PT data
    void LoadScene(const std::string& path);

    // Path tracing pipeline
    std::shared_ptr<Graphic::PathTracingPipeline> m_ptPipeline;

    // Current scene (camera hosted in Scene)
    Scene* m_scene = nullptr;

    // Delegated components
    std::unique_ptr<StatsOverlay> m_statsOverlay;
    std::unique_ptr<HeadlessRunner> m_headlessRunner;

    // Scene hot-reload state
    std::string m_scenePath;                // current scene path for F5 reload
    std::vector<std::string> m_sceneList;   // multi-scene list from project.jsonc
    int m_currentSceneIndex = -1;           // index in m_sceneList (F6/F7)

    // Core state
    bool m_ptConverged = false;
    uint32_t m_ptMaxSamples = 512;
    bool m_enableNEE = false;
    bool m_usePrimitiveSphere = true;
};

} // namespace Prisma
