#pragma once

#include "graphic/interfaces/IBuffer.h"
#include "graphic/RenderDesc.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "Logger.h"
#include <memory>
#include <cstdint>
#include <cmath>
#include <cfloat>

namespace Prisma {

// ============================================================================
// SH9 Probe Buffer Declarations
//
// Layout: vec4/float4 per coefficient
//   Index: probeIndex * 9 + coeffIndex   (0 <= coeffIndex < 9)
//   .xyz = RGB coefficient value
//   .w   = reserved (unused, set to 0.0)
//
// Each probe stores 9 spherical harmonic coefficients (SH9, order 3),
// totaling probeCount * 9 vec4 elements in GPU storage buffers.
//
// Three buffers are maintained for temporal accumulation:
//   currentProbeSH  — current frame's computed SH coefficients
//   historyProbeSH  — accumulated history from previous frames
//   blendedProbeSH  — temporally blended result for rendering
// ============================================================================
struct SHProbeBuffers {
    // ---- GPU storage buffers ----

    /// Current frame probe SH data (GPU storage buffer, read/write)
    std::unique_ptr<Graphic::IBuffer> currentProbeSH;

    /// History/temporal accumulation probe SH data (GPU storage buffer, read/write)
    std::unique_ptr<Graphic::IBuffer> historyProbeSH;

    /// Blended/interpolated probe SH data (GPU storage buffer, read-only for shading)
    std::unique_ptr<Graphic::IBuffer> blendedProbeSH;

    // ---- CPU-side metadata ----

    uint32_t probeCount = 0;         ///< Number of probes in the grid
    uint32_t temporalFrameCount = 0; ///< Number of temporal accumulation frames so far

    // ---- Derived queries ----

    /// Total number of SH9 coefficients across all probes (= probeCount * 9)
    uint32_t GetCoefficientCount() const { return probeCount * 9; }

    /// Total buffer size in bytes (= probeCount * 9 * sizeof(vec4))
    uint64_t GetBufferSize() const {
        return static_cast<uint64_t>(probeCount) * 9ULL * sizeof(PrismaMath::vec4);
    }

    // ---- Lifecycle ----

    /// Create all three SH storage buffers via the resource factory.
    /// @param factory  Valid IResourceFactory pointer
    /// @param count    Number of probes to allocate space for
    /// @return true if all three buffers were created successfully
    bool Create(Graphic::IResourceFactory* factory, uint32_t count) {
        if (!factory || count == 0) {
            LOG_ERROR("SHProbeBuffers", "Create: invalid factory or count=0");
            return false;
        }

        probeCount = count;
        const uint64_t bufSize = GetBufferSize();

        Graphic::BufferDesc desc{};
        desc.type  = Graphic::BufferType::Structured;
        desc.size  = bufSize;
        desc.stride = sizeof(PrismaMath::vec4);  // 16 bytes per coefficient
        desc.usage = Graphic::BufferUsage::ShaderResource
                   | Graphic::BufferUsage::UnorderedAccess
                   | Graphic::BufferUsage::Dynamic;  // CPU-writable for debug upload

        desc.name = "SHProbe_Current";
        currentProbeSH = factory->CreateBufferImpl(desc);
        if (!currentProbeSH) {
            LOG_ERROR("SHProbeBuffers", "Failed to create currentProbeSH ({} bytes)", bufSize);
            return false;
        }

        desc.name = "SHProbe_History";
        historyProbeSH = factory->CreateBufferImpl(desc);
        if (!historyProbeSH) {
            LOG_ERROR("SHProbeBuffers", "Failed to create historyProbeSH");
            return false;
        }

        desc.name = "SHProbe_Blended";
        blendedProbeSH = factory->CreateBufferImpl(desc);
        if (!blendedProbeSH) {
            LOG_ERROR("SHProbeBuffers", "Failed to create blendedProbeSH");
            return false;
        }

        temporalFrameCount = 0;
        LOG_INFO("SHProbeBuffers", "Created {} SH buffers, {} probes, {} coefficients, {} bytes each",
                 3, probeCount, GetCoefficientCount(), bufSize);
        return true;
    }

    /// Destroy all three SH buffers and reset state.
    void Destroy() {
        currentProbeSH.reset();
        historyProbeSH.reset();
        blendedProbeSH.reset();
        probeCount = 0;
        temporalFrameCount = 0;
    }

    // ---- Debug dump ----

    /// Dump SH buffer info via logger: probe count, coefficient count,
    /// temporal frame count, and a summary of non-zero / non-finite values.
    /// @param label     Descriptive label for the log output
    /// @param data      Pointer to CPU-side coefficient data (probeCount * 9 vec4)
    ///                  or nullptr if data is not available on CPU
    void DebugDump(const char* label, const float* data = nullptr) const {
        LOG_INFO("SHProbeBuffers",
                 "[{}] probes={} coeffs={} temporalFrames={} bufSize={} bytes",
                 label ? label : "SH",
                 probeCount,
                 GetCoefficientCount(),
                 temporalFrameCount,
                 GetBufferSize());

        if (data && probeCount > 0) {
            DebugDumpSH(label, probeCount, data);
        } else {
            LOG_INFO("SHProbeBuffers",
                     "[{}] CPU data not available for coefficient analysis", label ? label : "SH");
        }
    }

    /// Static helper: analyze a flat array of SH9 coefficient data and log a summary.
    /// @param label  Descriptive label
    /// @param count  Number of probes
    /// @param data   Flat array of probeCount * 9 * 4 floats (vec4 per coefficient)
    static void DebugDumpSH(const char* label, uint32_t count, const float* data) {
        if (!data || count == 0) return;

        const uint32_t totalCoeffs = count * 9;
        uint32_t nonZeroCount = 0;
        uint32_t nonFiniteCount = 0;
        float minVal = FLT_MAX;
        float maxVal = -FLT_MAX;
        float sumAbs = 0.0f;

        for (uint32_t i = 0; i < totalCoeffs; ++i) {
            // Each coefficient is a vec4: .xyz = RGB, .w = reserved
            const float* coeff = data + i * 4;

            // Check .xyz components for non-zero / non-finite
            bool hasNonZero = false;
            for (int c = 0; c < 3; ++c) {
                const float v = coeff[c];
                if (!std::isfinite(v)) {
                    ++nonFiniteCount;
                }
                if (std::fabs(v) > 1e-10f) {
                    hasNonZero = true;
                }
                if (v < minVal) minVal = v;
                if (v > maxVal) maxVal = v;
                sumAbs += std::fabs(v);
            }
            if (hasNonZero) ++nonZeroCount;

            // Warn if .w is non-zero (reserved field should be 0)
            if (std::fabs(coeff[3]) > 1e-10f) {
                LOG_WARN("SHProbeBuffers",
                         "[{}] coeff[{}].w = {} (reserved, expected 0.0)",
                         label ? label : "SH", i, coeff[3]);
            }
        }

        const float avgAbs = (totalCoeffs > 0) ? (sumAbs / static_cast<float>(totalCoeffs * 3)) : 0.0f;

        LOG_INFO("SHProbeBuffers",
                 "[{}] coeffs={} nonZero={} nonFinite={} | range=[{:.6f}, {:.6f}] avg|val|={:.6f}",
                 label ? label : "SH",
                 totalCoeffs, nonZeroCount, nonFiniteCount,
                 minVal, maxVal, avgAbs);

        if (nonFiniteCount > 0) {
            LOG_WARN("SHProbeBuffers",
                     "[{}] WARNING: {} coefficients contain NaN/Inf values!",
                     label ? label : "SH", nonFiniteCount);
        }
    }
};

} // namespace Prisma
