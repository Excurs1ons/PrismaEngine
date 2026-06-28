#pragma once

#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include <memory>
#include <cstdint>

namespace Prisma {

// ============================================================================
// MLGITemporalPass — GPU temporal accumulation for probe SH coefficients
//
// Blends current-frame SH with history SH into blended/output SH using a
// configurable blend factor. Handles first-frame initialization and reset.
//
// Usage:
//   1. Setup(device) — create pipeline and descriptor sets
//   2. Execute(cmd, currentSH, historySH, blendedSH, ...) — dispatch blend
//   3. CopyBack(cmd, blendedSH, historySH) — copy blended back to history
//   4. Cleanup() — release resources
//
// Push constants layout (16 bytes):
//   float  blendFactor       — lerp factor (default 0.9)
//   uint   totalCoeffCount   — total vec4 coefficients (probeCount * 9)
//   uint   temporalFrame     — 0 = first frame (no history)
//   uint   resetFlag         — 1 = reset history
//
// SSBO bindings:
//   binding 0: currentSH  — current frame SH (read-only)
//   binding 1: historySH  — history SH (read-only)
//   binding 2: blendedSH  — blended output (write-only)
// ============================================================================

class MLGITemporalPass {
public:
    MLGITemporalPass();
    ~MLGITemporalPass();

    /// Create compute pipeline and descriptor sets.
    /// @param device  Valid IRenderDevice pointer
    /// @return true on success
    bool Setup(Graphic::IRenderDevice* device);

    /// Dispatch temporal blend: blended = lerp(history, current, blendFactor).
    /// First frame (temporalFrame == 0): copies current to blended.
    /// Reset (resetFlag == true): copies current to blended, clears history.
    /// @param cmd               Command buffer to record into
    /// @param currentSH         Current frame SH buffer (SSBO, read)
    /// @param historySH         History SH buffer (SSBO, read)
    /// @param blendedSH         Blended output SH buffer (SSBO, write)
    /// @param totalCoeffCount   Total vec4 coefficients (probeCount * 9)
    /// @param blendFactor       Lerp factor [0, 1], default 0.9
    /// @param temporalFrame     Frame count (0 = first frame)
    /// @param reset             If true, reset history
    void Execute(Graphic::ICommandBuffer* cmd,
                 Graphic::IBuffer* currentSH,
                 Graphic::IBuffer* historySH,
                 Graphic::IBuffer* blendedSH,
                 uint32_t totalCoeffCount,
                 float blendFactor,
                 uint32_t temporalFrame,
                 bool reset = false);

    /// Copy blended SH back to history SH at frame end.
    /// Uses a second dispatch to perform the copy.
    /// @param cmd         Command buffer to record into
    /// @param blendedSH   Source blended SH buffer (SSBO, read)
    /// @param historySH   Destination history SH buffer (SSBO, write)
    /// @param totalCoeffCount  Total vec4 coefficients
    void CopyBack(Graphic::ICommandBuffer* cmd,
                  Graphic::IBuffer* blendedSH,
                  Graphic::IBuffer* historySH,
                  uint32_t totalCoeffCount);

    /// Release all GPU resources.
    void Cleanup();

    bool IsReady() const { return m_ready; }

private:
    bool CreatePipelines();
    bool CreateDescriptorSets();

    Graphic::IRenderDevice*   m_device  = nullptr;
    Graphic::IResourceFactory* m_factory = nullptr;

    // Shader and pipeline
    std::shared_ptr<Graphic::IShader>          m_temporalShader;
    std::shared_ptr<Graphic::IComputePipeline> m_temporalPipeline;

    // Descriptor set for temporal blend dispatch
    std::shared_ptr<Graphic::IDescriptorSetLayout> m_descSetLayout;
    std::shared_ptr<Graphic::IDescriptorSet>       m_descSet;

    bool       m_ready = false;
};

} // namespace Prisma
