#include "MLGISystem.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "Logger.h"

#include <cmath>

namespace Prisma {

MLGISystem::MLGISystem() = default;
MLGISystem::~MLGISystem() { Shutdown(); }

void MLGISystem::Initialize(const MLGIConfig& config, Graphic::RenderSystem* renderSystem) {
    m_config = config;
    m_renderSystem = renderSystem;

    if (!m_config.enableMLGI) {
        LOG_INFO("MLGISystem", "MLGI disabled by config, skipping allocation");
        m_enabled = false;
        m_initialized = true;
        return;
    }

    if (m_renderSystem) {
        auto* device = m_renderSystem->GetDevice();
        if (device && !device->IsRayQuerySupported()) {
            LOG_WARN("MLGISystem", "RayQuery unsupported by device, MLGI disabled");
            m_enabled = false;
            m_initialized = true;
            return;
        }
    }

    const glm::uvec3 gridDim(
        m_config.probeGridDim.x,
        m_config.probeGridDim.y,
        m_config.probeGridDim.z
    );

    m_probeGrid = ProbeGrid(
        gridDim,
        glm::vec3(0.0f),
        glm::vec3(m_config.probeSpacing)
    );
    m_probeGrid.LogConfig();

    AllocateProbeBuffers();

    if (m_renderSystem) {
        auto* device = m_renderSystem->GetDevice();
        if (device) {
            m_probeUpdatePass = std::make_unique<MLGIProbeUpdatePass>();
            if (!m_probeUpdatePass->Setup(device, m_probeGrid, m_config.raysPerProbe)) {
                LOG_WARN("MLGISystem", "Probe update pass setup failed, MLGI pipeline incomplete");
                m_probeUpdatePass.reset();
            }

            m_temporalPass = std::make_unique<MLGITemporalPass>();
            if (!m_temporalPass->Setup(device)) {
                LOG_WARN("MLGISystem", "Temporal pass setup failed, falling back to CPU blending");
                m_temporalPass.reset();
            }

            // Use app spec width/height if available, else default 1920x1080
            m_screenWidth  = 1920;
            m_screenHeight = 1080;
            m_screenGatherPass = std::make_unique<MLGIScreenGatherPass>();
            if (!m_screenGatherPass->Initialize(device, m_probeGrid.probeCount, m_screenWidth, m_screenHeight)) {
                LOG_WARN("MLGISystem", "Screen gather pass setup failed, MLGI pipeline incomplete");
                m_screenGatherPass.reset();
            } else {
                m_screenGatherPass->SetProbeGrid(m_probeGrid);
            }
        }
    }

    m_enabled = true;
    m_initialized = true;

    LOG_INFO("MLGISystem", "MLGI initialized: grid {}x{}x{}, spacing=({:.1f},{:.1f},{:.1f}), probes={}, rays/probe={}",
             m_probeGrid.dimensions.x, m_probeGrid.dimensions.y, m_probeGrid.dimensions.z,
             m_probeGrid.spacing.x, m_probeGrid.spacing.y, m_probeGrid.spacing.z,
             m_probeGrid.probeCount, m_config.raysPerProbe);
}

void MLGISystem::Update(Timestep ts) {
    if (!IsReady()) {
        return;
    }

    m_probeGrid.frameIndex++;
    m_shBuffers.temporalFrameCount++;

    if (!m_temporalPass && m_cpuShBuffer.probeCount > 0) {
        float blend = m_config.temporalBlendFactor;
        for (uint32_t i = 0; i < m_cpuShBuffer.coeffCount; ++i) {
            m_cpuShBuffer.blendedData[i] = blend * m_cpuShBuffer.historyData[i]
                                      + (1.0f - blend) * m_cpuShBuffer.currentData[i];
        }
        m_cpuShBuffer.historyData.swap(m_cpuShBuffer.blendedData);
    }

    (void)ts;
}

void MLGISystem::ExecuteTemporal(Graphic::ICommandBuffer* cmd,
                                  Graphic::IBuffer* currentSH,
                                  Graphic::IBuffer* historySH,
                                  Graphic::IBuffer* blendedSH) {
    if (!IsReady() || !cmd || !m_temporalPass) {
        return;
    }

    uint32_t coeffCount = m_cpuShBuffer.coeffCount;
    if (coeffCount == 0) {
        return;
    }

    uint32_t temporalFrame = m_probeGrid.frameIndex;

    m_temporalPass->Execute(cmd, currentSH, historySH, blendedSH,
                            coeffCount, m_config.temporalBlendFactor,
                            temporalFrame, false);

    m_temporalPass->CopyBack(cmd, blendedSH, historySH, coeffCount);
}

void MLGISystem::ResetTemporal(MLGIResetReason reason) {
    m_probeGrid.frameIndex = 0;
    m_shBuffers.temporalFrameCount = 0;

    if (m_cpuShBuffer.probeCount > 0) {
        std::fill(m_cpuShBuffer.historyData.begin(), m_cpuShBuffer.historyData.end(), 0.0f);
        std::fill(m_cpuShBuffer.blendedData.begin(), m_cpuShBuffer.blendedData.end(), 0.0f);
    }

    LOG_INFO("MLGISystem", "Temporal accumulation reset: reason={}, frameIndex=0, history cleared",
             MLGIResetReasonName(reason));
}

void MLGISystem::OnResize(uint32_t width, uint32_t height) {
    if (!IsReady()) {
        return;
    }

    if (width == m_screenWidth && height == m_screenHeight) {
        return;
    }

    LOG_INFO("MLGISystem", "OnResize: {}x{} -> {}x{}", m_screenWidth, m_screenHeight, width, height);

    m_screenWidth  = width;
    m_screenHeight = height;

    if (m_screenGatherPass) {
        m_screenGatherPass->Resize(width, height);
    }

    ResetTemporal(MLGIResetReason::Resize);
}

void MLGISystem::OnSceneReload() {
    if (!IsReady()) {
        return;
    }

    LOG_INFO("MLGISystem", "OnSceneReload: resetting temporal accumulation for new scene");
    ResetTemporal(MLGIResetReason::SceneReload);
}

void MLGISystem::ApplyConfig(const MLGIConfig& newConfig) {
    if (!m_initialized) {
        LOG_WARN("MLGISystem", "ApplyConfig: not initialized, ignoring");
        return;
    }

    const bool gridDimChanged = (newConfig.probeGridDim.x != m_config.probeGridDim.x)
                             || (newConfig.probeGridDim.y != m_config.probeGridDim.y)
                             || (newConfig.probeGridDim.z != m_config.probeGridDim.z);
    bool gridChanged  = gridDimChanged
                     || (newConfig.probeSpacing != m_config.probeSpacing);
    bool blendChanged = (newConfig.temporalBlendFactor != m_config.temporalBlendFactor);
    bool raysChanged  = (newConfig.raysPerProbe != m_config.raysPerProbe);
    bool enabledChanged = (newConfig.enableMLGI != m_config.enableMLGI);

    if (enabledChanged) {
        LOG_INFO("MLGISystem", "ApplyConfig: enableMLGI changed ({} -> {}), reinitializing",
                 m_config.enableMLGI, newConfig.enableMLGI);
        Shutdown();
        Initialize(newConfig, m_renderSystem);
        return;
    }

    if (gridChanged || raysChanged) {
        LOG_INFO("MLGISystem", "ApplyConfig: probe grid or rays changed, reinitializing");
        Shutdown();
        Initialize(newConfig, m_renderSystem);
        return;
    }

    if (blendChanged) {
        LOG_INFO("MLGISystem", "ApplyConfig: blend factor {:.2f} -> {:.2f}, resetting temporal",
                 m_config.temporalBlendFactor, newConfig.temporalBlendFactor);
        m_config.temporalBlendFactor = newConfig.temporalBlendFactor;
        ResetTemporal(MLGIResetReason::ConfigChange);
        return;
    }

    LOG_INFO("MLGISystem", "ApplyConfig: no meaningful change detected, keeping current state");
}

void MLGISystem::ExecuteMLGIPipeline(Graphic::ICommandBuffer* cmd,
                                      void* tlasHandle,
                                      Graphic::IBuffer* sceneSSBO,
                                      Graphic::IBuffer* probeSamplesBuffer,
                                      Graphic::ITexture* depthTexture,
                                      Graphic::ITexture* normalTexture,
                                      Graphic::ITexture* giOutputTexture,
                                      float giStrength,
                                      bool useNormalMap) {
    if (!IsReady() || !cmd) {
        return;
    }

    bool probeUpdateReady = m_probeUpdatePass && m_probeUpdatePass->IsReady();
    bool temporalReady    = m_temporalPass && m_temporalPass->IsReady();
    bool gatherReady      = m_screenGatherPass && m_screenGatherPass->IsInitialized();

    if (!probeUpdateReady && !temporalReady && !gatherReady) {
        return;
    }

    LOG_INFO("MLGISystem", "=== MLGI Dispatch Timeline ===");
    LOG_INFO("MLGISystem", "  Frame {} | Probes: {} | Rays/probe: {}",
             m_probeGrid.frameIndex, m_probeGrid.probeCount, m_config.raysPerProbe);

    // ─────────────────────────────────────────────────────────
    // Stage 1: ProbeUpdate (writes currentSH via RayQuery)
    // ─────────────────────────────────────────────────────────
    if (probeUpdateReady && tlasHandle && sceneSSBO && probeSamplesBuffer && m_shBuffers.currentProbeSH) {
        LOG_INFO("MLGISystem", "  [1/3] ProbeUpdate → dispatch {} workgroups (RayQuery)",
                 m_probeGrid.probeCount);
        cmd->BeginDebugGroup("MLGI_ProbeUpdate");
        m_probeUpdatePass->Execute(cmd, probeSamplesBuffer, tlasHandle, sceneSSBO,
                                   m_shBuffers.currentProbeSH.get());
        cmd->EndDebugGroup();
    } else {
        LOG_WARN("MLGISystem", "  [1/3] ProbeUpdate — SKIPPED (resources not ready)");
    }

    // Barrier: ProbeUpdate write (currentSH, UnorderedAccess) → Temporal read (ShaderRead)
    {
        LOG_INFO("MLGISystem", "  [BARRIER] currentSH: UnorderedAccess → ShaderRead");
        cmd->PipelineBarrier();
    }

    // ─────────────────────────────────────────────────────────
    // Stage 2: Temporal (reads currentSH, writes blendedSH)
    // ─────────────────────────────────────────────────────────
    if (temporalReady && m_shBuffers.currentProbeSH && m_shBuffers.historyProbeSH && m_shBuffers.blendedProbeSH) {
        LOG_INFO("MLGISystem", "  [2/3] Temporal → blend (factor={:.2f}, frame={})",
                 m_config.temporalBlendFactor, m_probeGrid.frameIndex);
        cmd->BeginDebugGroup("MLGI_Temporal");
        ExecuteTemporal(cmd,
                        m_shBuffers.currentProbeSH.get(),
                        m_shBuffers.historyProbeSH.get(),
                        m_shBuffers.blendedProbeSH.get());
        cmd->EndDebugGroup();
    } else {
        LOG_WARN("MLGISystem", "  [2/3] Temporal — SKIPPED (pass or buffers not ready)");
    }

    // Barrier: Temporal write (blendedSH, UnorderedAccess) → ScreenGather read (ShaderRead)
    {
        LOG_INFO("MLGISystem", "  [BARRIER] blendedSH: UnorderedAccess → ShaderRead");
        cmd->PipelineBarrier();
    }

    // ─────────────────────────────────────────────────────────
    // Stage 3: ScreenGather (reads blendedSH, writes giOutput)
    // ─────────────────────────────────────────────────────────
    if (gatherReady && m_shBuffers.blendedProbeSH && depthTexture && giOutputTexture) {
        LOG_INFO("MLGISystem", "  [3/3] ScreenGather → dispatch {}x{} (GI gather)",
                 (m_screenWidth + 7) / 8, (m_screenHeight + 7) / 8);
        cmd->BeginDebugGroup("MLGI_ScreenGather");
        m_screenGatherPass->Dispatch(cmd,
                                     m_shBuffers.blendedProbeSH.get(),
                                     depthTexture,
                                     normalTexture,
                                     giOutputTexture,
                                     giStrength,
                                     useNormalMap);
        cmd->EndDebugGroup();
    } else {
        LOG_WARN("MLGISystem", "  [3/3] ScreenGather — SKIPPED (resources not ready)");
    }

    LOG_INFO("MLGISystem", "=== MLGI Dispatch Complete ===");
}

void MLGISystem::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_screenGatherPass) {
        m_screenGatherPass->Shutdown();
        m_screenGatherPass.reset();
    }

    if (m_temporalPass) {
        m_temporalPass->Cleanup();
        m_temporalPass.reset();
    }

    if (m_probeUpdatePass) {
        m_probeUpdatePass->Cleanup();
        m_probeUpdatePass.reset();
    }

    m_shBuffers.Destroy();
    ReleaseProbeBuffers();
    m_enabled = false;
    m_initialized = false;
    m_renderSystem = nullptr;

    LOG_INFO("MLGISystem", "MLGI shutdown complete");
}

void MLGISystem::DebugDumpSH() const {
    if (!IsReady() || m_cpuShBuffer.probeCount == 0) {
        LOG_INFO("MLGISystem", "DebugDumpSH: MLGI not ready or no probes");
        return;
    }

    LOG_INFO("MLGISystem", "DebugDumpSH: probes={}, coeffs={}, frame={}",
             m_cpuShBuffer.probeCount, m_cpuShBuffer.coeffCount, m_probeGrid.frameIndex);

    uint32_t nonZeroCount = 0;
    for (uint32_t i = 0; i < m_cpuShBuffer.coeffCount; ++i) {
        if (std::fabs(m_cpuShBuffer.currentData[i]) > 1e-6f) {
            nonZeroCount++;
        }
    }
    LOG_INFO("MLGISystem", "DebugDumpSH: non-zero current coeffs={}/{}",
             nonZeroCount, m_cpuShBuffer.coeffCount);
}

MLGISystem::Metrics MLGISystem::GetMetrics() const {
    Metrics m;
    m.enabled            = m_enabled;
    m.rayQueryAvailable  = m_renderSystem && m_renderSystem->GetDevice()
                           ? m_renderSystem->GetDevice()->IsRayQuerySupported()
                           : false;
    m.probeCount         = m_probeGrid.probeCount;
    m.raysPerProbe       = m_config.raysPerProbe;
    m.temporalFrameCount = m_shBuffers.temporalFrameCount;
    m.totalCoeffCount    = m_cpuShBuffer.coeffCount;

    if (m_enabled && m_cpuShBuffer.coeffCount > 0) {
        uint32_t nz = 0;
        for (uint32_t i = 0; i < m_cpuShBuffer.coeffCount; ++i) {
            if (std::fabs(m_cpuShBuffer.currentData[i]) > 1e-6f) ++nz;
        }
        m.nonZeroCoeffCount = nz;
    }

    if (!m_enabled && m_initialized) {
        if (!m_config.enableMLGI)
            m.fallbackReason = "disabled by config";
        else if (m_renderSystem && m_renderSystem->GetDevice()
                 && !m_renderSystem->GetDevice()->IsRayQuerySupported())
            m.fallbackReason = "RayQuery unsupported";
        else
            m.fallbackReason = "initialization failed";
    } else if (!m_initialized) {
        m.fallbackReason = "not initialized";
    }

    return m;
}

void MLGISystem::AllocateProbeBuffers() {
    uint32_t probeCount = m_probeGrid.probeCount;
    uint32_t coeffCount = probeCount * 9;

    m_cpuShBuffer.probeCount = probeCount;
    m_cpuShBuffer.coeffCount = coeffCount;
    m_cpuShBuffer.currentData.assign(coeffCount, 0.0f);
    m_cpuShBuffer.historyData.assign(coeffCount, 0.0f);
    m_cpuShBuffer.blendedData.assign(coeffCount, 0.0f);

    if (m_renderSystem) {
        auto* device = m_renderSystem->GetDevice();
        if (device) {
            auto* factory = device->GetResourceFactory();
            if (factory) {
                if (!m_shBuffers.Create(factory, probeCount)) {
                    LOG_WARN("MLGISystem", "Failed to create GPU SH buffers, using CPU-only fallback");
                }
            }
        }
    }

    LOG_INFO("MLGISystem", "Allocated SH9 buffers: {} probes x 9 coeffs = {} floats",
             probeCount, coeffCount);
}

void MLGISystem::ReleaseProbeBuffers() {
    m_cpuShBuffer.currentData.clear();
    m_cpuShBuffer.currentData.shrink_to_fit();
    m_cpuShBuffer.historyData.clear();
    m_cpuShBuffer.historyData.shrink_to_fit();
    m_cpuShBuffer.blendedData.clear();
    m_cpuShBuffer.blendedData.shrink_to_fit();
    m_cpuShBuffer.probeCount = 0;
    m_cpuShBuffer.coeffCount = 0;
}

} // namespace Prisma
