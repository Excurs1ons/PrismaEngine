#pragma once

#include "ISubSystem.h"
#include "Export.h"
#include "core/Timestep.h"
#include "water/WaterConfig.h"
#include "water/WaterMesh.h"
#include "water/WaveSimulation.h"
#include "water/WaterInteraction.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/ISampler.h"
#include "math/MathTypes.h"
#include <memory>
#include <cstdint>
#include <functional>

namespace Prisma::Water {

/**
 * @brief WaterSystem - manages water simulation and rendering
 * 
 * Subsystem lifecycle:
 * - Initialize(): create mesh, waves, shaders, reflection/refraction framebuffers
 * - Update(dt): advance wave time, update interactions, reflection/refraction captures,
 *               submit draw calls
 * - Shutdown(): cleanup all GPU resources
 */
class ENGINE_API WaterSystem : public ISubSystem {
public:
    WaterSystem();
    ~WaterSystem() override;

    // ========== ISubSystem Interface ==========
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "WaterSystem"; }

    // ========== Configuration ==========

    /**
     * @brief Get water configuration
     */
    WaterConfig& GetConfig() { return m_Config; }
    const WaterConfig& GetConfig() const { return m_Config; }

    /**
     * @brief Set water configuration
     */
    void SetConfig(const WaterConfig& config) { m_Config = config; }

    // ========== Wave Access ==========

    /**
     * @brief Get wave simulation
     */
    WaveSimulation& GetWaveSimulation() { return m_WaveSimulation; }
    const WaveSimulation& GetWaveSimulation() const { return m_WaveSimulation; }

    // ========== Interaction ==========

    /**
     * @brief Get water interaction system
     */
    WaterInteraction& GetInteraction() { return m_Interaction; }
    const WaterInteraction& GetInteraction() const { return m_Interaction; }

    /**
     * @brief Spawn a ripple at world position
     * @param position  impact point
     * @param strength  impact strength
     */
    void SpawnRipple(const Vector3& position, float strength = 1.0f);

    // ========== State ==========

    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }

    bool IsInitialized() const { return m_Initialized; }

    /**
     * @brief Get water level height
     */
    float GetWaterLevel() const { return m_Config.render.waterLevel; }
    void SetWaterLevel(float level) { m_Config.render.waterLevel = level; }

    /**
     * @brief Evaluate wave height at world position (with ripples)
     */
    float GetWaveHeight(const Vector2& worldPos) const;

    /**
     * @brief Evaluate full wave displacement at world position (includes ripples)
     */
    Vector3 GetWaveDisplacement(const Vector2& worldPos) const;

private:
    // ========== Internal Methods ==========

    void InitMesh();
    void InitWaves();
    void CreateFrameBuffers();
    void CaptureReflection();
    void CaptureRefraction();
    void SubmitDrawCalls();

    // GPU resource creation helpers
    void CreateWaterResources();
    void CreateReflectionTextures();
    void CreateRefractionTextures();
    void SetupDescriptorSet();
    void UploadWaveData();

    // Mirror camera for reflection
    void BuildReflectionCamera(Prisma::Matrix4x4& outView, Prisma::Matrix4x4& outProj,
                               Prisma::Vector3& outPos) const;
    void BuildRefractionCamera(Prisma::Matrix4x4& outView, Prisma::Matrix4x4& outProj,
                               Prisma::Vector3& outPos) const;

    // Water render callback (called by RenderSystem during EndFrame)
    void RenderWater(Graphic::ICommandBuffer* cmd, Graphic::IRenderDevice* device);
    bool EnsureWaterPipeline(Graphic::IRenderDevice* device);
    bool EnsureWaterDescriptorSet(Graphic::IRenderDevice* device);
    bool EnsureWaterMeshBuffers(Graphic::IRenderDevice* device);

    // ========== Member Variables ==========

    // Config
    WaterConfig m_Config;

    // Core components
    std::unique_ptr<WaterMesh> m_Mesh;
    WaveSimulation m_WaveSimulation;
    WaterInteraction m_Interaction;

    // State
    bool m_Enabled = true;
    bool m_Initialized = false;

    // Reflection/refraction resolution
    uint32_t m_ReflectionResolution = 512;
    uint32_t m_RefractionResolution = 512;

    // Camera state (cached each frame)
    Vector3 m_CameraPosition = Vector3(0.0f);
    Vector3 m_CameraForward = Vector3(0.0f, 0.0f, -1.0f);
    Vector3 m_CameraUp = Vector3(0.0f, 1.0f, 0.0f);
    float m_CameraFOV = 70.0f;
    float m_CameraAspect = 16.0f / 9.0f;

    // Reflection/refraction GPU textures
    std::shared_ptr<Graphic::ITexture> m_ReflectionColorTex;
    std::shared_ptr<Graphic::ITexture> m_ReflectionDepthTex;
    std::shared_ptr<Graphic::ITexture> m_RefractionColorTex;
    std::shared_ptr<Graphic::ITexture> m_RefractionDepthTex;

    // Cubemap for reflection (required by water.frag samplerCube at binding 3)
    // All 6 faces filled from m_ReflectionColorTex
    std::shared_ptr<Graphic::ITexture> m_ReflectionCubeTex;

    // Water pipeline resources
    std::shared_ptr<Graphic::IShader> m_WaterVertShader;
    std::shared_ptr<Graphic::IShader> m_WaterFragShader;
    std::shared_ptr<Graphic::IPipelineState> m_WaterPipeline;

    // Water mesh GPU buffers
    std::shared_ptr<Graphic::IBuffer> m_VertexBuffer;
    std::shared_ptr<Graphic::IBuffer> m_IndexBuffer;

    // Uniform buffer for camera data (binding 0)
    struct alignas(16) CameraUniforms {
        Prisma::Matrix4x4 viewProjection;
        Prisma::Matrix4x4 view;
        Prisma::Matrix4x4 projection;
        Prisma::Vector4 cameraPos;      // xyz = position, w = near
        Prisma::Vector4 cameraForward;  // xyz = forward, w = far
    };
    std::shared_ptr<Graphic::IBuffer> m_CameraUBO;

    // Uniform buffer for water params (binding 1)
    struct alignas(16) WaterUniforms {
        Prisma::Vector4 deepColor;       // rgb = deep color, a = unused
        Prisma::Vector4 shallowColor;    // rgb = shallow color, a = unused
        Prisma::Vector4 fogColor;        // rgb = fog color, a = density
        float waterLevel;
        float waveHeightScale;
        float fresnelPower;
        float fresnelStrength;
        float specularStrength;
        float specularPower;
        float transparency;
        float refractionScale;
        float sunGlowStrength;
        float sunGlowPower;
        float time;
        float fogDensity;
        float shallowDepth;
        float deepDepth;
        float _padding[2]; // pad to vec4
    };
    std::shared_ptr<Graphic::IBuffer> m_WaterUBO;

    // Wave storage buffer (SSBO, binding 2)
    // Gerstner wave data packed into vec4 arrays matching shader layout
    struct alignas(16) WaveDataElement {
        Prisma::Vector4 waveData;  // x = dirX, y = dirZ, z = amplitude, w = frequency
        Prisma::Vector4 wavePhase; // x = speed, y = steepness, z unused, w unused
    };
    std::shared_ptr<Graphic::IBuffer> m_WaveSSBO;

    // Descriptor set for water pipeline (all bindings in set 0)
    std::shared_ptr<Graphic::IDescriptorSetLayout> m_DescriptorSetLayout;
    std::shared_ptr<Graphic::IDescriptorSet> m_DescriptorSet;

    // Samplers
    std::shared_ptr<Graphic::ISampler> m_LinearSampler;
    std::shared_ptr<Graphic::ISampler> m_ReflectionSampler;

    // Mesh data version tracking (for GPU buffer updates)
    size_t m_LastVertexCount = 0;
    size_t m_LastIndexCount = 0;

    // Reflection/refraction camera data (computed in Update, used in RenderWater)
    Prisma::Matrix4x4 m_ReflectionView;
    Prisma::Matrix4x4 m_ReflectionProj;
    Prisma::Vector3 m_ReflectionPos;
    bool m_ReflectionValid = false;

    Prisma::Matrix4x4 m_RefractionView;
    Prisma::Matrix4x4 m_RefractionProj;
    Prisma::Vector3 m_RefractionPos;
    bool m_RefractionValid = false;

    // Has the water rendering callback been registered with the render system
    bool m_CallbackRegistered = false;

    // Scene depth texture reference (for water.frag depth-based effects)
    std::shared_ptr<Graphic::ITexture> m_SceneDepthTexture;
};

} // namespace Prisma::Water
