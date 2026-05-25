#include "SoundSynthesizer.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <random>

using namespace Prisma::Audio;

static void WriteSampleInt16(std::vector<uint8_t>& data, int16_t sample) {
    size_t offset = data.size();
    data.resize(offset + sizeof(int16_t));
    data[offset + 0] = static_cast<uint8_t>(sample & 0xFF);
    data[offset + 1] = static_cast<uint8_t>((sample >> 8) & 0xFF);
}

static int16_t FloatToInt16(float sample) {
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;
    return static_cast<int16_t>(sample * 32767.0f);
}

// ========== 正弦波 ==========

AudioClip SoundSynthesizer::generateSine(float frequency, float duration, uint32_t sampleRate) {
    AudioClip clip;
    clip.format.sampleRate = sampleRate;
    clip.format.channels = 1;
    clip.format.bitsPerSample = 16;

    size_t totalSamples = static_cast<size_t>(sampleRate * duration);
    clip.data.reserve(totalSamples * sizeof(int16_t));

    for (size_t i = 0; i < totalSamples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float sample = std::sin(2.0f * XM_PI * frequency * t);
        WriteSampleInt16(clip.data, FloatToInt16(sample));
    }

    clip.duration = duration;
    return clip;
}

// ========== 方波 ==========

AudioClip SoundSynthesizer::generateSquare(float frequency, float duration, uint32_t sampleRate) {
    AudioClip clip;
    clip.format.sampleRate = sampleRate;
    clip.format.channels = 1;
    clip.format.bitsPerSample = 16;

    size_t totalSamples = static_cast<size_t>(sampleRate * duration);
    clip.data.reserve(totalSamples * sizeof(int16_t));

    float period = static_cast<float>(sampleRate) / frequency;

    for (size_t i = 0; i < totalSamples; ++i) {
        float phase = std::fmod(static_cast<float>(i), period) / period;
        float sample = (phase < 0.5f) ? 1.0f : -1.0f;
        WriteSampleInt16(clip.data, FloatToInt16(sample));
    }

    clip.duration = duration;
    return clip;
}

// ========== 锯齿波 ==========

AudioClip SoundSynthesizer::generateSawtooth(float frequency, float duration, uint32_t sampleRate) {
    AudioClip clip;
    clip.format.sampleRate = sampleRate;
    clip.format.channels = 1;
    clip.format.bitsPerSample = 16;

    size_t totalSamples = static_cast<size_t>(sampleRate * duration);
    clip.data.reserve(totalSamples * sizeof(int16_t));

    float period = static_cast<float>(sampleRate) / frequency;

    for (size_t i = 0; i < totalSamples; ++i) {
        float phase = std::fmod(static_cast<float>(i), period) / period;
        float sample = 2.0f * phase - 1.0f;
        WriteSampleInt16(clip.data, FloatToInt16(sample));
    }

    clip.duration = duration;
    return clip;
}

// ========== 三角波 ==========

AudioClip SoundSynthesizer::generateTriangle(float frequency, float duration, uint32_t sampleRate) {
    AudioClip clip;
    clip.format.sampleRate = sampleRate;
    clip.format.channels = 1;
    clip.format.bitsPerSample = 16;

    size_t totalSamples = static_cast<size_t>(sampleRate * duration);
    clip.data.reserve(totalSamples * sizeof(int16_t));

    float period = static_cast<float>(sampleRate) / frequency;

    for (size_t i = 0; i < totalSamples; ++i) {
        float phase = std::fmod(static_cast<float>(i), period) / period;
        float sample = 4.0f * std::abs(phase - 0.5f) - 1.0f;
        WriteSampleInt16(clip.data, FloatToInt16(sample));
    }

    clip.duration = duration;
    return clip;
}

// ========== 白噪声 ==========

AudioClip SoundSynthesizer::generateNoise(float duration, uint32_t sampleRate) {
    AudioClip clip;
    clip.format.sampleRate = sampleRate;
    clip.format.channels = 1;
    clip.format.bitsPerSample = 16;

    size_t totalSamples = static_cast<size_t>(sampleRate * duration);
    clip.data.reserve(totalSamples * sizeof(int16_t));

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (size_t i = 0; i < totalSamples; ++i) {
        WriteSampleInt16(clip.data, FloatToInt16(dist(rng)));
    }

    clip.duration = duration;
    return clip;
}

// ========== ADSR 包络 ==========

void SoundSynthesizer::generateWithADSR(AudioClip& clip, float attackTime, float decayTime,
                                          float sustainLevel, float releaseTime) {
    if (!clip.IsValid()) return;
    if (clip.format.bitsPerSample != 16) return;

    size_t sampleCount = clip.GetSampleCount();
    size_t totalFrames = clip.GetFrameCount();
    float sampleRate = static_cast<float>(clip.format.sampleRate);

    size_t attackFrames  = static_cast<size_t>(attackTime * sampleRate);
    size_t decayFrames   = static_cast<size_t>(decayTime * sampleRate);
    size_t releaseFrames = static_cast<size_t>(releaseTime * sampleRate);

    // Sustain length = remaining frames
    size_t sustainFrames = 0;
    if (attackFrames + decayFrames + releaseFrames < totalFrames) {
        sustainFrames = totalFrames - attackFrames - decayFrames - releaseFrames;
    } else {
        // Not enough frames for full envelope — clamp to available
        float totalEnv = attackTime + decayTime + releaseTime;
        if (totalEnv > 0.0f) {
            float ratio = clip.duration / totalEnv;
            attackFrames  = static_cast<size_t>(attackTime  * sampleRate * ratio);
            decayFrames   = static_cast<size_t>(decayTime   * sampleRate * ratio);
            releaseFrames = static_cast<size_t>(releaseTime * sampleRate * ratio);
            if (attackFrames + decayFrames + releaseFrames > totalFrames) {
                attackFrames  = totalFrames / 3;
                decayFrames   = totalFrames / 3;
                releaseFrames = totalFrames - attackFrames - decayFrames;
            }
        }
        sustainFrames = 0;
    }

    int16_t* samples = reinterpret_cast<int16_t*>(clip.data.data());

    for (size_t f = 0; f < totalFrames; ++f) {
        float envelope = 0.0f;

        if (f < attackFrames) {
            // Attack: linear 0→1
            envelope = static_cast<float>(f) / static_cast<float>(attackFrames);
        } else if (f < attackFrames + decayFrames) {
            // Decay: linear 1→sustainLevel
            float decayPos = static_cast<float>(f - attackFrames) / static_cast<float>(decayFrames);
            envelope = 1.0f - (1.0f - sustainLevel) * decayPos;
        } else if (f < attackFrames + decayFrames + sustainFrames) {
            // Sustain
            envelope = sustainLevel;
        } else {
            // Release: linear sustainLevel→0
            size_t relPos = f - (attackFrames + decayFrames + sustainFrames);
            if (relPos < releaseFrames) {
                envelope = sustainLevel * (1.0f - static_cast<float>(relPos) / static_cast<float>(releaseFrames));
            } else {
                envelope = 0.0f;
            }
        }

        // Apply envelope to all channels at this frame
        for (uint16_t ch = 0; ch < clip.format.channels; ++ch) {
            size_t idx = f * clip.format.channels + ch;
            samples[idx] = FloatToInt16(static_cast<float>(samples[idx]) / 32767.0f * envelope);
        }
    }
}

// ========== 通用音调生成 ==========

AudioClip SoundSynthesizer::generateTone(float frequency, float duration, Waveform waveform) {
    const uint32_t sampleRate = 44100;

    switch (waveform) {
        case Waveform::Sine:
            return generateSine(frequency, duration, sampleRate);
        case Waveform::Square:
            return generateSquare(frequency, duration, sampleRate);
        case Waveform::Sawtooth:
            return generateSawtooth(frequency, duration, sampleRate);
        case Waveform::Triangle:
            return generateTriangle(frequency, duration, sampleRate);
        case Waveform::Noise:
            return generateNoise(duration, sampleRate);
        default:
            return generateSine(frequency, duration, sampleRate);
    }
}


