#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace Prisma {

// ============================================================================
// SampleMode — deterministic low-discrepancy sequence type
// ============================================================================
enum class SampleMode : uint32_t {
    Halton     = 0,  // Halton(2) for x, Halton(3) for y
    Hammersley = 1   // Hammersley 2D (Van der Corput base 2 + stratified)
};

// ============================================================================
// ProbeSample — single 2D sample in [0,1]² (matches GLSL vec2 layout)
// ============================================================================
struct ProbeSample {
    float x, y;
};

// ============================================================================
// SampleSource — deterministic low-discrepancy sample source for probe rays
//
// Generates a fixed-size sequence of 2D samples at startup using either
// Halton or Hammersley. The seed/sequence dimensions are logged for
// reproducible static QA. The raw buffer can be uploaded to GPU as an SSBO.
// ============================================================================
class SampleSource {
public:
    explicit SampleSource(
        uint32_t seed        = 42,
        uint32_t sampleCount = 1024,
        SampleMode mode      = SampleMode::Halton
    );

    // Generate the sample sequence (called once at startup)
    void Generate();

    // ── Accessors ──

    const ProbeSample* GetSamples()  const { return m_samples.data(); }
    uint32_t           GetCount()    const { return m_sampleCount; }
    uint32_t           GetSeed()     const { return m_seed; }
    SampleMode         GetMode()     const { return m_mode; }
    bool               IsGenerated() const { return m_generated; }

    // Get sample by index (wraps around via modulo)
    ProbeSample GetSample(uint32_t index) const;

    // ── GPU buffer helpers (std430 layout) ──
    // Buffer layout:
    //   [0] uint sampleCount  (4 bytes)
    //   [1] uint seed         (4 bytes)
    //   [2] uint mode         (4 bytes)
    //   [3] uint pad          (4 bytes)  → 16-byte header
    //   [4..] vec2 samples[]  (8 bytes each)
    size_t          GetBufferSize() const;
    const void*     GetBufferData() const;

    // ── Logging ──
    std::string     ToString() const;

private:
    // Halton sequence for a single dimension
    static float Halton(uint32_t index, uint32_t base);

    // Hammersley 2D: x = i/N, y = radicalInverse(i, base)
    static float RadicalInverse(uint32_t i, uint32_t base);

    uint32_t              m_seed;
    uint32_t              m_sampleCount;
    SampleMode            m_mode;
    bool                  m_generated = false;
    std::vector<ProbeSample> m_samples;
    // Cached GPU buffer (header + samples)
    mutable std::vector<uint8_t> m_gpuBuffer;
};

} // namespace Prisma
