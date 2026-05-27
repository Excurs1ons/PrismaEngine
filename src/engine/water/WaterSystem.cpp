#include "water/WaterSystem.h"
#include "Logger.h"
#include "Engine.h"
#include "SceneManager.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/IResourceManager.h"
#include "transform/Camera.h"
#include <algorithm>
#include <cmath>

namespace Prisma::Water {

WaterSystem::WaterSystem()
{
    m_Mesh = std::make_unique<WaterMesh>();
}

WaterSystem::~WaterSystem()
{
    Shutdown();
}

int WaterSystem::Initialize()
{
    if (m_Initialized) return 0;

    LOG_INFO("Water", "Water system initializing...");

    InitMesh();
    InitWaves();
    CreateFrameBuffers();

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (renderSystem && renderSystem->GetDevice()) {
        LOG_INFO("Water", "Water shaders will be loaded at first render frame");
    }

    m_Initialized = true;
    LOG_INFO("Water", "Water system initialized");
    return 0;
}

void WaterSystem::Shutdown()
{
    if (!m_Initialized) return;

    LOG_INFO("Water", "Water system shutting down...");

    m_Mesh->Clear();
    m_Interaction.Clear();

    m_ReflectionFramebuffer = 0;
    m_RefractionFramebuffer = 0;
    m_WaterShader = 0;

    m_Initialized = false;
    LOG_INFO("Water", "Water system shut down");
}

void WaterSystem::Update(Timestep ts)
{
    if (!m_Initialized || !m_Enabled) return;

    float dt = ts.GetSeconds();
    if (dt <= 0.0f || dt > 0.1f) dt = 0.1f;

    auto& engine = Engine::Get();
    auto* sceneManager = engine.GetSceneManager();
    if (sceneManager) {
        auto* scene = sceneManager->GetCurrentScene();
        if (scene) {
            auto camera = scene->GetMainCamera();
            if (camera) {
                m_CameraPosition = camera->GetPosition();
                m_CameraForward = camera->GetForward();
                m_CameraUp = camera->GetUp();
                m_CameraFOV = camera->GetFOV();
                m_CameraAspect = camera->GetAspectRatio();
            }
        }
    }

    m_WaveSimulation.AdvanceTime(dt);

    if (m_Config.enableInteraction) {
        m_Interaction.Update(dt, m_Config.rippleSpeed, m_Config.rippleDecay);
    }

    m_Mesh->Generate(
        m_CameraPosition,
        m_Config.render.tileSize,
        m_Config.render.baseSegments,
        m_Config.render.farSegments,
        m_Config.render.lodDistance
    );

    CaptureReflection();
    CaptureRefraction();
    SubmitDrawCalls();
}

void WaterSystem::SpawnRipple(const Vector3& position, float strength)
{
    if (!m_Config.enableInteraction) return;
    m_Interaction.SpawnRipple(position, strength);
}

float WaterSystem::GetWaveHeight(const Vector2& worldPos) const
{
    WaveDisplacement displacement = m_WaveSimulation.Evaluate(worldPos);
    float height = displacement.position.y;

    if (m_Config.enableInteraction) {
        height += m_Interaction.GetDisplacement(worldPos);
    }

    return m_Config.render.waterLevel + height * m_Config.render.waveHeightScale;
}

Vector3 WaterSystem::GetWaveDisplacement(const Vector2& worldPos) const
{
    WaveDisplacement displacement = m_WaveSimulation.Evaluate(worldPos);

    Vector3 result;
    result.x = displacement.position.x;
    result.y = m_Config.render.waterLevel + displacement.position.y * m_Config.render.waveHeightScale;
    result.z = displacement.position.z;

    if (m_Config.enableInteraction) {
        result.y += m_Interaction.GetDisplacement(worldPos);
    }

    return result;
}

// ============================================================================
// Internal Methods
// ============================================================================

void WaterSystem::InitMesh()
{
    m_Mesh->Generate(
        Vector3(0.0f),
        m_Config.render.tileSize,
        m_Config.render.baseSegments,
        m_Config.render.farSegments,
        m_Config.render.lodDistance
    );
    LOG_INFO("Water", "Water mesh created: {} vertices, {} triangles",
             m_Mesh->GetVertexCount(), m_Mesh->GetTriangleCount());
}

void WaterSystem::InitWaves()
{
    m_WaveSimulation.SetGerstnerWaves(m_Config.gerstnerWaves);
    m_WaveSimulation.SetUseFFT(m_Config.useFFT);
    if (m_Config.useFFT) {
        m_WaveSimulation.SetFFTConfig(m_Config.fft);
        LOG_INFO("Water", "FFT wave simulation enabled (resolution: {})",
                 m_Config.fft.resolution);
    }
    LOG_INFO("Water", "Wave simulation initialized with {} Gerstner waves",
             m_WaveSimulation.GetWaveCount());
}

void WaterSystem::CreateFrameBuffers()
{
    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* device = renderSystem->GetDevice();
    if (!device) return;

    // Reflection framebuffer creation would be handled by the render system
    // Here we record the desired resolution for later use
    m_ReflectionResolution = 512;
    m_RefractionResolution = static_cast<uint32_t>(
        static_cast<float>(m_ReflectionResolution) / m_CameraAspect);

    LOG_INFO("Water", "Reflection/refraction framebuffers: {}x{}, {}x{}",
             m_ReflectionResolution, m_ReflectionResolution,
             m_RefractionResolution, m_RefractionResolution);
}

void WaterSystem::CaptureReflection()
{
    // Placeholder: reflection capture would:
    // 1. Mirror camera across water plane (Y = waterLevel)
    // 2. Render scene to reflection framebuffer
    // 3. Store for use in water fragment shader
    //
    // Implementation uses render system reflection pass when available.
}

void WaterSystem::CaptureRefraction()
{
    // Placeholder: refraction capture would:
    // 1. Render scene below water level to refraction framebuffer
    // 2. Store for use in water fragment shader
    //
    // The refraction target is typically the scene color buffer
    // sampled at distorted UV coordinates.
}

void WaterSystem::SubmitDrawCalls()
{
    if (!m_Mesh || !m_Mesh->IsValid()) return;

    auto& engine = Engine::Get();
    auto* renderSystem = engine.GetRenderSystem();
    if (!renderSystem) return;

    auto* pipeline = renderSystem->GetMainPipeline();
    if (!pipeline) return;

    // Water rendering would be submitted as a transparent render pass:
    // 1. Bind water shader
    // 2. Set uniforms: view-projection matrix, camera pos, wave data, time
    // 3. Bind reflection/refraction textures
    // 4. Bind water vertex + index buffers
    // 5. Issue draw call
    //
    // The actual GPU rendering is handled by the render pipeline's
    // transparent pass, which sorts by distance and renders last.
}

} // namespace Prisma::Water
