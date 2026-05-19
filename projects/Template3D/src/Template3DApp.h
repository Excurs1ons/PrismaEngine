#pragma once

#include "app/Application.h"
#include "scripting/ScriptEngine.h"
#include "core/EntityManager.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/pipelines/pathtracing/PathTracingPipeline.h"
#include <memory>
#include <vector>
#include <functional>

namespace Prisma {
namespace Graphic {
    class OrthographicCamera;
}

class Template3DApp : public Application {
public:
    Template3DApp();
    ~Template3DApp() override;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }
    void SetSamples(uint32_t samples) { m_ptMaxSamples = samples; }
    void SetHeadlessConfig(uint32_t totalFrames, const std::string& outputPath,
                           uint32_t width = 80, uint32_t height = 60) {
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
    void OnShutdown() override;

private:
    enum class RenderMode {
        Forward3D,
        PathTracing
    };

    // Forward 3D
    void InitForwardResources();
    void RenderForward3D();

    // Path tracing via engine pipeline
    void RenderPathTracing();

    // Overlay callback
    void OnPresentOverlay(Graphic::ICommandBuffer* cmd);

    void SavePathTracingOutput();
    void DrawStatsOverlay();
    void InitGizmoResources();
    void ProcessGizmoOverlay(Graphic::ICommandBuffer* cmd);

    void OnWindowResize(uint32_t w, uint32_t h);
    void BuildPathTracingScene();

    bool m_autoQuit = false;
    RenderMode m_renderMode = RenderMode::PathTracing;

    // Forward 3D
    std::unique_ptr<Graphic::IBuffer> m_cornellBoxVB;
    std::unique_ptr<Graphic::IBuffer> m_cornellBoxIB;
    uint32_t m_cornellBoxIndexCount = 0;

    // Path tracing pipeline（引擎原生管线）
    std::shared_ptr<Graphic::PathTracingPipeline> m_ptPipeline;

    struct PTSceneObject {
        float p0[4];
        float p1[4];
        float p2[4];
        float color[4];
    };

    struct SceneDataSSBO {
        int objectCount = 0;
        float pad1 = 0, pad2 = 0, pad3 = 0;
        PTSceneObject objects[32]{};
    };

    // Camera control
    struct CameraControl {
        glm::vec3 position = glm::vec3(0.0f, 0.0f, 2.5f);
        glm::vec3 target   = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);
        float fov          = 70.0f;
        float yaw          = 0.0f;
        float pitch        = 0.0f;
    } m_camera;

    struct HeadlessConfig {
        bool enabled = false;
        uint32_t totalFrames = 100;
        uint32_t width = 80;
        uint32_t height = 60;
        std::string outputPath = "output.png";
    } m_headlessCfg;

    bool m_pathTracingDirty = true;
    bool m_ptConverged = false;
    uint32_t m_ptMaxSamples = 512;
    bool m_sceneLoaded = false;
    bool m_enableNEE = false;

    Graphic::IRenderDevice* m_device = nullptr;

    // Gizmo overlay
    std::shared_ptr<Graphic::IShader> m_gizmoVertShader;
    std::shared_ptr<Graphic::IShader> m_gizmoFragShader;
    std::shared_ptr<Graphic::IPipelineState> m_gizmoPSO;
    std::shared_ptr<Graphic::OrthographicCamera> m_gizmoCamera;
};

} // namespace Prisma
