#pragma once

#include "app/Application.h"
#include "app/ProjectConfig.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ITexture.h"
#include "core/Timestep.h"
#include "ProbeGrid.h"
#include "ProbeSHBuffer.h"
#include "MLGITemporalPass.h"
#include "MLGIProbeUpdatePass.h"
#include "MLGIScreenGatherPass.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace Prisma {

namespace Graphic { class RenderSystem; }
namespace Graphic { class ICommandBuffer; }
namespace Graphic { class IBuffer; }
namespace Graphic { class ITexture; }

/// Reasons for temporal accumulation reset, logged for debugging.
enum class MLGIResetReason {
    Manual       = 0,
    SceneReload  = 1,
    Resize       = 2,
    ConfigChange = 3
};

/// Human-readable name for each reset reason.
inline const char* MLGIResetReasonName(MLGIResetReason reason) {
    switch (reason) {
        case MLGIResetReason::Manual:       return "Manual";
        case MLGIResetReason::SceneReload:  return "SceneReload";
        case MLGIResetReason::Resize:       return "Resize";
        case MLGIResetReason::ConfigChange: return "ConfigChange";
        default:                            return "Unknown";
    }
}

class MLGISystem {
public:
    MLGISystem();
    ~MLGISystem();

    void Initialize(const MLGIConfig& config, Graphic::RenderSystem* renderSystem);
    void Update(Timestep ts);
    void Shutdown();

    bool IsEnabled()  const { return m_enabled; }
    bool IsReady()    const { return m_initialized && m_enabled; }
    const MLGIConfig& GetConfig()    const { return m_config; }
    const ProbeGrid&  GetProbeGrid() const { return m_probeGrid; }
    void DebugDumpSH() const;

    // ── Resize / Reload / Config-Change Handlers ──

    /// Called when the window or render target is resized.
    /// Recreates screen-sized resources (screen gather pass) and resets
    /// temporal accumulation to avoid history with stale screen dimensions.
    void OnResize(uint32_t width, uint32_t height);

    /// Called when the scene is reloaded or switched.
    /// Resets temporal accumulation so old probe history does not bleed
    /// into the new scene's lighting.
    void OnSceneReload();

    /// Apply a new MLGI config at runtime. Detects changes to probe grid
    /// dimensions/spacing or blend factor and reinitializes / resets
    /// temporal accumulation as needed.
    void ApplyConfig(const MLGIConfig& newConfig);

    // GPU temporal accumulation
    void ExecuteTemporal(Graphic::ICommandBuffer* cmd,
                         Graphic::IBuffer* currentSH,
                         Graphic::IBuffer* historySH,
                         Graphic::IBuffer* blendedSH);
    void ResetTemporal(MLGIResetReason reason = MLGIResetReason::Manual);

    // ── Full MLGI pipeline dispatch ──
    // Sequences: ProbeUpdate → Temporal → ScreenGather with barriers.
    // Call once per frame from the app's render loop.
    void ExecuteMLGIPipeline(Graphic::ICommandBuffer* cmd,
                             void* tlasHandle,
                             Graphic::IBuffer* sceneSSBO,
                             Graphic::IBuffer* probeSamplesBuffer,
                             Graphic::ITexture* depthTexture,
                             Graphic::ITexture* normalTexture,
                             Graphic::ITexture* giOutputTexture,
                             float giStrength,
                             bool useNormalMap);

    SHProbeBuffers&       GetSHBuffers()       { return m_shBuffers; }
    const SHProbeBuffers& GetSHBuffers() const { return m_shBuffers; }

private:
    void AllocateProbeBuffers();
    void ReleaseProbeBuffers();

    MLGIConfig               m_config;
    ProbeGrid                m_probeGrid;
    Graphic::RenderSystem*   m_renderSystem = nullptr;
    bool                     m_enabled      = false;
    bool                     m_initialized  = false;

    // Screen dimensions (for resize tracking)
    uint32_t                 m_screenWidth  = 0;
    uint32_t                 m_screenHeight = 0;

    std::unique_ptr<MLGIProbeUpdatePass>   m_probeUpdatePass;
    std::unique_ptr<MLGITemporalPass>      m_temporalPass;
    std::unique_ptr<MLGIScreenGatherPass>  m_screenGatherPass;

    SHProbeBuffers m_shBuffers;

    struct CPUProbeSHBuffer {
        uint32_t probeCount  = 0;
        uint32_t coeffCount  = 0;
        std::vector<float> currentData;
        std::vector<float> historyData;
        std::vector<float> blendedData;
    };
    CPUProbeSHBuffer m_cpuShBuffer;

public:
    struct Metrics {
        bool     enabled            = false;
        bool     rayQueryAvailable  = false;
        uint32_t probeCount         = 0;
        uint32_t raysPerProbe       = 0;
        uint32_t temporalFrameCount = 0;
        uint32_t nonZeroCoeffCount  = 0;
        uint32_t totalCoeffCount    = 0;
        std::string fallbackReason;
    };

    Metrics GetMetrics() const;
};

} // namespace Prisma
