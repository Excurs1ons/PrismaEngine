#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>

namespace Prisma::Audio::DSP {

class SidechainNode : public AudioNode {
public:
    SidechainNode() {
        AddInputPin("Input");
        AddInputPin("Sidechain");
        AddOutputPin("Output");
        SetName("Sidechain");
        SetParameter("ratio", 0.5f);
        SetParameter("attack", 0.003f);
        SetParameter("release", 0.050f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        AudioBuffer* sidechain = ReadInput("Sidechain");

        if (!input) { output.Clear(); return; }

        float ratio = std::clamp(GetParameter("ratio"), 0.0f, 1.0f);
        float attackCoeff = expf(-1.0f / (GetParameter("attack") * ctx.sampleRate));
        float releaseCoeff = expf(-1.0f / (GetParameter("release") * ctx.sampleRate));

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                float dry = src[f];

                float scLevel = 1.0f;
                if (sidechain) {
                    float scSample = std::abs(sidechain->GetChannel(std::min(c, sidechain->GetChannels() - 1))[f]);
                    scLevel = 1.0f - scSample * ratio;
                    scLevel = std::max(0.0f, scLevel);
                }

                float target = scLevel;
                if (target < m_env) {
                    m_env += (target - m_env) * (1.0f - attackCoeff);
                } else {
                    m_env += (target - m_env) * (1.0f - releaseCoeff);
                }

                dst[f] = dry * m_env;
            }
        }
    }

    void Reset() override { m_env = 1.0f; }

private:
    float m_env = 1.0f;
};

} // namespace Prisma::Audio::DSP
