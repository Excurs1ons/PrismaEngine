#include <gtest/gtest.h>
#include <audio/dsp/AudioMath.h>
#include <audio/dsp/AudioBuffer.h> // for ConvertFormat, SampleFormat, AudioFormatEx

#include <cmath>
#include <cstring>
#include <cstdint>

using namespace Prisma::Audio::DSP;

// ============================================================================
// DB_TO_LINEAR / LINEAR_TO_DB
// ============================================================================

TEST(AudioMathTest, DbToLinear) {
    // 0 dB → gain 1.0
    EXPECT_FLOAT_EQ(AudioMath::DB_TO_LINEAR(0.0f), 1.0f);

    // -6 dB → gain ≈ 0.5
    float gain = AudioMath::DB_TO_LINEAR(-6.0f);
    EXPECT_NEAR(gain, 0.5f, 0.01f);

    // +6 dB → gain ≈ 2.0
    gain = AudioMath::DB_TO_LINEAR(6.0f);
    EXPECT_NEAR(gain, 2.0f, 0.01f);
}

TEST(AudioMathTest, LinearToDb) {
    // 1.0 → 0 dB
    EXPECT_FLOAT_EQ(AudioMath::LINEAR_TO_DB(1.0f), 0.0f);

    // 0.5 → ≈ -6 dB
    float db = AudioMath::LINEAR_TO_DB(0.5f);
    EXPECT_NEAR(db, -6.0f, 0.1f);

    // 2.0 → ≈ +6 dB
    db = AudioMath::LINEAR_TO_DB(2.0f);
    EXPECT_NEAR(db, 6.0f, 0.1f);
}

TEST(AudioMathTest, LinearToDbZero) {
    // Linear ≤ 0 → -144 dB (silence floor)
    EXPECT_FLOAT_EQ(AudioMath::LINEAR_TO_DB(0.0f), -144.0f);
    EXPECT_FLOAT_EQ(AudioMath::LINEAR_TO_DB(-1.0f), -144.0f);
}

TEST(AudioMathTest, DbToLinearRoundTrip) {
    float original = -12.0f;
    float linear   = AudioMath::DB_TO_LINEAR(original);
    float roundtrip = AudioMath::LINEAR_TO_DB(linear);
    EXPECT_NEAR(roundtrip, original, 0.01f);
}

// ============================================================================
// GainRamp
// ============================================================================

TEST(AudioMathTest, GainRamp) {
    EXPECT_FLOAT_EQ(AudioMath::GainRamp(0.0f, 1.0f, 10, 0), 0.0f);
    EXPECT_FLOAT_EQ(AudioMath::GainRamp(0.0f, 1.0f, 10, 9), 1.0f);
    // midpoint
    float mid = AudioMath::GainRamp(0.0f, 1.0f, 10, 4);
    EXPECT_NEAR(mid, 4.0f / 9.0f, 0.0001f);
}

TEST(AudioMathTest, GainRampSingleFrame) {
    EXPECT_FLOAT_EQ(AudioMath::GainRamp(0.5f, 1.0f, 1, 0), 1.0f);
}

// ============================================================================
// CalculatePan
// ============================================================================

TEST(AudioMathTest, CalculatePanCenter) {
    float left, right;
    AudioMath::CalculatePan(0.0f, left, right);
    EXPECT_NEAR(left, 0.7071f, 0.001f);
    EXPECT_NEAR(right, 0.7071f, 0.001f);
}

TEST(AudioMathTest, CalculatePanHardLeft) {
    float left, right;
    AudioMath::CalculatePan(-1.0f, left, right);
    EXPECT_NEAR(left, 1.0f, 0.001f);
    EXPECT_NEAR(right, 0.0f, 0.001f);
}

TEST(AudioMathTest, CalculatePanHardRight) {
    float left, right;
    AudioMath::CalculatePan(1.0f, left, right);
    EXPECT_NEAR(left, 0.0f, 0.001f);
    EXPECT_NEAR(right, 1.0f, 0.001f);
}

// ============================================================================
// FreqToMIDI / MIDIToFreq
// ============================================================================

TEST(AudioMathTest, FreqToMIDIA440) {
    float midi = AudioMath::FreqToMIDI(440.0f);
    EXPECT_FLOAT_EQ(midi, 69.0f);
}

TEST(AudioMathTest, MIDIToFreqA440) {
    float freq = AudioMath::MIDIToFreq(69.0f);
    EXPECT_FLOAT_EQ(freq, 440.0f);
}

TEST(AudioMathTest, MIDIToFreqC0) {
    float freq = AudioMath::MIDIToFreq(12.0f);  // C0 = 16.35 Hz
    EXPECT_NEAR(freq, 16.35f, 0.01f);
}

TEST(AudioMathTest, FreqToMIDIRoundTrip) {
    float original = 1000.0f;
    float midi     = AudioMath::FreqToMIDI(original);
    float roundtrip = AudioMath::MIDIToFreq(midi);
    EXPECT_NEAR(roundtrip, original, 0.001f);
}

TEST(AudioMathTest, FreqToMIDIZero) {
    EXPECT_FLOAT_EQ(AudioMath::FreqToMIDI(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(AudioMath::FreqToMIDI(-1.0f), 0.0f);
}

// ============================================================================
// 包络辅助函数
// ============================================================================

TEST(AudioMathTest, LinearRamp) {
    EXPECT_FLOAT_EQ(AudioMath::LinearRamp(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(AudioMath::LinearRamp(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(AudioMath::LinearRamp(0.5f), 0.5f);
}

TEST(AudioMathTest, LinearRampClamp) {
    EXPECT_FLOAT_EQ(AudioMath::LinearRamp(-0.5f), 0.0f);
    EXPECT_FLOAT_EQ(AudioMath::LinearRamp(1.5f), 1.0f);
}

TEST(AudioMathTest, ExpRamp) {
    float v0 = AudioMath::ExpRamp(0.0f);
    float v1 = AudioMath::ExpRamp(1.0f);
    EXPECT_FLOAT_EQ(v0, 0.0f);
    EXPECT_FLOAT_EQ(v1, 1.0f);
    // Exponential: fast initial rise
    EXPECT_GT(AudioMath::ExpRamp(0.5f), 0.5f);
}

TEST(AudioMathTest, SmoothStep) {
    EXPECT_FLOAT_EQ(AudioMath::SmoothStep(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(AudioMath::SmoothStep(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(AudioMath::SmoothStep(0.5f), 0.5f);
    // At 0.25: 0.25*0.25*(3-0.5) = 0.0625*2.5 = 0.15625
    EXPECT_FLOAT_EQ(AudioMath::SmoothStep(0.25f), 0.15625f);
}

TEST(AudioMathTest, SmoothStepClamp) {
    EXPECT_FLOAT_EQ(AudioMath::SmoothStep(-0.5f), 0.0f);
    EXPECT_FLOAT_EQ(AudioMath::SmoothStep(1.5f), 1.0f);
}

// ============================================================================
// 削波函数
// ============================================================================

TEST(AudioMathTest, HardClip) {
    EXPECT_FLOAT_EQ(AudioMath::HardClip(0.5f, 1.0f), 0.5f);
    EXPECT_FLOAT_EQ(AudioMath::HardClip(1.5f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(AudioMath::HardClip(-2.0f, 1.0f), -1.0f);
    EXPECT_FLOAT_EQ(AudioMath::HardClip(0.3f, 0.5f), 0.3f);
    EXPECT_FLOAT_EQ(AudioMath::HardClip(1.0f, 0.5f), 0.5f);
}

TEST(AudioMathTest, SoftClip) {
    // Near zero: approximately linear
    EXPECT_NEAR(AudioMath::SoftClip(0.1f, 1.0f), 0.1f, 0.001f);
    // Large input: approaches threshold
    float clipped = AudioMath::SoftClip(10.0f, 1.0f);
    EXPECT_NEAR(clipped, 1.0f, 0.01f);
    // Negative
    EXPECT_NEAR(AudioMath::SoftClip(-10.0f, 1.0f), -1.0f, 0.01f);
}

// ============================================================================
// 插值函数
// ============================================================================

TEST(AudioMathTest, LinearInterp) {
    EXPECT_FLOAT_EQ(AudioMath::LinearInterp(0.0f, 2.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(AudioMath::LinearInterp(0.0f, 2.0f, 1.0f), 2.0f);
    EXPECT_FLOAT_EQ(AudioMath::LinearInterp(0.0f, 2.0f, 0.5f), 1.0f);
}

TEST(AudioMathTest, HermiteInterp) {
    // At t=0: should equal y0
    float v = AudioMath::HermiteInterp(-1.0f, 0.0f, 1.0f, 2.0f, 0.0f);
    EXPECT_FLOAT_EQ(v, 0.0f);
    // At t=1: should equal y1
    v = AudioMath::HermiteInterp(-1.0f, 0.0f, 1.0f, 2.0f, 1.0f);
    EXPECT_FLOAT_EQ(v, 1.0f);
    // Linear signal: -1, 0, 1, 2 → hermite at t=0.5 should be 0.5
    v = AudioMath::HermiteInterp(-1.0f, 0.0f, 1.0f, 2.0f, 0.5f);
    EXPECT_NEAR(v, 0.5f, 0.001f);
}

// ============================================================================
// ConvertFormat — 格式转换
// ============================================================================

TEST(AudioMathTest, ConvertFormatFloat32ToInt16) {
    std::vector<float> src = {0.5f, -0.5f, 1.0f, -1.0f, 0.0f};
    std::vector<int16_t> dst(5);

    AudioFormatEx srcFmt;
    srcFmt.sampleRate = 44100;
    srcFmt.channels   = 1;
    srcFmt.format     = SampleFormat::Float32;
    srcFmt.frames     = 5;

    AudioFormatEx dstFmt;
    dstFmt.sampleRate = 44100;
    dstFmt.channels   = 1;
    dstFmt.format     = SampleFormat::Int16;
    dstFmt.frames     = 5;

    EXPECT_TRUE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(src.data()), srcFmt,
        reinterpret_cast<uint8_t*>(dst.data()), dstFmt, 5));

    // 0.5 → 16384, -0.5 → -16384, 1.0 → 32767, -1.0 → -32768, 0.0 → 0
    EXPECT_EQ(dst[0], 16384);
    EXPECT_EQ(dst[1], -16384);
    EXPECT_EQ(dst[2], 32767);
    EXPECT_EQ(dst[3], -32767); // round(-1.0 * 32767) = -32767
    EXPECT_EQ(dst[4], 0);
}

TEST(AudioMathTest, ConvertFormatInt16ToFloat32) {
    std::vector<int16_t> src = {16384, -16384, 32767, -32768, 0};
    std::vector<float> dst(5);

    AudioFormatEx srcFmt;
    srcFmt.sampleRate = 44100;
    srcFmt.channels   = 1;
    srcFmt.format     = SampleFormat::Int16;
    srcFmt.frames     = 5;

    AudioFormatEx dstFmt;
    dstFmt.sampleRate = 44100;
    dstFmt.channels   = 1;
    dstFmt.format     = SampleFormat::Float32;
    dstFmt.frames     = 5;

    EXPECT_TRUE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(src.data()), srcFmt,
        reinterpret_cast<uint8_t*>(dst.data()), dstFmt, 5));

    EXPECT_NEAR(dst[0], 0.5f, 0.001f);
    EXPECT_NEAR(dst[1], -0.5f, 0.001f);
    EXPECT_NEAR(dst[2], 1.0f, 0.001f);
    EXPECT_NEAR(dst[3], -1.0f, 0.001f);
    EXPECT_NEAR(dst[4], 0.0f, 0.001f);
}

TEST(AudioMathTest, ConvertFormatFormatPrecision) {
    // Round-trip test: float → int16 → float
    std::vector<float> src = {0.333f, 0.001f, -0.999f};
    std::vector<int16_t> temp(3);
    std::vector<float> dst(3);

    AudioFormatEx fmt1ch;
    fmt1ch.sampleRate = 44100;
    fmt1ch.channels   = 1;
    fmt1ch.frames     = 3;

    AudioFormatEx fmtFloat;
    fmtFloat = fmt1ch;
    fmtFloat.format = SampleFormat::Float32;

    AudioFormatEx fmtInt16;
    fmtInt16 = fmt1ch;
    fmtInt16.format = SampleFormat::Int16;

    // float → int16
    EXPECT_TRUE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(src.data()), fmtFloat,
        reinterpret_cast<uint8_t*>(temp.data()), fmtInt16, 3));

    // int16 → float
    EXPECT_TRUE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(temp.data()), fmtInt16,
        reinterpret_cast<uint8_t*>(dst.data()), fmtFloat, 3));

    // Round-trip should be within int16 quantization error
    for (int i = 0; i < 3; ++i) {
        EXPECT_NEAR(dst[i], src[i], 2.0f / 32768.0f);
    }
}

TEST(AudioMathTest, ConvertFormatNullPtr) {
    AudioFormatEx fmt;
    fmt.sampleRate = 44100;
    fmt.channels   = 1;
    fmt.format     = SampleFormat::Float32;
    fmt.frames     = 10;

    float dummy[10];
    EXPECT_FALSE(ConvertFormat(nullptr, fmt,
        reinterpret_cast<uint8_t*>(dummy), fmt, 10));
    EXPECT_FALSE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(dummy), fmt,
        nullptr, fmt, 10));
}

TEST(AudioMathTest, ConvertFormatZeroFrames) {
    AudioFormatEx fmt;
    fmt.sampleRate = 44100;
    fmt.channels   = 1;
    fmt.format     = SampleFormat::Float32;
    fmt.frames     = 0;

    float dummy[10]{};
    EXPECT_TRUE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(dummy), fmt,
        reinterpret_cast<uint8_t*>(dummy), fmt, 0));
}

TEST(AudioMathTest, ConvertFormatUnsupported) {
    // Int24 is not directly supported by ConvertFormat
    AudioFormatEx srcFmt, dstFmt;
    srcFmt.sampleRate = dstFmt.sampleRate = 44100;
    srcFmt.channels   = dstFmt.channels = 1;
    srcFmt.format     = SampleFormat::Int24;
    dstFmt.format     = SampleFormat::Float32;
    srcFmt.frames     = dstFmt.frames = 4;

    int32_t dummy[4]{};
    EXPECT_FALSE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(dummy), srcFmt,
        reinterpret_cast<uint8_t*>(dummy), dstFmt, 4));
}

TEST(AudioMathTest, ConvertFormatSameFormatSameChannels) {
    AudioFormatEx fmt;
    fmt.sampleRate = 44100;
    fmt.channels   = 2;
    fmt.format     = SampleFormat::Float32;
    fmt.frames     = 4;

    float data[8] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    float dst[8]  = {};

    EXPECT_TRUE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(data), fmt,
        reinterpret_cast<uint8_t*>(dst), fmt, 4));
    // Should be an exact memcpy
    for (int i = 0; i < 8; ++i) {
        EXPECT_FLOAT_EQ(dst[i], data[i]);
    }
}

TEST(AudioMathTest, ConvertFormatMonoToStereo) {
    AudioFormatEx srcFmt, dstFmt;
    srcFmt.sampleRate = dstFmt.sampleRate = 44100;
    srcFmt.channels   = 1;
    dstFmt.channels   = 2;
    srcFmt.format     = dstFmt.format = SampleFormat::Float32;
    srcFmt.frames     = 3;
    dstFmt.frames     = 3;

    float mono[3] = {0.5f, -0.3f, 0.8f};
    float stereo[6] = {};

    EXPECT_TRUE(ConvertFormat(
        reinterpret_cast<const uint8_t*>(mono), srcFmt,
        reinterpret_cast<uint8_t*>(stereo), dstFmt, 3));

    // Mono → stereo: both channels get the same value
    EXPECT_FLOAT_EQ(stereo[0], 0.5f);
    EXPECT_FLOAT_EQ(stereo[1], 0.5f);
    EXPECT_FLOAT_EQ(stereo[2], -0.3f);
    EXPECT_FLOAT_EQ(stereo[3], -0.3f);
    EXPECT_FLOAT_EQ(stereo[4], 0.8f);
    EXPECT_FLOAT_EQ(stereo[5], 0.8f);
}

TEST(AudioMathTest, ConvertFormatUnsupportedInt24Float) {
    AudioFormatEx srcFmt, dstFmt;
    srcFmt.sampleRate = dstFmt.sampleRate = 44100;
    srcFmt.channels   = dstFmt.channels = 1;
    srcFmt.format     = SampleFormat::Int24;
    dstFmt.format     = SampleFormat::Int32;
    srcFmt.frames     = dstFmt.frames = 2;

    // Both non-float and non-int16 — should fail
    uint8_t dummy[8]{};
    EXPECT_FALSE(ConvertFormat(dummy, srcFmt, dummy, dstFmt, 2));
}
