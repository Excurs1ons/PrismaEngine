#pragma once
#include <map>
#include <DirectXMath.h>
#include <vector>
using namespace DirectX;
// 基础音频接口
class AudioGenerator {
public:
    virtual ~AudioGenerator() = default;
    virtual void GenerateSamples(float* buffer, size_t samples) = 0;
    virtual void SetFrequency(float freq) = 0;
    virtual void SetVolume(float vol) = 0;
};

// 基础波形生成器
class WaveformGenerator : public AudioGenerator {
protected:
    float frequency;
    float volume;
    float phase;
    float sampleRate;

public:
    WaveformGenerator(float sampleRate = 44100.0f)
        : sampleRate(sampleRate), frequency(440.0f), volume(0.5f), phase(0.0f) {
    }

    void SetFrequency(float freq) override { frequency = freq; }
    void SetVolume(float vol) override { volume = vol; }

protected:
    float AdvancePhase(size_t samples = 1) {
        phase += XM_2PI * frequency / sampleRate * samples;
        while (phase >= XM_2PI) phase -= XM_2PI;
        return phase;
    }
};

// 正弦波生成器
class SineWaveGenerator : public WaveformGenerator {
public:
    void GenerateSamples(float* buffer, size_t sampleCount) override {
        for (size_t i = 0; i < sampleCount; ++i) {
            buffer[i] = XMScalarSin(AdvancePhase()) * volume;
        }
    }
};

// 方波生成器（类似FC）
class SquareWaveGenerator : public WaveformGenerator {
private:
    float dutyCycle; // 占空比 0.0-1.0

public:
    SquareWaveGenerator(float sampleRate = 44100.0f)
        : WaveformGenerator(sampleRate), dutyCycle(0.5f) {
    }

    void SetDutyCycle(float duty) { dutyCycle = duty; }

    void GenerateSamples(float* buffer, size_t sampleCount) override {
        float phaseIncrement = 2.0f * XM_PI * frequency / sampleRate;

        for (size_t i = 0; i < sampleCount; ++i) {
            // 方波：相位在0-2π范围内，根据占空比决定输出
            buffer[i] = (phase < XM_2PI * dutyCycle) ? volume : -volume;
            phase += phaseIncrement;
            if (phase >= XM_2PI) phase -= XM_2PI;
        }
    }
};

// 三角波生成器
class TriangleWaveGenerator : public WaveformGenerator {
public:
    TriangleWaveGenerator(float sampleRate = 44100.0f)
        : WaveformGenerator(sampleRate) {
    }
    void GenerateSamples(float* buffer, size_t sampleCount) override {
        float phaseIncrement = XM_2PI * frequency / sampleRate;

        for (size_t i = 0; i < sampleCount; ++i) {
            // 三角波：使用绝对值函数创建
            float normalizedPhase = phase / XM_2PI;
            buffer[i] = (2.0f * std::abs(2.0f * normalizedPhase - 1.0f) - 1.0f) * volume;
            phase += phaseIncrement;
            if (phase >= XM_2PI) phase -= XM_2PI;
        }
    }
};

// 噪声生成器（类似FC的噪声通道）
class NoiseGenerator : public WaveformGenerator {
private:
    uint32_t lfsr; // 线性反馈移位寄存器
    uint32_t period;

public:
    NoiseGenerator(float sampleRate = 44100.0f)
        : WaveformGenerator(sampleRate), lfsr(1), period(1) {
    }

    void SetNoisePeriod(uint32_t p) { period = p; }

    void GenerateSamples(float* buffer, size_t sampleCount) override {
        for (size_t i = 0; i < sampleCount; ++i) {
            // LFSR算法生成伪随机噪声
            if ((lfsr & 1) == 1) {
                lfsr = (lfsr >> 1) ^ 0x6000; // 不同的多项式可以产生不同特性的噪声
                buffer[i] = volume;
            }
            else {
                lfsr >>= 1;
                buffer[i] = -volume;
            }
        }
    }
};
class SoundSynthesizer
{
private:
    // 仿NES，有5个声音通道：2个方波，1个三角波，1个噪声，1个PCM
    SquareWaveGenerator pulse1;
    SquareWaveGenerator pulse2;
    TriangleWaveGenerator triangle;
    NoiseGenerator noise;

    // 包络和效果
    struct Envelope {
        float attackTime;
        float decayTime;
        float sustainLevel;
        float releaseTime;
        float currentLevel;
        enum State { ATTACK, DECAY, SUSTAIN, RELEASE } state;
    };

    std::map<uint32_t, Envelope> envelopes;
public:
    SoundSynthesizer(float sampleRate)
        : pulse1(sampleRate), pulse2(sampleRate),
        triangle(sampleRate), noise(sampleRate) {
        // 设置FC典型的参数
        pulse1.SetDutyCycle(0.125f); // 12.5% 占空比
        pulse2.SetDutyCycle(0.25f);  // 25% 占空比
    }
    void generateSamples(float* buffer, size_t sampleCount) {
        std::vector<float> mixBuffer(sampleCount, 0.0f);
        std::vector<float> tempBuffer(sampleCount);

        // 混合所有通道
        pulse1.GenerateSamples(tempBuffer.data(), sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) mixBuffer[i] += tempBuffer[i] * 0.3f;

        pulse2.GenerateSamples(tempBuffer.data(), sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) mixBuffer[i] += tempBuffer[i] * 0.3f;

        triangle.GenerateSamples(tempBuffer.data(), sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) mixBuffer[i] += tempBuffer[i] * 0.2f;

        noise.GenerateSamples(tempBuffer.data(), sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) mixBuffer[i] += tempBuffer[i] * 0.2f;

        // 应用主音量控制和限制
        for (size_t i = 0; i < sampleCount; ++i) {
            buffer[i] = std::max(-1.0f, std::min(1.0f, mixBuffer[i]));
        }
    }

    // 音符控制
    void NoteOn(int channel, float frequency) {
        switch (channel) {
        case 0: pulse1.SetFrequency(frequency); break;
        case 1: pulse2.SetFrequency(frequency); break;
        case 2: triangle.SetFrequency(frequency); break;
        }
    }

    void NoteOff(int channel) {
        // 实现释放逻辑
    }
};

