#pragma once

#include "app/Application.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include <memory>
#include <string>

namespace Prisma {

class Scene;

class Template3DApp : public Application {
public:
    Template3DApp();
    ~Template3DApp() override;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }
    void SetSamples(uint32_t samples) { m_ptMaxSamples = samples; }
    void SetHeadlessConfig(uint32_t totalFrames, const std::string& outputPath,
                           uint32_t width = 320, uint32_t height = 240) {
        m_headlessCfg.enabled = true;
        m_headlessCfg.totalFrames = totalFrames;
        m_headlessCfg.outputPath = outputPath;
        m_headlessCfg.width = width;
        m_headlessCfg.height = height;
    }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    void DrawStatsOverlay();

    void SavePathTracingOutput();


    bool m_autoQuit = false;

    // Path tracing pipeline
    std::shared_ptr<Graphic::PathTracingPipeline> m_ptPipeline;

    // 当前场景（相机托管于 Scene 中）
    Scene* m_scene = nullptr;

    struct HeadlessConfig {
        bool enabled = false;
        uint32_t totalFrames = 100;
        uint32_t width = 320;
        uint32_t height = 240;
        std::string outputPath = "output.png";
    } m_headlessCfg;

    // Overlay stat 缓存（替代 static 局部变量）
    std::string m_overlayTimingInfo = "Calculating...";
    std::string m_overlayStatusStr = "Loading...";
    std::string m_overlayResInfo;
    PrismaMath::vec4 m_overlayStatusColor = {0.2f, 1.0f, 0.2f, 1.0f};
    float m_overlayRefreshTimer = 0.0f;
    double m_overlayLastTime = 0.0;

    // SPS（自重置以来的平均 samples/sec）
    double m_accumStartTime = 0.0;
    uint32_t m_cachedSPS = 0;

    bool m_ptConverged = false;
    uint32_t m_ptMaxSamples = 512;
    bool m_enableNEE = false;
    bool m_usePrimitiveSphere = true; // P 键切换: true=PrimitiveComponent, false=MeshRenderer
};

} // namespace Prisma
