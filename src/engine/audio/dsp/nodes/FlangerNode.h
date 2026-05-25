#pragma once

#include <audio/dsp/AudioNode.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class FlangerNode : public AudioNode {
public:
    FlangerNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("Flanger");
        SetParameter("rate", 0.25f);
        SetParameter("depth", 0.8f);
        SetParameter("feedback", 0.5f);
        SetParameter("mix", 0.5f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float rate = GetParameter("rate");
        float depth = std::clamp(GetParameter("depth"), 0.0f, 1.0f);
        float feedback = std::clamp(GetParameter("feedback"), 0.0f, 0.95f);
        float mix = std::clamp(GetParameter("mix"), 0.0f, 1.0f);

        uint32_t maxDelay = static_cast<uint32_t>(10.0f * ctx.sampleRate / 1000.0f) + 2;
        if (m_buffer.size() < maxDelay + ctx.framesPerBlock) {
            m_buffer.resize(maxDelay + ctx.framesPerBlock, 0.0f);
        }

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                float lfo = sinf(m_phase + (c > 0 ? 1.57f : 0.0f));
                float modDelayMs = 0.5f + (lfo + 1.0f) * 0.5f * depth * 4.5f;
                float modDelaySamples = modDelayMs * ctx.sampleRate / 1000.0f;
                uint32_t readDelay = static_cast<uint32_t>(modDelaySamples);
                float frac = modDelaySamples - readDelay;

                m_buffer[m_writePos] = src[f] + m_feedbackState * feedback;

                size_t readPos = (m_writePos + m_buffer.size() - readDelay) % m_buffer.size();
                size_t readPos2 = (readPos + 1) % m_buffer.size();
                float wet = m_buffer[readPos] + frac * (m_buffer[readPos2] - m_buffer[readPos]);
                m_feedbackState = wet;

                m_writePos = (m_writePos + 1) % m_buffer.size();

                float dry = src[f];
                dst[f] = dry * (1.0f - mix) + wet * mix;
            }
        }

        m_phase += 2.0f * M_PI * rate / ctx.sampleRate * frames;
        if (m_phase > 2.0f * M_PI) m_phase -= 2.0f * M_PI;
    }

    void Reset() override {
        m_buffer.assign(m_buffer.size(), 0.0f);
        m_writePos = 0;
        m_phase = 0.0f;
        m_feedbackState = 0.0f;
    }

private:
    std::vector<float> m_buffer;
    size_t m_writePos = 0;
    float m_phase = 0.0f;
    float m_feedbackState = 0.0f;
};

} // namespace Prisma::Audio::DSP
