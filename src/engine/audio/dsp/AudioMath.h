#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <numbers>

namespace Prisma {
namespace Audio {
namespace DSP {
namespace AudioMath {

// ======================================================================
// Gain / Level
// ======================================================================

// Convert decibels to linear gain scale.
inline float DB_TO_LINEAR(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

// Convert linear gain to decibels.
inline float LINEAR_TO_DB(float linear)
{
    return linear <= 0.0f ? -144.0f : 20.0f * std::log10(linear);
}

// Linear gain ramp for per-frame interpolation.
inline float GainRamp(float startGain, float endGain, uint32_t frames, uint32_t index)
{
    if (frames <= 1) return endGain;
    float t = static_cast<float>(index) / static_cast<float>(frames - 1);
    return startGain + (endGain - startGain) * t;
}

// ======================================================================
// Pan (sine / cosine law)
// ======================================================================

// Calculate left/right channel gains from a pan position.
///
/// Uses the sine/cosine constant-power panning law:
///   leftGain  = cos((panPos + 1) * PI/4)
///   rightGain = sin((panPos + 1) * PI/4)
inline void CalculatePan(float panPos, float& leftGain, float& rightGain)
{
    constexpr float kPiOver4 = static_cast<float>(std::numbers::pi) / 4.0f;
    float angle                 = (panPos + 1.0f) * kPiOver4;
    leftGain                    = std::cos(angle);
    rightGain                   = std::sin(angle);
}

// ======================================================================
// Frequency / MIDI conversion
// ======================================================================

// Convert frequency (Hz) to MIDI note number.
inline float FreqToMIDI(float freq)
{
    if (freq <= 0.0f) return 0.0f;
    return 69.0f + 12.0f * std::log2(freq / 440.0f);
}

// Convert MIDI note number to frequency (Hz).
inline float MIDIToFreq(float midiNote)
{
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

// ======================================================================
// Ramp / envelope helper shapes
// ======================================================================

// Linear ramp, t clamped to [0, 1].
inline constexpr float LinearRamp(float t)
{
    return std::clamp(t, 0.0f, 1.0f);
}

// Exponential ramp with decay constant k=5.
///         Provides a fast-then-gentle curve suitable for envelope release.
///         y(t) = (1 - exp(-t*5)) / (1 - exp(-5))
inline float ExpRamp(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    constexpr float k = 5.0f;
    return (1.0f - std::exp(-t * k)) / (1.0f - std::exp(-k));
}

// Smooth Hermite-style step: t * t * (3 - 2 * t).
///         Also known as "smoothstep". t is clamped to [0, 1].
inline constexpr float SmoothStep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// ======================================================================
// Clipping / waveshaping
// ======================================================================

// Hard clip (clamp) a sample to [-threshold, +threshold].
inline constexpr float HardClip(float sample, float threshold)
{
    return std::clamp(sample, -threshold, threshold);
}

// Soft clipping with hyperbolic tangent curve.
///         Smooth, musical saturation as sample approaches threshold.
///         y = threshold * tanh(sample / threshold)
inline float SoftClip(float sample, float threshold)
{
    return threshold * std::tanh(sample / threshold);
}

// ======================================================================
// Interpolation
// ======================================================================

// Standard linear interpolation between two samples.
inline constexpr float LinearInterp(float y0, float y1, float t)
{
    return y0 + (y1 - y0) * t;
}

// 4-point, 3rd-order Hermite (Catmull-Rom) interpolation.
///
/// Coefficients:
///   c0 = y0
///   c1 = (y1 - ym1) / 2
///   c2 = ym1 - 2.5*y0 + 2*y1 - 0.5*y2
///   c3 = 0.5*(y2 - ym1) + 1.5*(y0 - y1)
inline constexpr float HermiteInterp(float ym1, float y0, float y1, float y2, float t)
{
    float c0 = y0;
    float c1 = (y1 - ym1) * 0.5f;
    float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * t + c2) * t + c1) * t + c0;
}

} // namespace AudioMath
} // namespace DSP
} // namespace Audio
} // namespace Prisma
