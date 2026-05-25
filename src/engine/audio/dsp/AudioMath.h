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

/// @brief Convert decibels to linear gain scale.
/// @param db  Gain in decibels.
/// @return    10^(db/20)
inline constexpr float DB_TO_LINEAR(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

/// @brief Convert linear gain to decibels.
/// @param linear  Linear gain value.
/// @return        20*log10(linear), or -144 dB when linear <= 0.
inline constexpr float LINEAR_TO_DB(float linear)
{
    return linear <= 0.0f ? -144.0f : 20.0f * std::log10(linear);
}

/// @brief  Linear gain ramp for per-frame interpolation.
/// @param startGain  Gain at the start of the ramp.
/// @param endGain    Gain at the end of the ramp.
/// @param frames     Total number of frames in the ramp.
/// @param index      Current frame index [0, frames-1].
/// @return           Linearly interpolated gain.
inline float GainRamp(float startGain, float endGain, uint32_t frames, uint32_t index)
{
    if (frames <= 1) return endGain;
    float t = static_cast<float>(index) / static_cast<float>(frames - 1);
    return startGain + (endGain - startGain) * t;
}

// ======================================================================
// Pan (sine / cosine law)
// ======================================================================

/// @brief  Calculate left/right channel gains from a pan position.
/// @param panPos    Pan position: -1.0 (full left), 0.0 (center), +1.0 (full right).
/// @param leftGain  [out] Left channel gain.
/// @param rightGain [out] Right channel gain.
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

/// @brief  Convert frequency (Hz) to MIDI note number.
/// @param freq  Frequency in Hertz.
/// @return      MIDI note number (69 = A4 = 440 Hz).
inline float FreqToMIDI(float freq)
{
    if (freq <= 0.0f) return 0.0f;
    return 69.0f + 12.0f * std::log2(freq / 440.0f);
}

/// @brief  Convert MIDI note number to frequency (Hz).
/// @param midiNote  MIDI note number (69 = A4 = 440 Hz).
/// @return          Frequency in Hertz.
inline float MIDIToFreq(float midiNote)
{
    return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
}

// ======================================================================
// Ramp / envelope helper shapes
// ======================================================================

/// @brief  Linear ramp, t clamped to [0, 1].
inline constexpr float LinearRamp(float t)
{
    return std::clamp(t, 0.0f, 1.0f);
}

/// @brief  Exponential ramp with decay constant k=5.
///         Provides a fast-then-gentle curve suitable for envelope release.
///         y(t) = (1 - exp(-t*5)) / (1 - exp(-5))
inline float ExpRamp(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    constexpr float k = 5.0f;
    return (1.0f - std::exp(-t * k)) / (1.0f - std::exp(-k));
}

/// @brief  Smooth Hermite-style step: t * t * (3 - 2 * t).
///         Also known as "smoothstep". t is clamped to [0, 1].
inline constexpr float SmoothStep(float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// ======================================================================
// Clipping / waveshaping
// ======================================================================

/// @brief  Hard clip (clamp) a sample to [-threshold, +threshold].
inline constexpr float HardClip(float sample, float threshold)
{
    return std::clamp(sample, -threshold, threshold);
}

/// @brief  Soft clipping with hyperbolic tangent curve.
///         Smooth, musical saturation as sample approaches threshold.
///         y = threshold * tanh(sample / threshold)
inline float SoftClip(float sample, float threshold)
{
    return threshold * std::tanh(sample / threshold);
}

// ======================================================================
// Interpolation
// ======================================================================

/// @brief  Standard linear interpolation between two samples.
inline constexpr float LinearInterp(float y0, float y1, float t)
{
    return y0 + (y1 - y0) * t;
}

/// @brief  4-point, 3rd-order Hermite (Catmull-Rom) interpolation.
/// @param ym1  Sample at index -1 (previous).
/// @param y0   Sample at index  0 (current).
/// @param y1   Sample at index +1 (next).
/// @param y2   Sample at index +2 (next-next).
/// @param t    Interpolation factor [0, 1].
/// @return     Smoothly interpolated value.
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
