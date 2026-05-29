#pragma once

#include <audio/dsp/AudioNode.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class DelayNode : public AudioNode {
public:
    DelayNode(uint32_t maxDelayMs = 5000) {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("Delay");
        SetParameter("delayTime", 0.2f);
        SetParameter("feedback", 0.3f);
        SetParameter("mix", 0.5f);
        SetParameter("lowpass", 0.7f);
        SetParameter("stereoSpread", 0.0f);
        m_maxDelaySamples = static_cast<uint32_t>(maxDelayMs * 48.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float delaySec = std::max(0.001f, GetParameter("delayTime"));
        float feedback = std::clamp(GetParameter("feedback"), 0.0f, 0.99f);
        float mix = std::clamp(GetParameter("mix"), 0.0f, 1.0f);
        float lpf = std::clamp(GetParameter("lowpass"), 0.0f, 0.999f);
        float spread = std::clamp(GetParameter("stereoSpread"), 0.0f, 0.02f);

        uint32_t delaySamples = static_cast<uint32_t>(delaySec * ctx.sampleRate + 0.5f);
        uint32_t spreadSamples = static_cast<uint32_t>(spread * ctx.sampleRate + 0.5f);

        uint32_t neededSize = delaySamples + spreadSamples + ctx.framesPerBlock;
        if (m_buffer.size() < neededSize) {
            m_buffer.resize(neededSize, 0.0f);
            m_writePos = 0;
        }

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            uint32_t chDelay = delaySamples + (c > 0 ? spreadSamples : 0);
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                float dry = src[f];

                uint32_t readPos = static_cast<uint32_t>((m_writePos + m_buffer.size() - chDelay) % m_buffer.size());
                float delayed = m_buffer[readPos];

                m_lpfState = m_lpfState + lpf * (delayed - m_lpfState);
                float wetSignal = m_lpfState;

                m_buffer[m_writePos] = dry + wetSignal * feedback;
                m_writePos = (m_writePos + 1) % m_buffer.size();

                dst[f] = dry * (1.0f - mix) + wetSignal * mix;
            }
        }
    }

    void Reset() override {
        m_buffer.assign(m_buffer.size(), 0.0f);
        m_writePos = 0;
        m_lpfState = 0.0f;
    }

private:
    std::vector<float> m_buffer;
    size_t m_writePos = 0;
    uint32_t m_maxDelaySamples = 0;
    float m_lpfState = 0.0f;
};

} // namespace Prisma::Audio::DSP
