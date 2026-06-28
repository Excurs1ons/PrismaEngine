#pragma once

#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/ShaderReflection.h"
#include "graphic/RenderDesc.h"
#include "ProbeGrid.h"
#include <memory>
#include <cstdint>
#include <vector>

namespace Prisma {

// ============================================================================
// MLGIScreenGatherPass — Screen-space GI gather/composite pass
//
// Dispatches a compute shader that:
//   1. Reads depth (and optionally normal) from scene textures
//   2. Reconstructs world position from depth + camera data
//   3. Performs trilinear lookup into the blended SH probe buffer
//   4. Evaluates SH9 at the surface direction
//   5. Writes GI contribution to an output storage image
//
// Out-of-grid positions are handled via clamp-to-edge on the probe grid,
// so no invalid memory access occurs.
//
// The pass can be enabled/disabled at runtime. When disabled, the output
// buffer is cleared to black (zero GI contribution).
// ============================================================================

class MLGIScreenGatherPass {
public:
    MLGIScreenGatherPass();
    ~MLGIScreenGatherPass();

    // ── Lifecycle ──

    /// Initialize the pass: create compute pipeline, descriptor sets, and
    /// internal resources. Must be called before Dispatch().
    /// @param device       Valid render device pointer
    /// @param probeCount   Number of probes in the grid (for SH buffer binding)
    /// @param width        Output buffer width in pixels
    /// @param height       Output buffer height in pixels
    /// @return true on success
    bool Initialize(Graphic::IRenderDevice* device,
                    uint32_t probeCount,
                    uint32_t width,
                    uint32_t height);

    /// Shutdown and release all GPU resources.
    void Shutdown();

    // ── Runtime ──

    /// Dispatch the screen gather compute shader.
    /// @param cmd               Valid command buffer
    /// @param blendedSHBuffer   GPU buffer containing blended SH coefficients
    /// @param depthTexture      Depth texture (sampler2D, binding 1)
    /// @param normalTexture     Normal texture (optional, binding 5; may be null)
    /// @param giOutputTexture   Output storage image (rgba16f, binding 0)
    /// @param giStrength        GI contribution multiplier
    /// @param useNormalMap      Whether to use the normal texture for SH evaluation
    void Dispatch(Graphic::ICommandBuffer* cmd,
                  Graphic::IBuffer* blendedSHBuffer,
                  Graphic::ITexture* depthTexture,
                  Graphic::ITexture* normalTexture,
                  Graphic::ITexture* giOutputTexture,
                  float giStrength,
                  bool useNormalMap);

    // ── State ──

    bool IsInitialized() const { return m_initialized; }
    bool IsEnabled()     const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// Update probe grid parameters (called when grid changes).
    void SetProbeGrid(const ProbeGrid& grid);

    /// Update output dimensions (called on resize).
    void Resize(uint32_t width, uint32_t height);

    // ── Accessors ──

    Graphic::IComputePipeline* GetPipeline() const { return m_pipeline.get(); }
    Graphic::IDescriptorSet*   GetDescriptorSet() const { return m_descriptorSet.get(); }

private:
    // ── Internal helpers ──

    bool CreatePipeline(Graphic::IRenderDevice* device);
    bool CreateDescriptorSet(Graphic::IRenderDevice* device);
    bool CreateOutputBuffer(Graphic::IRenderDevice* device, uint32_t width, uint32_t height);

    // ── Resources ──

    std::unique_ptr<Graphic::IComputePipeline> m_pipeline;
    std::shared_ptr<Graphic::IShader>           m_screenGatherShader;
    std::shared_ptr<Graphic::IDescriptorSet>   m_descriptorSet;
    std::unique_ptr<Graphic::IBuffer>          m_probeGridUBO;
    std::unique_ptr<Graphic::IBuffer>          m_cameraUBO;

    // ── State ──

    ProbeGrid           m_probeGrid;
    uint32_t            m_width  = 0;
    uint32_t            m_height = 0;
    bool                m_initialized = false;
    bool                m_enabled      = true;

    // Push constant layout matches GLSL:
    //   float giStrength
    //   float ambientFallback
    //   int   useNormalMap
    //   int   _pad0
    struct GatherPushConstants {
        float giStrength      = 1.0f;
        float ambientFallback = 0.02f;
        int   useNormalMap    = 0;
        int   _pad0           = 0;
    };
};

} // namespace Prisma
