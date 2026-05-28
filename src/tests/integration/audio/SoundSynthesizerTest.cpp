#include <gtest/gtest.h>
#include "audio/SoundSynthesizer.h"
#include "audio/AudioTypes.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace Prisma {
namespace {

static int16_t ReadSample16(const Prisma::Audio::AudioClip& clip, size_t frameIndex) {
    EXPECT_LE((frameIndex + 1) * sizeof(int16_t), clip.data.size());
    const int16_t* samples = reinterpret_cast<const int16_t*>(clip.data.data());
    return samples[frameIndex];
}

static size_t SampleCount(const Prisma::Audio::AudioClip& clip) {
    return clip.data.size() / (clip.format.bitsPerSample / 8);
}

// ======================================================================
// Waveform Format Basics
// ======================================================================

TEST(SoundSynthesizerTest, SineWaveFormat) {
    auto clip = SoundSynthesizer::generateSine(440.0f, 1.0f, 44100);
    EXPECT_EQ(clip.format.sampleRate, 44100u);
    EXPECT_EQ(clip.format.channels, 1u);
    EXPECT_EQ(clip.format.bitsPerSample, 16u);
    EXPECT_FLOAT_EQ(clip.duration, 1.0f);
    EXPECT_FALSE(clip.data.empty());
    EXPECT_EQ(clip.data.size(), 44100u * sizeof(int16_t));
    EXPECT_TRUE(clip.IsValid());
}

TEST(SoundSynthesizerTest, SineWaveSampleRange) {
    auto clip = SoundSynthesizer::generateSine(440.0f, 0.1f, 44100);
    size_t n = SampleCount(clip);
    for (size_t i = 0; i < n; ++i) {
        int16_t s = ReadSample16(clip, i);
        EXPECT_GE(s, -32767);
        EXPECT_LE(s, 32767);
    }
}

TEST(SoundSynthesizerTest, SineWaveZeroCrossings) {
    auto clip = SoundSynthesizer::generateSine(440.0f, 0.1f, 44100);
    size_t n = SampleCount(clip);

    int crossings = 0;
    for (size_t i = 1; i < n; ++i) {
        int16_t prev = ReadSample16(clip, i - 1);
        int16_t cur  = ReadSample16(clip, i);
        if ((prev >= 0 && cur < 0) || (prev < 0 && cur >= 0))
            ++crossings;
    }
    EXPECT_NEAR(crossings, 88, 4);
}

TEST(SoundSynthesizerTest, SquareWaveValues) {
    auto clip = SoundSynthesizer::generateSquare(440.0f, 0.1f, 44100);
    size_t n = SampleCount(clip);
    for (size_t i = 0; i < n; ++i) {
        int16_t s = ReadSample16(clip, i);
        EXPECT_TRUE(s == 32767 || s == -32767)
            << "Square sample[" << i << "] = " << s << " (expected ±32767)";
    }
}

TEST(SoundSynthesizerTest, SawtoothWaveRange) {
    auto clip = SoundSynthesizer::generateSawtooth(440.0f, 0.1f, 44100);
    size_t n = SampleCount(clip);
    for (size_t i = 0; i < n; ++i) {
        int16_t s = ReadSample16(clip, i);
        EXPECT_GE(s, -32767);
        EXPECT_LE(s, 32767);
    }
}

TEST(SoundSynthesizerTest, TriangleWaveRange) {
    auto clip = SoundSynthesizer::generateTriangle(440.0f, 0.1f, 44100);
    size_t n = SampleCount(clip);
    for (size_t i = 0; i < n; ++i) {
        int16_t s = ReadSample16(clip, i);
        EXPECT_GE(s, -32767);
        EXPECT_LE(s, 32767);
    }
}

TEST(SoundSynthesizerTest, TriangleWaveShape) {
    auto clip = SoundSynthesizer::generateTriangle(440.0f, 0.002f, 44100);
    size_t n = SampleCount(clip);
    ASSERT_GT(n, 2u) << "Need at least 2 samples for shape test";

    int16_t first = ReadSample16(clip, 0);
    // Triangle starts at phase=0: 4*abs(0-0.5)-1 = 1.0 → +32767
    EXPECT_EQ(first, 32767);
}

// ======================================================================
// Noise
// ======================================================================

TEST(SoundSynthesizerTest, NoiseNonConstant) {
    auto clip = SoundSynthesizer::generateNoise(0.1f, 44100);
    size_t n = SampleCount(clip);
    ASSERT_GT(n, 1u);

    int16_t first = ReadSample16(clip, 0);
    bool allSame = true;
    for (size_t i = 1; i < n; ++i) {
        if (ReadSample16(clip, i) != first) { allSame = false; break; }
    }
    EXPECT_FALSE(allSame);
}

TEST(SoundSynthesizerTest, NoiseInRange) {
    auto clip = SoundSynthesizer::generateNoise(0.1f, 44100);
    size_t n = SampleCount(clip);
    for (size_t i = 0; i < n; ++i) {
        int16_t s = ReadSample16(clip, i);
        EXPECT_GE(s, -32767);
        EXPECT_LE(s, 32767);
    }
}

// ======================================================================
// Determinism – same params → bit-identical output
// ======================================================================

TEST(SoundSynthesizerTest, SineDeterminism) {
    auto a = SoundSynthesizer::generateSine(440.0f, 0.5f, 44100);
    auto b = SoundSynthesizer::generateSine(440.0f, 0.5f, 44100);
    EXPECT_EQ(a.data, b.data);
    EXPECT_FLOAT_EQ(a.duration, b.duration);
    EXPECT_EQ(a.format.sampleRate, b.format.sampleRate);
    EXPECT_EQ(a.format.channels, b.format.channels);
    EXPECT_EQ(a.format.bitsPerSample, b.format.bitsPerSample);
}

TEST(SoundSynthesizerTest, SquareDeterminism) {
    auto a = SoundSynthesizer::generateSquare(200.0f, 0.3f, 48000);
    auto b = SoundSynthesizer::generateSquare(200.0f, 0.3f, 48000);
    EXPECT_EQ(a.data, b.data);
}

TEST(SoundSynthesizerTest, TriangleDeterminism) {
    auto a = SoundSynthesizer::generateTriangle(880.0f, 0.2f, 22050);
    auto b = SoundSynthesizer::generateTriangle(880.0f, 0.2f, 22050);
    EXPECT_EQ(a.data, b.data);
}

// ======================================================================
// Parameter changes on WaveformGenerator classes
// ======================================================================

TEST(SoundSynthesizerTest, SineFrequencyChange) {
    SineWaveGenerator gen;
    gen.SetVolume(0.5f);

    gen.SetFrequency(440.0f);
    std::vector<float> bufA(441);
    gen.GenerateSamples(bufA.data(), bufA.size());

    gen.SetFrequency(880.0f);
    std::vector<float> bufB(441);
    gen.GenerateSamples(bufB.data(), bufB.size());

    auto countCrossings = [](const std::vector<float>& v) {
        int c = 0;
        for (size_t i = 1; i < v.size(); ++i)
            if ((v[i - 1] >= 0 && v[i] < 0) || (v[i - 1] < 0 && v[i] >= 0)) ++c;
        return c;
    };
    int crossA = countCrossings(bufA);
    int crossB = countCrossings(bufB);

    EXPECT_GT(crossB, crossA) << "880 Hz should cross zero more than 440 Hz";
}

TEST(SoundSynthesizerTest, SineVolumeChange) {
    SineWaveGenerator gen;
    gen.SetFrequency(440.0f);

    gen.SetVolume(0.1f);
    std::vector<float> bufLow(441);
    gen.GenerateSamples(bufLow.data(), bufLow.size());
    float peakLow = 0.0f;
    for (float s : bufLow) peakLow = std::max(peakLow, std::abs(s));

    gen.SetVolume(0.9f);
    std::vector<float> bufHigh(441);
    gen.GenerateSamples(bufHigh.data(), bufHigh.size());
    float peakHigh = 0.0f;
    for (float s : bufHigh) peakHigh = std::max(peakHigh, std::abs(s));

    EXPECT_GT(peakHigh, peakLow);
    EXPECT_NEAR(peakLow, 0.1f, 0.01f);
    EXPECT_NEAR(peakHigh, 0.9f, 0.01f);
}

TEST(SoundSynthesizerTest, SquareWaveDutyCycleDefault) {
        auto clip = SoundSynthesizer::generateSquare(440.0f, 0.1f, 44100);
    size_t n = SampleCount(clip);
    int pos = 0, neg = 0;
    for (size_t i = 0; i < n; ++i) {
        int16_t s = ReadSample16(clip, i);
        if (s > 0) ++pos;
        else if (s < 0) ++neg;
    }
    float ratio = static_cast<float>(pos) / static_cast<float>(std::max(neg, 1));
    EXPECT_NEAR(ratio, 1.0f, 0.1f);
}

// ======================================================================
// WaveformGenerator determinism
// ======================================================================

TEST(SoundSynthesizerTest, WaveformGeneratorDeterminism) {
    SineWaveGenerator gen;
    gen.SetFrequency(440.0f);
    gen.SetVolume(0.5f);

    std::vector<float> bufA(2048);
    std::vector<float> bufB(2048);

    gen.GenerateSamples(bufA.data(), bufA.size());
    SineWaveGenerator gen2;
    gen2.SetFrequency(440.0f);
    gen2.SetVolume(0.5f);
    gen2.GenerateSamples(bufB.data(), bufB.size());

    for (size_t i = 0; i < bufA.size(); ++i) {
        EXPECT_FLOAT_EQ(bufA[i], bufB[i])
            << "Sample " << i << " differs between identical generators";
    }
}

// ======================================================================
// ADSR envelope
// ======================================================================

TEST(SoundSynthesizerTest, ADSRFirstSampleNearZero) {
    auto clip = SoundSynthesizer::generateSine(440.0f, 1.0f, 44100);
    SoundSynthesizer::generateWithADSR(clip, 0.1f, 0.1f, 0.7f, 0.2f);
    int16_t first = ReadSample16(clip, 0);
    EXPECT_NEAR(first, 0, 100);
}

TEST(SoundSynthesizerTest, ADSRPeakInAttack) {
    auto clip = SoundSynthesizer::generateSine(440.0f, 1.0f, 44100);
    SoundSynthesizer::generateWithADSR(clip, 0.1f, 0.1f, 0.7f, 0.2f);

    size_t attackEnd = static_cast<size_t>(0.095f * 44100);
    int16_t val = ReadSample16(clip, attackEnd);
    EXPECT_GT(std::abs(val), 29000);
}

TEST(SoundSynthesizerTest, ADSRReleaseEndNearZero) {
    auto clip = SoundSynthesizer::generateSine(440.0f, 1.0f, 44100);
    SoundSynthesizer::generateWithADSR(clip, 0.1f, 0.1f, 0.7f, 0.2f);
    size_t n = SampleCount(clip);

    int16_t last = ReadSample16(clip, n - 5);
    EXPECT_NEAR(last, 0, 500);
}

// ======================================================================
// generateTone wrapper
// ======================================================================

TEST(SoundSynthesizerTest, GenerateToneAllWaveforms) {
    for (int w = 0; w <= static_cast<int>(Waveform::Noise); ++w) {
        auto wf   = static_cast<Waveform>(w);
        auto clip = SoundSynthesizer::generateTone(440.0f, 0.1f, wf);
        EXPECT_TRUE(clip.IsValid())
            << "generateTone waveform " << w << " should produce valid clip";
        EXPECT_EQ(clip.format.sampleRate, 44100u);
    }
}

// ======================================================================
// Short clip edge cases
// ======================================================================

TEST(SoundSynthesizerTest, VeryShortSine) {
    auto clip = SoundSynthesizer::generateSine(440.0f, 0.001f, 44100);
    EXPECT_TRUE(clip.IsValid());
    EXPECT_EQ(clip.data.size(), 44100u * sizeof(int16_t) / 1000);
}

} // namespace
} // namespace Prisma
