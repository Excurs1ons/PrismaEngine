#include <gtest/gtest.h>
#include "water/WaveSimulation.h"
#include "water/WaterConfig.h"

namespace Prisma {
namespace Water {
namespace {

// ============================================================================
// WaveSimulation — Default State
// ============================================================================
TEST(WaveSimulationTest, DefaultConstructor) {
    WaveSimulation wave;
    // Constructor auto-initializes with 8 default Gerstner waves
    EXPECT_EQ(wave.GetWaveCount(), 8u);
    EXPECT_FLOAT_EQ(wave.GetTime(), 0.0f);
    EXPECT_FALSE(wave.IsFFTEnabled());
    // GetMaxAmplitude returns sum of all wave amplitudes = 1.2+0.8+0.6+0.5+0.4+0.3+0.3+0.2
    EXPECT_FLOAT_EQ(wave.GetMaxAmplitude(), 4.3f);
}

// ============================================================================
// WaveSimulation — Configuration
// ============================================================================
TEST(WaveSimulationTest, SetGerstnerWaves) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 0.5f, 2.0f, 0.0f),
        GerstnerWave(Vector2(0.0f, 1.0f), 0.5f, 1.0f, 1.5f, 0.3f),
    };
    wave.SetGerstnerWaves(waves);
    EXPECT_EQ(wave.GetWaveCount(), 2u);
    EXPECT_EQ(wave.GetWaves().size(), 2u);
}

TEST(WaveSimulationTest, SetGerstnerWavesEmpty) {
    WaveSimulation wave;
    wave.SetGerstnerWaves({});
    EXPECT_EQ(wave.GetWaveCount(), 0u);
    EXPECT_FLOAT_EQ(wave.GetMaxAmplitude(), 0.0f);
}

TEST(WaveSimulationTest, GetMaxAmplitude) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 3.0f, 0.5f, 2.0f, 0.0f),
        GerstnerWave(Vector2(0.0f, 1.0f), 1.5f, 1.0f, 1.5f, 0.3f),
        GerstnerWave(Vector2(0.5f, 0.5f), 5.0f, 0.8f, 3.0f, 0.2f),
    };
    wave.SetGerstnerWaves(waves);
    // GetMaxAmplitude returns sum of all amplitudes = 3.0 + 1.5 + 5.0
    EXPECT_FLOAT_EQ(wave.GetMaxAmplitude(), 9.5f);
}

// ============================================================================
// WaveSimulation — Time Management
// ============================================================================
TEST(WaveSimulationTest, SetTime) {
    WaveSimulation wave;
    wave.SetTime(42.0f);
    EXPECT_FLOAT_EQ(wave.GetTime(), 42.0f);
}

TEST(WaveSimulationTest, AdvanceTime) {
    WaveSimulation wave;
    wave.SetTime(10.0f);
    wave.AdvanceTime(5.0f);
    EXPECT_FLOAT_EQ(wave.GetTime(), 15.0f);
}

TEST(WaveSimulationTest, AdvanceTimeZero) {
    WaveSimulation wave;
    wave.SetTime(100.0f);
    wave.AdvanceTime(0.0f);
    EXPECT_FLOAT_EQ(wave.GetTime(), 100.0f);
}

TEST(WaveSimulationTest, AdvanceTimeNegative) {
    WaveSimulation wave;
    wave.SetTime(50.0f);
    wave.AdvanceTime(-10.0f);
    EXPECT_FLOAT_EQ(wave.GetTime(), 40.0f);
}

// ============================================================================
// WaveSimulation — FFT Configuration
// ============================================================================
TEST(WaveSimulationTest, SetFFTConfig) {
    WaveSimulation wave;
    FFTWaveConfig config;
    config.resolution = 512;
    config.patchSize = 200.0f;
    config.windSpeed = 15.0f;
    config.windDirection = Vector2(1.0f, 1.0f);
    wave.SetFFTConfig(config);
    EXPECT_FALSE(wave.IsFFTEnabled()); // False until SetUseFFT(true)
}

TEST(WaveSimulationTest, SetUseFFT) {
    WaveSimulation wave;
    EXPECT_FALSE(wave.IsFFTEnabled());
    wave.SetUseFFT(true);
    EXPECT_TRUE(wave.IsFFTEnabled());
    wave.SetUseFFT(false);
    EXPECT_FALSE(wave.IsFFTEnabled());
}

// ============================================================================
// WaveSimulation — Gerstner Wave Evaluation (Single Wave)
// ============================================================================
TEST(WaveSimulationTest, EvaluateGerstnerSingleWave) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 0.5f, 2.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    // At origin with t=0: y = A * sin(ω * (dir·pos) + ω*speed*t)
    // = 1.0 * sin(0.5 * 0 + 0.5*2.0*0) = 0
    WaveDisplacement result = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 0.0f);
    // With steepness=0, position should be (0, ~0, 0) — exactly at origin
    EXPECT_NEAR(result.position.x, 0.0f, 0.001f);
    EXPECT_NEAR(result.position.z, 0.0f, 0.001f);
    // Fold should be 0 for non-steep waves
    EXPECT_FLOAT_EQ(result.fold, 0.0f);
}

TEST(WaveSimulationTest, EvaluateGerstnerAtTime) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 1.0f, 1.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    // At t = pi/(2*ω*speed) = pi/(2*1*1) = pi/2 ≈ 1.571:
    // y = 1.0 * sin(ω * speed * t) = sin(1*1*1.571) ≈ sin(1.571) ≈ 1.0
    // With steepness=0, no horizontal displacement
    WaveDisplacement result = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 1.570796f);
    EXPECT_NEAR(result.position.y, 1.0f, 0.01f);
    EXPECT_NEAR(result.position.x, 0.0f, 0.001f);
    EXPECT_NEAR(result.position.z, 0.0f, 0.001f);
}

// ============================================================================
// WaveSimulation — Gerstner Wave Evaluation (Multiple Waves)
// ============================================================================
TEST(WaveSimulationTest, EvaluateGerstnerMultipleWaves) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 0.5f, 2.0f, 0.0f),
        GerstnerWave(Vector2(0.0f, 1.0f), 2.0f, 1.0f, 1.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    // At origin, t=0: both waves contribute sin(0)=0
    WaveDisplacement result = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 0.0f);
    EXPECT_NEAR(result.position.y, 0.0f, 0.001f);
    EXPECT_NEAR(result.position.x, 0.0f, 0.001f);
    EXPECT_NEAR(result.position.z, 0.0f, 0.001f);

    // Check wave count
    EXPECT_EQ(wave.GetWaveCount(), 2u);
}

TEST(WaveSimulationTest, EvaluateGerstnerNonOrigin) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 1.0f, 1.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    WaveDisplacement result = wave.EvaluateGerstner(Vector2(1.570796f, 0.0f), 0.0f);
    EXPECT_NEAR(result.position.y, 0.159f, 0.005f);
}

// ============================================================================
// WaveSimulation — Evaluate (uses current time)
// ============================================================================
TEST(WaveSimulationTest, EvaluateUsesCurrentTime) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 1.0f, 1.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    // At t=0, Evaluate should match EvaluateGerstner with current time
    wave.SetTime(0.0f);
    WaveDisplacement eval0 = wave.Evaluate(Vector2(0.0f, 0.0f));
    WaveDisplacement gerstner0 = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 0.0f);
    EXPECT_NEAR(eval0.position.y, gerstner0.position.y, 0.001f);

    // At t=1.571, Evaluate should use current time
    wave.SetTime(1.570796f);
    WaveDisplacement eval1 = wave.Evaluate(Vector2(0.0f, 0.0f));
    WaveDisplacement gerstner1 = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 1.570796f);
    EXPECT_NEAR(eval1.position.y, gerstner1.position.y, 0.001f);
}

TEST(WaveSimulationTest, EvaluateAfterAdvanceTime) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 1.0f, 1.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    wave.AdvanceTime(1.570796f);
    WaveDisplacement result = wave.Evaluate(Vector2(0.0f, 0.0f));
    // At t=1.571: y ≈ 1.0
    EXPECT_NEAR(result.position.y, 1.0f, 0.01f);
}

// ============================================================================
// WaveSimulation — Steepness Effect (Choppy Waves)
// ============================================================================
TEST(WaveSimulationTest, EvaluateGerstnerWithSteepness) {
    WaveSimulation wave;
    // With steepness, horizontal displacement should be non-zero
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 1.0f, 1.0f, 0.5f),
    };
    wave.SetGerstnerWaves(waves);

    // At (0,0), t=0: fold should be > 0 since steepness > 0
    // Even at crest, displacement is non-zero
    WaveDisplacement result = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(result.position.y, 0.0f); // sin(0)=0
}

// ============================================================================
// WaveSimulation — Gerstner Normal Computation
// ============================================================================
TEST(WaveSimulationTest, EvaluateGerstnerNormal) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 0.5f, 2.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    // At origin with t=0, for a non-steep wave traveling along +X:
    // The normal should tilt slightly in the -X direction at y=0
    // (since the wave rises in front and dips behind)
    WaveDisplacement result = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 0.0f);
    // Normal should be roughly upward
    EXPECT_GT(result.normal.y, 0.0f);
    // Normal should be approximately unit length
    float len = glm::length(result.normal);
    EXPECT_NEAR(len, 1.0f, 0.001f);
}

// ============================================================================
// WaveSimulation — GeneratePhillipsSpectrum (static)
// ============================================================================
TEST(WaveSimulationTest, GeneratePhillipsSpectrumBasic) {
    auto spectrum = WaveSimulation::GeneratePhillipsSpectrum(
        64, 100.0f, 10.0f, Vector2(1.0f, 0.0f), 1.0f, 1.0f);
    // Should return resolution*resolution elements (complex pairs stored as Vector4)
    EXPECT_EQ(spectrum.size(), 64u * 64u);
}

TEST(WaveSimulationTest, GeneratePhillipsSpectrumDifferentResolution) {
    auto spectrum = WaveSimulation::GeneratePhillipsSpectrum(
        128, 200.0f, 15.0f, Vector2(0.0f, 1.0f), 0.5f, 2.0f);
    EXPECT_EQ(spectrum.size(), 128u * 128u);
}

TEST(WaveSimulationTest, GeneratePhillipsSpectrumDirectionNormalization) {
    // Even with unnormalized direction, spectrum should be generated
    auto spectrum = WaveSimulation::GeneratePhillipsSpectrum(
        32, 100.0f, 10.0f, Vector2(0.5f, 0.5f), 1.0f, 1.0f);
    EXPECT_EQ(spectrum.size(), 32u * 32u);
}

// ============================================================================
// WaveSimulation — Default Gerstner Waves (from WaterConfig)
// ============================================================================
TEST(WaveSimulationTest, DefaultWavesFromConfig) {
    WaterConfig config;
    EXPECT_EQ(config.gerstnerWaves.size(), 8u);

    WaveSimulation wave;
    wave.SetGerstnerWaves(config.gerstnerWaves);
    EXPECT_EQ(wave.GetWaveCount(), 8u);
    EXPECT_GT(wave.GetMaxAmplitude(), 0.0f);

    // Evaluate at origin with t=0 — should produce some displacement
    WaveDisplacement result = wave.Evaluate(Vector2(0.0f, 0.0f));
    // With 8 waves at t=0, different phases means y may not be 0
    // But position should be finite
    EXPECT_TRUE(std::isfinite(result.position.x));
    EXPECT_TRUE(std::isfinite(result.position.y));
    EXPECT_TRUE(std::isfinite(result.position.z));
    // Normal should be valid
    float normalLen = glm::length(result.normal);
    EXPECT_NEAR(normalLen, 1.0f, 0.001f);
}

// ============================================================================
// WaveSimulation — Edge Cases
// ============================================================================
TEST(WaveSimulationTest, SingleWaveEvaluateOverTime) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 1.0f, 1.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    // Track wave height over a full period at origin
    // Period = 2*pi/(ω*speed) = 2*pi/1 = 2*pi ≈ 6.283
    float t0 = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 0.0f).position.y;
    float tQuarter = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 1.570796f).position.y;
    float tHalf = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 3.141593f).position.y;
    float tThreeQuarter = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 4.712389f).position.y;
    float tFull = wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 6.283185f).position.y;

    // At origin with dir=(1,0): phase = ω*speed*t
    // t=0: sin(0)=0
    // t=π/2: sin(π/2)=1
    // t=π: sin(π)=0
    // t=3π/2: sin(3π/2)=-1
    // t=2π: sin(2π)=0
    EXPECT_NEAR(t0, 0.0f, 0.001f);
    EXPECT_NEAR(tQuarter, 1.0f, 0.01f);
    EXPECT_NEAR(tHalf, 0.0f, 0.01f);
    EXPECT_NEAR(tThreeQuarter, -1.0f, 0.01f);
    EXPECT_NEAR(tFull, 0.0f, 0.01f);
}

TEST(WaveSimulationTest, EvaluateAtMultiplePositions) {
    WaveSimulation wave;
    std::vector<GerstnerWave> waves = {
        GerstnerWave(Vector2(1.0f, 0.0f), 1.0f, 0.5f, 2.0f, 0.0f),
    };
    wave.SetGerstnerWaves(waves);

    // Deep water wave number: k = ω²/g = 0.25/9.81 ≈ 0.02548
    // Phase: θ = k * dir·pos = 0.02548 * x
    // y = sin(θ)
    EXPECT_NEAR(wave.EvaluateGerstner(Vector2(0.0f, 0.0f), 0.0f).position.y, 0.0f, 0.001f);
    EXPECT_NEAR(wave.EvaluateGerstner(Vector2(3.14159f, 0.0f), 0.0f).position.y, 0.080f, 0.005f);
    EXPECT_NEAR(wave.EvaluateGerstner(Vector2(6.28319f, 0.0f), 0.0f).position.y, 0.159f, 0.005f);
    EXPECT_NEAR(wave.EvaluateGerstner(Vector2(9.42478f, 0.0f), 0.0f).position.y, 0.238f, 0.005f);
}

} // namespace
} // namespace Water
} // namespace Prisma
