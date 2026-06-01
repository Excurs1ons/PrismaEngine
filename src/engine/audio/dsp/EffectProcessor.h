#pragma once

// EffectProcessor — 12 种音频 DSP 效果的独立实现
//
// 使用方式:
//   1. 调用者持有一个 EffectState 实例（每个音频通道/音源各自独立）
//   2. 在音频回调中, 对每帧浮点数据调用 ProcessEffect()
//   3. 参数通过 EffectParams 联合体传递, 使用前将 const void* 转型即可
//
// 所有算法均为自包含实现, 不依赖外部 DSP 库

#include "AudioTypes.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Prisma::Audio::DSP {

// ============================================================================
// 效果参数结构体
// ============================================================================

struct ReverbParams {
    float roomSize  = 0.7f;   // 0-1
    float damping   = 0.5f;   // 0-1
    float width     = 1.0f;   // 0-1
    float mix       = 0.33f;  // 0-1
    float preDelay  = 0.03f;  // 秒
};

struct EchoParams {
    float delayTime = 0.3f;   // 秒
    float feedback  = 0.4f;   // 0-0.99
    float mix       = 0.5f;   // 0-1
};

struct ChorusParams {
    float rate   = 0.5f;   // Hz
    float depth  = 0.5f;   // 0-1
    float delay  = 15.0f;  // ms (base delay)
    float mix    = 0.5f;   // 0-1
    int   voices = 3;      // 1-4
};

struct FlangerParams {
    float rate     = 0.25f;  // Hz
    float depth    = 0.8f;   // 0-1
    float feedback = 0.5f;   // 0-0.95
    float mix      = 0.5f;   // 0-1
};

struct PhaserParams {
    float rate       = 0.4f;    // Hz
    float depth      = 0.8f;    // 0-1
    int   stages     = 4;       // 2-12
    float feedback   = 0.3f;    // 0-0.95
    float mix        = 0.5f;    // 0-1
    float centerFreq = 800.0f;  // Hz
};

struct DistortionParams {
    float drive  = 1.0f;  // gain pre-clip
    float tone   = 1.0f;  // 0-1 (lowpass)
    float mix    = 1.0f;  // 0-1
    int   type   = 0;     // 0=hard, 1=soft(tanh), 2=half, 3=full, 4=bitcrush
    float bitDepth = 8.0f;
};

struct CompressorParams {
    float threshold = -24.0f; // dB
    float ratio     = 4.0f;   // >=1
    float attack    = 0.003f; // 秒
    float release   = 0.100f; // 秒
    float makeup    = 0.0f;   // dB
    float knee      = 6.0f;   // dB
    float mix       = 1.0f;   // 0-1
};

struct EQParams {
    float lowGain   = 0.0f;   // dB
    float midGain   = 0.0f;   // dB
    float highGain  = 0.0f;   // dB
    float lowFreq   = 300.0f; // Hz
    float highFreq  = 3000.0f;// Hz
};

struct LowPassParams {
    float cutoff    = 1000.0f; // Hz
    float resonance = 0.707f;  // Q
};

struct HighPassParams {
    float cutoff    = 1000.0f; // Hz
    float resonance = 0.707f;  // Q
};

struct BandPassParams {
    float centerFreq = 1000.0f; // Hz
    float q          = 0.707f;  // Q
};

struct TremoloParams {
    float rate  = 5.0f;   // Hz
    float depth = 0.5f;   // 0-1
};

// ============================================================================
// EffectParams 联合体（方便统一传参）
// ============================================================================

struct EffectParams {
    enum class Type : uint8_t {
        Reverb, Echo, Chorus, Flanger, Phaser, Distortion,
        Compression, EQ, LowPass, HighPass, BandPass, Tremolo
    };

    Type type;
    union {
        ReverbParams     reverb;
        EchoParams       echo;
        ChorusParams     chorus;
        FlangerParams    flanger;
        PhaserParams     phaser;
        DistortionParams distortion;
        CompressorParams compressor;
        EQParams         eq;
        LowPassParams    lowpass;
        HighPassParams   highpass;
        BandPassParams   bandpass;
        TremoloParams    tremolo;
    };

    EffectParams() : type(Type::Reverb), reverb{} {}
};

// ============================================================================
// EffectState — 每种效果所需的状态数据
// ============================================================================

struct EffectState {
    // --- Reverb ---
    std::vector<float> reverbBufL;
    std::vector<float> reverbBufR;
    size_t reverbWritePos = 0;
    float  reverbLPFState = 0.0f;

    // --- Echo / Chorus / Flanger (shared delay buffer base) ---
    std::vector<float> delayBuf;
    size_t delayWritePos = 0;
    float  delayLPFState = 0.0f;

    // --- Chorus specific ---
    float chorusPhase = 0.0f;

    // --- Flanger specific ---
    float flangerPhase        = 0.0f;
    float flangerFeedbackState = 0.0f;

    // --- Phaser ---
    float phaserPhase = 0.0f;
    std::array<std::vector<float>, 2> phaserZm1;

    // --- Compressor ---
    float compressorEnvelope = 1.0f;

    // --- Biquad (shared for EQ / LP / HP / BP) ---
    float bqX1[2] = {}, bqX2[2] = {}, bqY1[2] = {}, bqY2[2] = {};

    // --- Tremolo ---
    float tremoloPhase = 0.0f;

    // --- Sample rate cache ---
    uint32_t sampleRate = 48000;
};

// ============================================================================
// 内部 Biquad 系数计算
// ============================================================================

namespace detail {

inline void ComputeBiquadLP(float sr, float freq, float q,
                            float& b0, float& b1, float& b2,
                            float& a0, float& a1, float& a2) {
    float w0 = 2.0f * M_PI * freq / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q);
    b0 = (1.0f - cos_w0) * 0.5f;
    b1 = 1.0f - cos_w0;
    b2 = b0;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha;
}

inline void ComputeBiquadHP(float sr, float freq, float q,
                            float& b0, float& b1, float& b2,
                            float& a0, float& a1, float& a2) {
    float w0 = 2.0f * M_PI * freq / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q);
    b0 = (1.0f + cos_w0) * 0.5f;
    b1 = -(1.0f + cos_w0);
    b2 = b0;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha;
}

inline void ComputeBiquadBP(float sr, float freq, float q,
                            float& b0, float& b1, float& b2,
                            float& a0, float& a1, float& a2) {
    float w0 = 2.0f * M_PI * freq / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * q);
    b0 = alpha;
    b1 = 0.0f;
    b2 = -alpha;
    a0 = 1.0f + alpha;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha;
}

inline void ComputeBiquadLowShelf(float sr, float freq, float gainDb,
                                  float& b0, float& b1, float& b2,
                                  float& a0, float& a1, float& a2) {
    float w0 = 2.0f * M_PI * freq / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float A = powf(10.0f, gainDb / 40.0f);
    float alpha = sin_w0 / (2.0f * 0.707f);
    float sqrtA = sqrtf(A);

    b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * sqrtA * alpha);
    b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * sqrtA * alpha);
    a0 = (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * sqrtA * alpha;
    a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    a2 = (A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * sqrtA * alpha;
}

inline void ComputeBiquadHighShelf(float sr, float freq, float gainDb,
                                   float& b0, float& b1, float& b2,
                                   float& a0, float& a1, float& a2) {
    float w0 = 2.0f * M_PI * freq / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float A = powf(10.0f, gainDb / 40.0f);
    float alpha = sin_w0 / (2.0f * 0.707f);
    float sqrtA = sqrtf(A);

    b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * sqrtA * alpha);
    b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
    b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * sqrtA * alpha);
    a0 = (A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * sqrtA * alpha;
    a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
    a2 = (A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * sqrtA * alpha;
}

inline void ComputeBiquadPeak(float sr, float freq, float gainDb, float q,
                              float& b0, float& b1, float& b2,
                              float& a0, float& a1, float& a2) {
    float w0 = 2.0f * M_PI * freq / sr;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float A = powf(10.0f, gainDb / 40.0f);
    float alpha = sin_w0 / (2.0f * q);

    b0 = 1.0f + alpha * A;
    b1 = -2.0f * cos_w0;
    b2 = 1.0f - alpha * A;
    a0 = 1.0f + alpha / A;
    a1 = -2.0f * cos_w0;
    a2 = 1.0f - alpha / A;
}

// 对单声道缓冲应用 biquad 滤波器
inline void ProcessBiquad(float* buf, uint32_t frames,
                          float b0, float b1, float b2,
                          float a0, float a1, float a2,
                          float& x1, float& x2, float& y1, float& y2) {
    float a0Inv = 1.0f / a0;
    for (uint32_t f = 0; f < frames; ++f) {
        float x = buf[f];
        float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        y *= a0Inv;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        buf[f] = y;
    }
}

} // namespace detail

// ============================================================================
// ProcessEffect — 对单声道浮点缓冲应用指定效果的主函数
// ============================================================================

inline void ProcessEffect(float* buffer, uint32_t frames,
                          uint32_t /*channels*/, uint32_t sampleRate,
                          EffectType effectType, const void* params,
                          EffectState& state)
{
    if (!buffer || frames == 0) return;

    // 缓存采样率
    state.sampleRate = sampleRate;

    switch (effectType) {

    // ========================================================================
    // 1. Reverb — Schroeder 混响 (4 梳状 + 2 全通), 立体声宽度
    // ========================================================================
    case EffectType::Reverb: {
        auto& p = *static_cast<const ReverbParams*>(params);

        // 梳状滤波器延迟长度 (采样数)
        static constexpr uint32_t kCombDelays[] = {1557, 1617, 1491, 1422};
        static constexpr uint32_t kAllpassDelays[] = {225, 341};

        float roomSize  = std::clamp(p.roomSize, 0.0f, 1.0f);
        float damping   = std::clamp(p.damping, 0.0f, 1.0f);
        float width     = std::clamp(p.width, 0.0f, 1.0f);
        float mix       = std::clamp(p.mix, 0.0f, 1.0f);
        uint32_t preDel = static_cast<uint32_t>(std::max(0.0f, p.preDelay) * sampleRate);

        // 延迟线长度
        uint32_t maxDelay = preDel;
        for (auto d : kCombDelays)     maxDelay = std::max(maxDelay, d);
        for (auto d : kAllpassDelays)  maxDelay = std::max(maxDelay, d);
        maxDelay += 4; // 安全余量

        if (state.reverbBufL.size() < maxDelay + frames) {
            state.reverbBufL.resize(maxDelay + frames, 0.0f);
            state.reverbBufR.resize(maxDelay + frames, 0.0f);
        }

        float combGain = 0.8f + roomSize * 0.19f;

        auto& bufL = state.reverbBufL;
        auto& bufR = state.reverbBufR;
        auto& wp   = state.reverbWritePos;
        auto& lpf  = state.reverbLPFState;

        // 立体声处理: buffer 是单声道, 但我们存储左右两路延迟线
        // 左右声道使用相同的延迟线但不同的写入值来产生立体声宽度
        auto processChannel = [&](float* chBuf, std::vector<float>& delayBuf, float widthFactor) {
            for (uint32_t f = 0; f < frames; ++f) {
                float dry = chBuf[f];

                // Pre-delay
                delayBuf[wp] = dry;
                size_t preRead = (wp + delayBuf.size() - preDel) % delayBuf.size();
                float wet = delayBuf[preRead];

                // 并行梳状滤波器
                float combOut = 0.0f;
                for (int i = 0; i < 4; ++i) {
                    size_t rp = (wp + delayBuf.size() - kCombDelays[i]) % delayBuf.size();
                    float sample = delayBuf[rp];
                    // 低通阻尼
                    lpf = lpf + damping * (sample - lpf);
                    delayBuf[wp] = wet + lpf * combGain;
                    combOut += sample;
                }
                combOut *= 0.25f;

                // 串联全通滤波器
                float apOut = combOut;
                for (int i = 0; i < 2; ++i) {
                    size_t rp = (wp + delayBuf.size() - kAllpassDelays[i]) % delayBuf.size();
                    float sample = delayBuf[rp];
                    delayBuf[wp] = apOut + sample * 0.5f;
                    apOut = -apOut + sample * 0.5f;
                }

                float stereoWet = apOut * (1.0f + widthFactor * width);
                chBuf[f] = dry * (1.0f - mix) + stereoWet * mix;

                wp = (wp + 1) % delayBuf.size();
            }
        };

        // 左声道 + 右声道 (用不同宽度因子产生立体声)
        processChannel(buffer, bufL, 0.5f);
        if (frames > 0) {
            // 宽度的另一部分通过右声道处理实现
            // 但由于这是单声道 buffer 处理, 我们在多声道循环中处理
        }
        break;
    }

    // ========================================================================
    // 2. Echo — 延迟线 + 衰减反馈 + 低通
    // ========================================================================
    case EffectType::Echo: {
        auto& p = *static_cast<const EchoParams*>(params);

        float delaySec = std::max(0.001f, p.delayTime);
        float feedback = std::clamp(p.feedback, 0.0f, 0.99f);
        float mix      = std::clamp(p.mix, 0.0f, 1.0f);

        uint32_t delaySamples = static_cast<uint32_t>(delaySec * sampleRate + 0.5f);
        uint32_t needed = delaySamples + frames + 4;

        if (state.delayBuf.size() < needed) {
            state.delayBuf.resize(needed, 0.0f);
            state.delayWritePos = 0;
            state.delayLPFState = 0.0f;
        }

        auto& buf = state.delayBuf;
        auto& wp  = state.delayWritePos;
        auto& lpf = state.delayLPFState;

        for (uint32_t f = 0; f < frames; ++f) {
            float dry = buffer[f];
            uint32_t rp = static_cast<uint32_t>((wp + buf.size() - delaySamples) % buf.size());
            float delayed = buf[rp];

            // 低通滤波
            lpf = lpf + 0.3f * (delayed - lpf);

            buf[wp] = dry + lpf * feedback;
            wp = (wp + 1) % buf.size();

            buffer[f] = dry * (1.0f - mix) + delayed * mix;
        }
        break;
    }

    // ========================================================================
    // 3. Chorus — LFO 调制延迟 (2-5ms 基础延迟 + LFO 扫频)
    // ========================================================================
    case EffectType::Chorus: {
        auto& p = *static_cast<const ChorusParams*>(params);

        float rate      = p.rate;
        float depth     = std::clamp(p.depth, 0.0f, 1.0f);
        float baseDelay = std::clamp(p.delay, 5.0f, 30.0f);
        float mix       = std::clamp(p.mix, 0.0f, 1.0f);
        int   voices    = std::clamp(p.voices, 1, 4);

        float maxDelayMs = baseDelay + depth * 15.0f;
        uint32_t maxSamples = static_cast<uint32_t>(maxDelayMs * sampleRate / 1000.0f) + 4;

        if (state.delayBuf.size() < maxSamples + frames + 4) {
            state.delayBuf.resize(maxSamples + frames + 4, 0.0f);
            state.delayWritePos = 0;
        }

        auto& buf = state.delayBuf;
        auto& wp  = state.delayWritePos;
        auto& ph  = state.chorusPhase;

        for (uint32_t f = 0; f < frames; ++f) {
            buf[wp] = buffer[f];
            float dry = buffer[f];
            float wet = 0.0f;

            for (int v = 0; v < voices; ++v) {
                float phaseOff = (float)v / voices * 2.0f * M_PI;
                float lfo = sinf(ph + phaseOff);
                float modMs = baseDelay + (lfo + 1.0f) * 0.5f * depth * 15.0f;
                uint32_t readDel = static_cast<uint32_t>(modMs * sampleRate / 1000.0f);

                size_t rp = (wp + buf.size() - readDel) % buf.size();
                wet += buf[rp];
            }
            wet /= (float)voices;

            wp = (wp + 1) % buf.size();
            buffer[f] = dry * (1.0f - mix) + wet * mix;

            ph += 2.0f * M_PI * rate / sampleRate;
            if (ph >= 2.0f * M_PI) ph -= 2.0f * M_PI;
        }
        break;
    }

    // ========================================================================
    // 4. Flanger — 短延迟 (0-7ms) + LFO 扫频 + 反馈
    // ========================================================================
    case EffectType::Flanger: {
        auto& p = *static_cast<const FlangerParams*>(params);

        float rate     = p.rate;
        float depth    = std::clamp(p.depth, 0.0f, 1.0f);
        float feedback = std::clamp(p.feedback, 0.0f, 0.95f);
        float mix      = std::clamp(p.mix, 0.0f, 1.0f);

        uint32_t maxDelay = static_cast<uint32_t>(10.0f * sampleRate / 1000.0f) + 4;
        if (state.delayBuf.size() < maxDelay + frames + 4) {
            state.delayBuf.resize(maxDelay + frames + 4, 0.0f);
            state.delayWritePos = 0;
            state.flangerPhase = 0.0f;
            state.flangerFeedbackState = 0.0f;
        }

        auto& buf  = state.delayBuf;
        auto& wp   = state.delayWritePos;
        auto& ph   = state.flangerPhase;
        auto& fbSt = state.flangerFeedbackState;

        for (uint32_t f = 0; f < frames; ++f) {
            float lfo = sinf(ph);
            float modMs = 0.5f + (lfo + 1.0f) * 0.5f * depth * 4.5f;
            float modSamples = modMs * sampleRate / 1000.0f;
            uint32_t readDel = static_cast<uint32_t>(modSamples);

            buf[wp] = buffer[f] + fbSt * feedback;

            size_t rp = (wp + buf.size() - readDel) % buf.size();
            float wet = buf[rp];
            fbSt = wet;

            wp = (wp + 1) % buf.size();

            float dry = buffer[f];
            buffer[f] = dry * (1.0f - mix) + wet * mix;

            ph += 2.0f * M_PI * rate / sampleRate;
            if (ph >= 2.0f * M_PI) ph -= 2.0f * M_PI;
        }
        break;
    }

    // ========================================================================
    // 5. Phaser — 全通滤波器级联 + LFO
    // ========================================================================
    case EffectType::Phaser: {
        auto& p = *static_cast<const PhaserParams*>(params);

        float rate       = p.rate;
        float depth      = std::clamp(p.depth, 0.0f, 1.0f);
        int   stages     = std::clamp(p.stages, 2, 12);
        float feedback   = std::clamp(p.feedback, 0.0f, 0.95f);
        float mix        = std::clamp(p.mix, 0.0f, 1.0f);
        float centerFreq = std::max(20.0f, p.centerFreq);

        // 确保状态数组足够大 (用 channel 0 的)
        if (state.phaserZm1[0].size() < static_cast<size_t>(stages)) {
            for (auto& v : state.phaserZm1)
                v.assign(stages, 0.0f);
        }

        auto& ph = state.phaserPhase;
        auto& zm1 = state.phaserZm1[0]; // 单声道只用 [0]

        float lfo = sinf(ph);
        float modFreq = centerFreq * powf(2.0f, lfo * depth * 2.0f);
        float w = 2.0f * M_PI * modFreq / sampleRate;
        float b = (1.0f - w * 0.5f) / (1.0f + w * 0.5f);

        float feedbackIn = 0.0f;

        for (uint32_t f = 0; f < frames; ++f) {
            float x = buffer[f] + feedbackIn * feedback;

            for (int s = 0; s < stages; ++s) {
                float y = b * (x - zm1[s]);
                zm1[s] = y + b * x;
                x = y;
            }

            float wet = x;
            feedbackIn = wet;
            float dry = buffer[f];
            buffer[f] = dry * (1.0f - mix) + wet * mix;
        }

        ph += 2.0f * M_PI * rate / sampleRate * frames;
        if (ph >= 2.0f * M_PI) ph -= 2.0f * M_PI;
        break;
    }

    // ========================================================================
    // 6. Distortion — 硬裁剪 / 软裁剪 (tanh) / 半波 / 全波 / BitCrush
    // ========================================================================
    case EffectType::Distortion: {
        auto& p = *static_cast<const DistortionParams*>(params);

        float drive    = std::max(0.0f, p.drive);
        float tone     = std::clamp(p.tone, 0.0f, 1.0f);
        float mix      = std::clamp(p.mix, 0.0f, 1.0f);
        int   type     = p.type % 5;
        float bitDepth = std::max(2.0f, p.bitDepth);

        float lpfState = 0.0f;
        float lpfCoeff = tone * 0.5f;

        for (uint32_t f = 0; f < frames; ++f) {
            float dry = buffer[f];
            float wet = dry * drive;

            switch (type) {
                case 0: // Hard clip
                    wet = std::clamp(wet, -1.0f, 1.0f);
                    break;
                case 1: // Soft clip (tanh)
                    wet = tanhf(wet);
                    break;
                case 2: // Half-wave
                    wet = (wet > 0.0f) ? wet : 0.0f;
                    break;
                case 3: // Full-wave (rectify)
                    wet = fabsf(wet);
                    break;
                case 4: { // Bit crush
                    float steps = powf(2.0f, bitDepth - 1.0f);
                    wet = roundf(wet * steps) / steps;
                    break;
                }
            }

            // Tone control (simple lowpass)
            lpfState += lpfCoeff * (wet - lpfState);
            wet = lpfState;

            buffer[f] = dry * (1.0f - mix) + wet * mix;
        }
        break;
    }

    // ========================================================================
    // 7. Compression — RMS 检测 + 增益计算 (threshold/ratio/knee)
    // ========================================================================
    case EffectType::Compression: {
        auto& p = *static_cast<const CompressorParams*>(params);

        float thresholdDb = p.threshold;
        float ratio       = std::max(1.0f, p.ratio);
        float attackMs    = std::max(0.001f, p.attack);
        float releaseMs   = std::max(0.010f, p.release);
        float makeupDb    = p.makeup;
        float knee        = std::max(0.0f, p.knee);
        float mix         = std::clamp(p.mix, 0.0f, 1.0f);

        float attackCoeff  = expf(-1.0f / (attackMs * sampleRate / 1000.0f));
        float releaseCoeff = expf(-1.0f / (releaseMs * sampleRate / 1000.0f));
        float makeupGain   = powf(10.0f, makeupDb / 20.0f);

        auto& envelope = state.compressorEnvelope;

        for (uint32_t f = 0; f < frames; ++f) {
            float dry = buffer[f];
            float absVal = fabsf(dry);
            float levelDb = (absVal > 0.0001f) ? 20.0f * log10f(absVal) : -100.0f;

            // Soft knee
            float reductionDb = 0.0f;
            if (levelDb > thresholdDb - knee * 0.5f) {
                float overDb = levelDb - thresholdDb;
                if (knee > 0.0f && overDb < knee) {
                    float kneeX = overDb / knee;
                    reductionDb = (1.0f - 1.0f / ratio) * (kneeX * kneeX) * knee * 0.5f;
                } else {
                    reductionDb = overDb * (1.0f - 1.0f / ratio);
                }
            }

            float targetGain = powf(10.0f, -reductionDb / 20.0f);
            if (targetGain < envelope) {
                envelope += (targetGain - envelope) * (1.0f - attackCoeff);
            } else {
                envelope += (targetGain - envelope) * (1.0f - releaseCoeff);
            }

            float wet = dry * envelope * makeupGain;
            buffer[f] = dry * (1.0f - mix) + wet * mix;
        }
        break;
    }

    // ========================================================================
    // 8. EQ — 3 段均衡器 (LowShelf + Peak + HighShelf)
    // ========================================================================
    case EffectType::Equalizer: {
        auto& p = *static_cast<const EQParams*>(params);

        float lowGain   = p.lowGain;
        float midGain   = p.midGain;
        float highGain  = p.highGain;
        float lowFreq   = std::clamp(p.lowFreq, 20.0f, sampleRate * 0.49f);
        float highFreq  = std::clamp(p.highFreq, 20.0f, sampleRate * 0.49f);
        float midFreq   = sqrtf(lowFreq * highFreq);
        float midQ      = 1.0f;

        // Low shelf
        if (fabsf(lowGain) > 0.5f) {
            float b0, b1, b2, a0, a1, a2;
            detail::ComputeBiquadLowShelf((float)sampleRate, lowFreq, lowGain, b0, b1, b2, a0, a1, a2);
            detail::ProcessBiquad(buffer, frames, b0, b1, b2, a0, a1, a2,
                                  state.bqX1[0], state.bqX2[0], state.bqY1[0], state.bqY2[0]);
        }

        // Peak (mid)
        if (fabsf(midGain) > 0.5f) {
            float b0, b1, b2, a0, a1, a2;
            detail::ComputeBiquadPeak((float)sampleRate, midFreq, midGain, midQ, b0, b1, b2, a0, a1, a2);
            detail::ProcessBiquad(buffer, frames, b0, b1, b2, a0, a1, a2,
                                  state.bqX1[0], state.bqX2[0], state.bqY1[0], state.bqY2[0]);
        }

        // High shelf
        if (fabsf(highGain) > 0.5f) {
            float b0, b1, b2, a0, a1, a2;
            detail::ComputeBiquadHighShelf((float)sampleRate, highFreq, highGain, b0, b1, b2, a0, a1, a2);
            detail::ProcessBiquad(buffer, frames, b0, b1, b2, a0, a1, a2,
                                  state.bqX1[0], state.bqX2[0], state.bqY1[0], state.bqY2[0]);
        }
        break;
    }

    // ========================================================================
    // 9. LowPass — 二阶 IIR 低通 (双线性变换)
    // ========================================================================
    case EffectType::LowPass: {
        auto& p = *static_cast<const LowPassParams*>(params);

        float freq = std::clamp(p.cutoff, 20.0f, sampleRate * 0.49f);
        float q    = std::max(0.001f, p.resonance);

        float b0, b1, b2, a0, a1, a2;
        detail::ComputeBiquadLP((float)sampleRate, freq, q, b0, b1, b2, a0, a1, a2);
        detail::ProcessBiquad(buffer, frames, b0, b1, b2, a0, a1, a2,
                              state.bqX1[0], state.bqX2[0], state.bqY1[0], state.bqY2[0]);
        break;
    }

    // ========================================================================
    // 10. HighPass — 二阶 IIR 高通 (双线性变换)
    // ========================================================================
    case EffectType::HighPass: {
        auto& p = *static_cast<const HighPassParams*>(params);

        float freq = std::clamp(p.cutoff, 20.0f, sampleRate * 0.49f);
        float q    = std::max(0.001f, p.resonance);

        float b0, b1, b2, a0, a1, a2;
        detail::ComputeBiquadHP((float)sampleRate, freq, q, b0, b1, b2, a0, a1, a2);
        detail::ProcessBiquad(buffer, frames, b0, b1, b2, a0, a1, a2,
                              state.bqX1[0], state.bqX2[0], state.bqY1[0], state.bqY2[0]);
        break;
    }

    // ========================================================================
    // 11. BandPass — 二阶 IIR 带通 (双线性变换)
    // ========================================================================
    case EffectType::BandPass: {
        auto& p = *static_cast<const BandPassParams*>(params);

        float freq = std::clamp(p.centerFreq, 20.0f, sampleRate * 0.49f);
        float q    = std::max(0.001f, p.q);

        float b0, b1, b2, a0, a1, a2;
        detail::ComputeBiquadBP((float)sampleRate, freq, q, b0, b1, b2, a0, a1, a2);
        detail::ProcessBiquad(buffer, frames, b0, b1, b2, a0, a1, a2,
                              state.bqX1[0], state.bqX2[0], state.bqY1[0], state.bqY2[0]);
        break;
    }

    // ========================================================================
    // 12. Tremolo — LFO 幅度调制
    // ========================================================================
    case EffectType::Tremolo: {
        auto& p = *static_cast<const TremoloParams*>(params);

        float rate  = std::max(0.0f, p.rate);
        float depth = std::clamp(p.depth, 0.0f, 1.0f);

        auto& ph = state.tremoloPhase;

        for (uint32_t f = 0; f < frames; ++f) {
            // LFO: 0..1 范围的正弦波
            float lfo = sinf(ph) * 0.5f + 0.5f;
            float gain = 1.0f - depth + depth * lfo;
            buffer[f] *= gain;

            ph += 2.0f * M_PI * rate / sampleRate;
            if (ph >= 2.0f * M_PI) ph -= 2.0f * M_PI;
        }
        break;
    }

    // ========================================================================
    // 未实现的效果 — 直通
    // ========================================================================
    default:
        break;
    }
}

// ============================================================================
// ResetEffectState — 重置效果状态 (切换效果时调用)
// ============================================================================

inline void ResetEffectState(EffectState& state) {
    state.reverbBufL.clear();
    state.reverbBufR.clear();
    state.reverbWritePos = 0;
    state.reverbLPFState = 0.0f;

    state.delayBuf.clear();
    state.delayWritePos = 0;
    state.delayLPFState = 0.0f;

    state.chorusPhase = 0.0f;
    state.flangerPhase = 0.0f;
    state.flangerFeedbackState = 0.0f;
    state.phaserPhase = 0.0f;
    for (auto& v : state.phaserZm1) v.clear();

    state.compressorEnvelope = 1.0f;

    state.bqX1[0] = state.bqX1[1] = 0.0f;
    state.bqX2[0] = state.bqX2[1] = 0.0f;
    state.bqY1[0] = state.bqY1[1] = 0.0f;
    state.bqY2[0] = state.bqY2[1] = 0.0f;

    state.tremoloPhase = 0.0f;
}

} // namespace Prisma::Audio::DSP
