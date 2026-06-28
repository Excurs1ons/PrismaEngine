#pragma once

#include "app/Application.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "app/ProjectConfig.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include "MLGISystem.h"
#include "ProbeGrid.h"
#include "SampleSource.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma {

class Scene;
class StatsOverlay;
class HeadlessRunner;
namespace Graphic { class RenderSystem; }

class MLGIPipelineApp : public Application {
public:
    MLGIPipelineApp();
    ~MLGIPipelineApp() override;

    int OnInitialize() override;
    void OnShutdown() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    // Hot-reload helper: load a scene file and rebuild PT data
    void LoadScene(const std::string& path);

    // ---- MLGI capability detection ----

    bool CheckRayQuerySupport();
    bool CheckTLASAvailability();
    bool ValidateProbeConfig();

    // Path tracing pipeline
    std::shared_ptr<Graphic::PathTracingPipeline> m_ptPipeline;

    // Current scene (camera hosted in Scene)
    Scene* m_scene = nullptr;

    // Delegated components
    std::unique_ptr<StatsOverlay> m_statsOverlay;
    std::unique_ptr<HeadlessRunner> m_headlessRunner;

    // MLGI probe GI system
    std::unique_ptr<MLGISystem> m_mlgiSystem;

    // Scene hot-reload state
    std::string m_scenePath;                // current scene path for F5 reload
    std::vector<std::string> m_sceneList;   // multi-scene list from project.jsonc
    int m_currentSceneIndex = -1;           // index in m_sceneList (F6/F7)

    MLGIConfig m_mlgiConfig;

    // Core state
    bool m_fallbackMode = false;
    bool m_mlgiEnabled = false;
    bool m_ptConverged = false;
    uint32_t m_ptMaxSamples = 512;
    bool m_enableNEE = false;
    bool m_usePrimitiveSphere = true;

    // ---- MLGI state ----

    bool m_rayQuerySupported = false;
    bool m_tlasAvailable = false;
    ProbeGrid m_probeGrid;

    // Render mode switching
    Graphic::RenderSystem* m_renderSystem = nullptr;
    RenderMode m_currentRenderMode = RenderMode::Mode3D_PathTracing;

    std::unique_ptr<SampleSource> m_sampleSource;
    SampleMode m_sampleMode = SampleMode::Halton;
    uint32_t m_sampleSeed = 42;
    uint32_t m_probeSampleCount = 1024;
};

} // namespace Prisma
