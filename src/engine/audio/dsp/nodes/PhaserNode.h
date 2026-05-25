#pragma once

#include <audio/dsp/AudioNode.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class PhaserNode : public AudioNode {
public:
    PhaserNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("Phaser");
        SetParameter("rate", 0.4f);
        SetParameter("depth", 0.8f);
        SetParameter("stages", 4.0f);
        SetParameter("feedback", 0.3f);
        SetParameter("mix", 0.5f);
        SetParameter("centerFreq", 800.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float rate = GetParameter("rate");
        float depth = std::clamp(GetParameter("depth"), 0.0f, 1.0f);
        int stages = std::clamp(static_cast<int>(GetParameter("stages")), 2, 12);
        float fb = std::clamp(GetParameter("feedback"), 0.0f, 0.95f);
        float mix = std::clamp(GetParameter("mix"), 0.0f, 1.0f);
        float centerFreq = GetParameter("centerFreq");

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        // Ensure filter state arrays
        if (m_zm1[0].size() < static_cast<size_t>(stages)) {
            for (int c = 0; c < 2; ++c) m_zm1[c].assign(stages, 0.0f);
        }

        float lfo = sinf(m_phase);
        float modFreq = centerFreq * std::pow(2.0f, lfo * depth * 2.0f);

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            float feedbackIn = 0.0f;

            for (uint32_t f = 0; f < frames; ++f) {
                float x = src[f] + feedbackIn * fb;

                for (int s = 0; s < stages; ++s) {
                    float w = 2.0f * M_PI * modFreq / ctx.sampleRate;
                    float b = (1.0f - w * 0.5f) / (1.0f + w * 0.5f);
                    float y = b * (x - m_zm1[c][s]);
                    m_zm1[c][s] = y + b * x;
                    x = y;
                }

                float wet = x;
                feedbackIn = wet;
                float dry = src[f];
                dst[f] = dry * (1.0f - mix) + wet * mix;
            }
        }

        m_phase += 2.0f * M_PI * rate / ctx.sampleRate * frames;
        if (m_phase > 2.0f * M_PI) m_phase -= 2.0f * M_PI;
    }

    void Reset() override {
        m_phase = 0.0f;
        for (int c = 0; c < 2; ++c)
            m_zm1[c].assign(m_zm1[c].size(), 0.0f);
    }

private:
    float m_phase = 0.0f;
    std::vector<float> m_zm1[2];
};

} // namespace Prisma::Audio::DSP
