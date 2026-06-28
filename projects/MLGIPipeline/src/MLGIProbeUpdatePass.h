#pragma once

#include "graphic/interfaces/IComputePipeline.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "ProbeGrid.h"
#include <memory>
#include <cstdint>

namespace Prisma {

// ============================================================================
// MLGIProbeUpdatePass — GPU probe irradiance update via RayQuery
//
// Dispatches a compute shader (mlgi_probe_update.comp) that traces rays from
// each probe position using hardware-accelerated ray queries, projects the
// observed radiance onto SH9 coefficients, and writes the result to the
// probe SH output buffer.
//
// Usage:
//   1. Setup(device, grid, raysPerProbe) — create pipeline and descriptor sets
//   2. Execute(cmd, probeSamples, tlas, sceneSSBO, outputSH) — dispatch
//   3. Cleanup() — release resources
//
// Shader bindings (set 0):
//   binding 0: ProbeGridUBO       (std140 uniform) — grid origin/spacing/dim
//   binding 1: ProbeSamplesBlock  (std430 SSBO)     — deterministic 2D samples
//   binding 2: tlas               (accelerationStructureEXT)
//   binding 3: SceneData          (std430 SSBO)     — scene objects (readonly)
//   binding 4: ProbeSHOutput      (std430 SSBO)     — SH coefficients (write)
// ============================================================================

class MLGIProbeUpdatePass {
public:
    MLGIProbeUpdatePass();
    ~MLGIProbeUpdatePass();

    /// Create compute pipeline and descriptor sets.
    /// @param device        Valid IRenderDevice pointer
    /// @param grid          Probe grid configuration
    /// @param raysPerProbe  Number of rays per probe (clamped to 32)
    /// @return true on success
    bool Setup(Graphic::IRenderDevice* device,
               const ProbeGrid& grid,
               uint32_t raysPerProbe);

    /// Release all GPU resources.
    void Cleanup();

    /// Dispatch probe update compute shader.
    /// @param cmd               Valid command buffer
    /// @param probeSamplesBuffer  Probe samples SSBO (binding 1, readonly)
    /// @param tlasHandle        TLAS acceleration structure handle (binding 2)
    /// @param sceneSSBO         Scene objects SSBO (binding 3, readonly)
    /// @param outputSH          Output SH coefficients SSBO (binding 4, write)
    void Execute(Graphic::ICommandBuffer* cmd,
                 Graphic::IBuffer* probeSamplesBuffer,
                 void* tlasHandle,
                 Graphic::IBuffer* sceneSSBO,
                 Graphic::IBuffer* outputSH);

    bool IsReady() const { return m_ready; }

private:
    bool CreatePipeline(Graphic::IRenderDevice* device);
    bool CreateDescriptorSets(Graphic::IRenderDevice* device);

    // Probe grid UBO data layout matching mlgi_probe_update.comp std140.
    // Total: 96 bytes (6 vec4 equivalents)
    struct alignas(16) ProbeGridUBOData {
        glm::vec4  gridOrigin;       // offset  0: .xyz = origin, .w unused
        float      probeSpacing;     // offset 16
        uint32_t   _pad1[3];         // offset 20
        glm::ivec4 gridDim;          // offset 32: .xyz = dimensions, .w unused
        int32_t    raysPerProbe;     // offset 48
        float      maxTraceDist;     // offset 52
        uint32_t   _pad2[2];         // offset 56
        glm::vec4  skyColor;         // offset 64: .xyz = sky color, .w unused
        float      _pad3;            // offset 80
        uint32_t   _pad4[3];         // offset 84
    };

    Graphic::IRenderDevice*    m_device  = nullptr;
    Graphic::IResourceFactory* m_factory = nullptr;

    std::shared_ptr<Graphic::IShader>          m_probeUpdateShader;
    std::shared_ptr<Graphic::IComputePipeline> m_probeUpdatePipeline;
    std::shared_ptr<Graphic::IDescriptorSetLayout> m_descSetLayout;
    std::shared_ptr<Graphic::IDescriptorSet>       m_descSet;

    std::unique_ptr<Graphic::IBuffer> m_probeGridUBO;

    ProbeGrid  m_probeGrid;
    uint32_t   m_raysPerProbe = 16;
    bool       m_ready = false;
};

} // namespace Prisma
