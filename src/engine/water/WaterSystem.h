#pragma once

#include "ISubSystem.h"
#include "Export.h"
#include "core/Timestep.h"
#include "water/WaterConfig.h"
#include "water/WaterMesh.h"
#include "water/WaveSimulation.h"
#include "water/WaterInteraction.h"
#include "math/MathTypes.h"
#include <memory>
#include <cstdint>

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

    /**
     * @brief Initialize water mesh
     */
    void InitMesh();

    /**
     * @brief Initialize wave simulation
     */
    void InitWaves();

    /**
     * @brief Create reflection/refraction framebuffers
     */
    void CreateFrameBuffers();

    /**
     * @brief Capture reflection cubemap or reflection render
     */
    void CaptureReflection();

    /**
     * @brief Capture refraction scene render
     */
    void CaptureRefraction();

    /**
     * @brief Submit water draw calls to render pipeline
     */
    void SubmitDrawCalls();

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

    // Reflection/refraction framebuffer handles (populated by render system)
    uint64_t m_ReflectionFramebuffer = 0;
    uint64_t m_RefractionFramebuffer = 0;
    uint32_t m_ReflectionResolution = 512;
    uint32_t m_RefractionResolution = 512;

    // Shader handles
    uint64_t m_WaterShader = 0;

    // Camera state (cached each frame)
    Vector3 m_CameraPosition = Vector3(0.0f);
    Vector3 m_CameraForward = Vector3(0.0f, 0.0f, -1.0f);
    Vector3 m_CameraUp = Vector3(0.0f, 1.0f, 0.0f);
    float m_CameraFOV = 70.0f;
    float m_CameraAspect = 16.0f / 9.0f;
};

} // namespace Prisma::Water
