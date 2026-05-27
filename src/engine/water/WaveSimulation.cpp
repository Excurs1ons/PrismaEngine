#include "water/WaveSimulation.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <cstring>

namespace Prisma::Water {

WaveSimulation::WaveSimulation()
{
    SetGerstnerWaves(WaterConfig().gerstnerWaves);
}

void WaveSimulation::SetGerstnerWaves(const std::vector<GerstnerWave>& waves)
{
    m_GerstnerWaves = waves;

    // Normalize directions
    for (auto& w : m_GerstnerWaves) {
        float len = glm::length(w.direction);
        if (len > 0.001f) {
            w.direction /= len;
        } else {
            w.direction = Vector2(1.0f, 0.0f);
        }
        // Clamp steepness to [0, 0.99)
        w.steepness = std::clamp(w.steepness, 0.0f, 0.99f);
    }

    m_GerstnerDirty = true;
}

void WaveSimulation::AdvanceTime(float deltaTime)
{
    m_Time += deltaTime;

    // Wrap time to avoid precision loss
    constexpr float k_MaxTime = 1000000.0f;
    if (m_Time > k_MaxTime) {
        m_Time -= k_MaxTime;
    }
}

WaveDisplacement WaveSimulation::EvaluateGerstner(const Vector2& worldPos, float time) const
{
    if (m_GerstnerWaves.empty()) {
        WaveDisplacement result;
        result.position = Vector3(worldPos.x, 0.0f, worldPos.y);
        result.normal = Vector3(0.0f, 1.0f, 0.0f);
        result.fold = 0.0f;
        return result;
    }

    // Gerstner wave evaluation
    // Reference: GPU Gems 1, Chapter 1 "Effective Water Simulation from Physical Models"
    Vector3 displaced(worldPos.x, 0.0f, worldPos.y);
    Vector3 tangent(1.0f, 0.0f, 0.0f);
    Vector3 bitangent(0.0f, 0.0f, 1.0f);

    for (const auto& wave : m_GerstnerWaves) {
        // Direction in 3D (XZ plane)
        Vector2 dir2D = wave.direction;
        float dirX = dir2D.x;
        float dirZ = dir2D.y;

        // Wave number k = frequency^2 / g (deep water dispersion)
        // For simplicity: k = frequency (adjusted for visual scale)
        float k = wave.frequency * wave.frequency / 9.81f;
        if (k < 0.001f) k = 0.001f;

        // Gerstner parameters
        float Qi = wave.steepness / (k * wave.amplitude * static_cast<float>(m_GerstnerWaves.size()));
        if (Qi > 1.0f) Qi = 1.0f;

        // Phase: angle in radians
        float theta = k * (dirX * worldPos.x + dirZ * worldPos.y) + wave.speed * time;

        // Cosine and sine
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        // Displacement
        float S = Qi * wave.amplitude * cosTheta;
        displaced.x += dirX * S;
        displaced.z += dirZ * S;
        displaced.y += wave.amplitude * sinTheta;

        // Normal calculation (analytical derivative)
        float WA = k * wave.amplitude;
        float C = Qi * cosTheta;
        float S_sin = sinTheta; // sin(theta)

        float derivX = dirX * (WA * cosTheta - Qi * WA * S_sin);
        float derivZ = dirZ * (WA * cosTheta - Qi * WA * S_sin);
        float derivY = WA * cosTheta;

        tangent.x += dirX * dirX * WA * C - Qi * WA * S_sin * dirX * dirX;
        tangent.z += dirX * dirZ * WA * C - Qi * WA * S_sin * dirX * dirZ;
        tangent.y += dirX * derivY;

        bitangent.x += dirZ * dirX * WA * C - Qi * WA * S_sin * dirZ * dirX;
        bitangent.z += dirZ * dirZ * WA * C - Qi * WA * S_sin * dirZ * dirZ;
        bitangent.y += dirZ * derivY;
    }

    // Compute normal from cross product of tangent and bitangent
    Vector3 normal = glm::normalize(glm::cross(bitangent, tangent));

    WaveDisplacement result;
    result.position = displaced;
    result.normal = normal;

    // Fold estimate: sum of steepness-weighted contributions
    float fold = 0.0f;
    for (const auto& wave : m_GerstnerWaves) {
        fold += wave.steepness * wave.amplitude;
    }
    result.fold = fold * 0.1f;

    return result;
}

WaveDisplacement WaveSimulation::Evaluate(const Vector2& worldPos) const
{
    return EvaluateGerstner(worldPos, m_Time);
}

float WaveSimulation::GetMaxAmplitude() const
{
    float maxAmp = 0.0f;
    for (const auto& w : m_GerstnerWaves) {
        maxAmp += w.amplitude;
    }
    return maxAmp;
}

// ============================================================================
// Phillips Spectrum Generation
// ============================================================================

std::vector<Vector4> WaveSimulation::GeneratePhillipsSpectrum(
    uint32_t resolution,
    float patchSize,
    float windSpeed,
    const Vector2& windDir,
    float amplitude,
    float lambda)
{
    // Generate Phillips spectrum for ocean waves
    // Reference: Tessendorf, "Simulating Ocean Water", SIGGRAPH 2001
    
    uint32_t total = resolution * resolution;
    std::vector<Vector4> spectrum(total);

    Vector2 wind = glm::normalize(windDir) * windSpeed;
    float windLen = glm::length(wind);
    if (windLen < 0.001f) windLen = 0.001f;

    float L = windLen * windLen / 9.81f; // largest wave from wind
    float A = amplitude;

    // Seeded random for phase initialization
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (uint32_t y = 0; y < resolution; ++y) {
        for (uint32_t x = 0; x < resolution; ++x) {
            // Wave vector
            float kx = (2.0f * PI * static_cast<float>(x)) / patchSize;
            float kz = (2.0f * PI * static_cast<float>(y)) / patchSize;

            // Center DC component
            if (x == 0 && y == 0) {
                spectrum[y * resolution + x] = Vector4(0.0f);
                continue;
            }

            float kLen = std::sqrt(kx * kx + kz * kz);
            if (kLen < 0.0001f) {
                spectrum[y * resolution + x] = Vector4(0.0f);
                continue;
            }

            // Normalized wave direction
            float kHatX = kx / kLen;
            float kHatZ = kz / kLen;

            // Wind alignment
            float windDotK = (wind.x * kHatX + wind.y * kHatZ);
            if (windDotK < 0.0f) windDotK = 0.0f;

            // Phillips spectrum
            float L2 = L * L;
            float kL2 = kLen * kLen * L2;
            float k4 = kLen * kLen * kLen * kLen;
            float phillips = A * std::exp(-1.0f / kL2) / (k4) * windDotK * windDotK;

            // Suppress waves shorter than 1cm (avoid numerical issues)
            float kSmall = 0.001f * 0.001f;
            phillips *= std::exp(-kLen * kLen * kSmall);

            // Choppy waves: displacement amplitude
            float choppyK = lambda / kLen;

            // Store spectrum as complex height + complex displacement
            // xy = complex height, zw = complex displacement magnitude
            // (the actual FFT will compute time evolution on GPU)
            spectrum[y * resolution + x] = Vector4(
                phillips * (dist(gen) * 2.0f - 1.0f),  // real
                phillips * (dist(gen) * 2.0f - 1.0f),  // imag
                choppyK * phillips * (dist(gen) * 2.0f - 1.0f), // displacement real
                choppyK * phillips * (dist(gen) * 2.0f - 1.0f)  // displacement imag
            );
        }
    }

    return spectrum;
}

} // namespace Prisma::Water
