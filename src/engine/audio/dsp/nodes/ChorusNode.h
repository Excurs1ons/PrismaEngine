#pragma once

#include <audio/dsp/AudioNode.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class ChorusNode : public AudioNode {
public:
    ChorusNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("Chorus");
        SetParameter("rate", 0.5f);
        SetParameter("depth", 0.5f);
        SetParameter("delay", 15.0f);
        SetParameter("mix", 0.5f);
        SetParameter("voices", 3.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float rate = GetParameter("rate");
        float depth = std::clamp(GetParameter("depth"), 0.0f, 1.0f);
        float baseDelay = std::clamp(GetParameter("delay"), 5.0f, 30.0f);
        float mix = std::clamp(GetParameter("mix"), 0.0f, 1.0f);
        int voices = std::clamp(static_cast<int>(GetParameter("voices")), 1, 4);

        float maxDelayMs = baseDelay + depth * 15.0f;
        uint32_t maxSamples = static_cast<uint32_t>(maxDelayMs * ctx.sampleRate / 1000.0f) + 2;

        if (m_buffer.size() < maxSamples + ctx.framesPerBlock) {
            m_buffer.resize(maxSamples + ctx.framesPerBlock, 0.0f);
        }

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                m_buffer[m_writePos] = src[f];

                float dry = src[f];
                float wet = 0.0f;

                for (int v = 0; v < voices; ++v) {
                    float phaseOffset = (float)v / voices * (2.0f * M_PI);
                    float lfo = sinf(m_phase + phaseOffset);
                    float modDelay = (baseDelay + (lfo + 1.0f) * 0.5f * depth * 15.0f) / 1000.0f;
                    uint32_t readDelay = static_cast<uint32_t>(modDelay * ctx.sampleRate);

                    size_t readPos = (m_writePos + m_buffer.size() - readDelay) % m_buffer.size();
                    float frac = (modDelay * ctx.sampleRate) - readDelay;
                    size_t readPos2 = (readPos + 1) % m_buffer.size();
                    wet += m_buffer[readPos] + frac * (m_buffer[readPos2] - m_buffer[readPos]);
                }
                wet /= voices;

                m_writePos = (m_writePos + 1) % m_buffer.size();
                dst[f] = dry * (1.0f - mix) + wet * mix;
            }
        }

        m_phase += 2.0f * float(M_PI) * rate / ctx.sampleRate * frames;
        if (m_phase > 2.0f * float(M_PI)) m_phase -= 2.0f * float(M_PI);
    }

    void Reset() override {
        m_buffer.assign(m_buffer.size(), 0.0f);
        m_writePos = 0;
        m_phase = 0.0f;
    }

private:
    std::vector<float> m_buffer;
    size_t m_writePos = 0;
    float m_phase = 0.0f;
};

} // namespace Prisma::Audio::DSP
