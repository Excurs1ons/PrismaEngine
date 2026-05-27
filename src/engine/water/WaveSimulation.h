#pragma once

#include "math/MathTypes.h"
#include "WaterConfig.h"
#include "Export.h"
#include <vector>
#include <cstdint>

namespace Prisma::Water {

/**
 * @brief Wave displacement result at a surface point
 */
struct WaveDisplacement {
    Vector3 position;   // displaced world position
    Vector3 normal;     // surface normal at displaced position
    float fold;         // choppy fold factor (0 = flat, >0 = folded)
};

/**
 * @brief FFT displacement map data (CPU-side storage)
 */
struct FFTDisplacementMap {
    std::vector<Vector4> displacement; // RG = slope XZ, B = height, A = fold
    uint32_t resolution = 0;
    float patchSize = 100.0f;
};

/**
 * @brief WaveSimulation - Gerstner + optional GPU FFT wave simulation
 * 
 * Provides two modes:
 * 1. Gerstner waves (CPU/GPU) - sum of directional sinusoids, good for open water
 * 2. FFT waves (GPU compute) - Phillips spectrum based, more realistic
 * 
 * CPU fallback: Gerstner only (FFT is too expensive on CPU for real-time)
 */
class ENGINE_API WaveSimulation {
public:
    WaveSimulation();
    ~WaveSimulation() = default;

    // ========== Configuration ==========

    /**
     * @brief Configure Gerstner wave components
     */
    void SetGerstnerWaves(const std::vector<GerstnerWave>& waves);

    /**
     * @brief Configure FFT parameters
     */
    void SetFFTConfig(const FFTWaveConfig& config) { m_FFTConfig = config; }

    /**
     * @brief Enable/disable FFT mode
     */
    void SetUseFFT(bool useFFT) { m_UseFFT = useFFT; }

    // ========== Time ==========

    /**
     * @brief Advance simulation time
     */
    void AdvanceTime(float deltaTime);

    /**
     * @brief Set time directly (e.g., for deterministic replay)
     */
    void SetTime(float time) { m_Time = time; }

    /**
     * @brief Get current simulation time
     */
    float GetTime() const { return m_Time; }

    // ========== Gerstner Evaluation (CPU) ==========

    /**
     * @brief Compute wave displacement + normal at a world position
     * @param worldPos  world XZ position to evaluate
     * @param time      current simulation time
     * @return displaced position, normal, and fold
     * 
     * Uses Gerstner waves. Computes both displacement and analytical normal.
     */
    WaveDisplacement EvaluateGerstner(const Vector2& worldPos, float time) const;

    /**
     * @brief Compute wave displacement + normal at world position (uses current time)
     */
    WaveDisplacement Evaluate(const Vector2& worldPos) const;

    // ========== FFT (GPU) ==========

    /**
     * @brief Get FFT displacement map for GPU upload
     */
    const FFTDisplacementMap& GetDisplacementMap() const { return m_DisplacementMap; }

    /**
     * @brief Generate Phillips spectrum (for compute shader)
     * @param resolution  texture resolution (must be power of 2)
     * @param patchSize   world size of simulated patch
     * @param windSpeed   wind speed in m/s
     * @param windDir     normalized wind direction
     * @param amplitude   amplitude scale
     * @param lambda      choppiness factor
     * @return float array of height field data (resolution*resolution complex values)
     */
    static std::vector<Vector4> GeneratePhillipsSpectrum(
        uint32_t resolution,
        float patchSize,
        float windSpeed,
        const Vector2& windDir,
        float amplitude,
        float lambda);

    // ========== Utility ==========

    /**
     * @brief Get number of active Gerstner waves
     */
    size_t GetWaveCount() const { return m_GerstnerWaves.size(); }

    /**
     * @brief Get Gerstner wave data (for GPU upload)
     */
    const std::vector<GerstnerWave>& GetWaves() const { return m_GerstnerWaves; }

    /**
     * @brief Get wave height range
     */
    float GetMaxAmplitude() const;

    /**
     * @brief Check whether FFT mode is enabled
     */
    bool IsFFTEnabled() const { return m_UseFFT; }

private:
    // Gerstner wave data
    std::vector<GerstnerWave> m_GerstnerWaves;
    bool m_GerstnerDirty = true;

    // FFT data
    FFTWaveConfig m_FFTConfig;
    FFTDisplacementMap m_DisplacementMap;
    bool m_UseFFT = false;

    // Simulation time
    float m_Time = 0.0f;
};

} // namespace Prisma::Water
