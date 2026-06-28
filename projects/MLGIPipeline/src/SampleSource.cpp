#include "SampleSource.h"
#include "Logger.h"

#include <cmath>
#include <cstring>
#include <format>

namespace Prisma {

// ============================================================================
// Halton sequence: radical inverse in given base
// ============================================================================
float SampleSource::Halton(uint32_t index, uint32_t base) {
    float result = 0.0f;
    float invBase = 1.0f / static_cast<float>(base);
    float fraction = invBase;
    while (index > 0) {
        result += static_cast<float>(index % base) * fraction;
        index /= base;
        fraction *= invBase;
    }
    return result;
}

// ============================================================================
// Radical inverse for Hammersley: bit-reversal in given base
// ============================================================================
float SampleSource::RadicalInverse(uint32_t i, uint32_t base) {
    float result = 0.0f;
    float invBase = 1.0f / static_cast<float>(base);
    float fraction = invBase;
    while (i > 0) {
        result += static_cast<float>(i % base) * fraction;
        i /= base;
        fraction *= invBase;
    }
    return result;
}

// ============================================================================
// Constructor
// ============================================================================
SampleSource::SampleSource(uint32_t seed, uint32_t sampleCount, SampleMode mode)
    : m_seed(seed)
    , m_sampleCount(sampleCount)
    , m_mode(mode)
{
}

// ============================================================================
// Generate — produce the full sample sequence
// ============================================================================
void SampleSource::Generate() {
    if (m_generated) return;
    if (m_sampleCount == 0) m_sampleCount = 1024;

    m_samples.resize(m_sampleCount);

    for (uint32_t i = 0; i < m_sampleCount; ++i) {
        uint32_t idx = m_seed + i; // deterministic offset by seed

        switch (m_mode) {
        case SampleMode::Halton:
            m_samples[i].x = Halton(idx, 2); // base 2 for x
            m_samples[i].y = Halton(idx, 3); // base 3 for y
            break;

        case SampleMode::Hammersley:
            m_samples[i].x = static_cast<float>(i) / static_cast<float>(m_sampleCount);
            m_samples[i].y = RadicalInverse(idx, 2);
            break;
        }
    }

    m_generated = true;

    // ── Log seed/sequence dimensions for reproducible static QA ──
    LOG_INFO("SampleSource",
             "Generated {} {} samples (seed={}, count={}, sizeof(ProbeSample)={})",
             ToString(),
             (m_mode == SampleMode::Halton) ? "Halton" : "Hammersley",
             m_seed,
             m_sampleCount,
             sizeof(ProbeSample));
}

// ============================================================================
// GetSample — wrap-around access
// ============================================================================
ProbeSample SampleSource::GetSample(uint32_t index) const {
    if (m_samples.empty()) return {0.0f, 0.0f};
    return m_samples[index % m_sampleCount];
}

// ============================================================================
// GPU buffer helpers (std430 layout)
//   Header (16 bytes):
//     [0] uint sampleCount
//     [1] uint seed
//     [2] uint mode
//     [3] uint pad
//   Payload:
//     [4..] vec2 samples[]  (8 bytes each, std430 vec2 is 8 bytes)
// ============================================================================
size_t SampleSource::GetBufferSize() const {
    return 16 + m_sampleCount * sizeof(ProbeSample);
}

const void* SampleSource::GetBufferData() const {
    if (!m_generated) return nullptr;

    m_gpuBuffer.resize(GetBufferSize());

    uint32_t* header = reinterpret_cast<uint32_t*>(m_gpuBuffer.data());
    header[0] = m_sampleCount;
    header[1] = m_seed;
    header[2] = static_cast<uint32_t>(m_mode);
    header[3] = 0;

    std::memcpy(m_gpuBuffer.data() + 16, m_samples.data(), m_sampleCount * sizeof(ProbeSample));

    return m_gpuBuffer.data();
}

// ============================================================================
// ToString — human-readable description
// ============================================================================
std::string SampleSource::ToString() const {
    const char* modeStr = (m_mode == SampleMode::Halton) ? "Halton" : "Hammersley";
    return std::format("SampleSource[mode={}, seed={}, count={}, generated={}]",
                       modeStr, m_seed, m_sampleCount, m_generated ? "yes" : "no");
}

} // namespace Prisma
