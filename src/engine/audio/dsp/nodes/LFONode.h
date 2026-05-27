#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>

namespace Prisma::Audio::DSP {

class LFONode : public AudioNode {
public:
    LFONode() {
        AddOutputPin("Output");
        SetName("LFO");
        SetParameter("rate", 1.0f);
        SetParameter("depth", 1.0f);
        SetParameter("waveform", 0.0f); // 0=Sine, 1=Triangle, 2=Square, 3=Saw, 4=S&H
        SetParameter("offset", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        float rate = GetParameter("rate");
        float depth = std::clamp(GetParameter("depth"), 0.0f, 1.0f);
        int wf = static_cast<int>(GetParameter("waveform")) % 5;
        float offset = GetParameter("offset");

        uint32_t channels = output.GetChannels();
        uint32_t frames = output.GetFrames();

        for (uint32_t f = 0; f < frames; ++f) {
            float val = 0.0f;

            switch (wf) {
                case 0: val = sinf(m_phase); break;
                case 1: val = (m_phase < M_PI) ? (m_phase / M_PI * 2.0f - 1.0f) : (3.0f - m_phase / M_PI * 2.0f); break;
                case 2: val = (m_phase < M_PI) ? 1.0f : -1.0f; break;
                case 3: val = m_phase / M_PI - 1.0f; break;
                case 4: {
                    if (m_phase >= 2.0f * M_PI) {
                        m_phase -= 2.0f * M_PI;
                        m_shValue = m_noiseDist(m_rng) * 2.0f - 1.0f;
                    }
                    val = m_shValue;
                    break;
                }
            }

            float out = (val * 0.5f + 0.5f) * depth + offset;
            out = std::clamp(out, 0.0f, 1.0f);

            for (uint32_t c = 0; c < channels; ++c)
                output.GetChannel(c)[f] = out;

            m_phase += 2.0f * M_PI * rate / ctx.sampleRate;
            if (m_phase >= 2.0f * M_PI) m_phase -= 2.0f * M_PI;
        }
    }

    void Reset() override {
        m_phase = 0.0f;
        m_shValue = 0.0f;
    }

private:
    float m_phase = 0.0f;
    float m_shValue = 0.0f;
    std::mt19937 m_rng{42};
    std::uniform_real_distribution<float> m_noiseDist{0.0f, 1.0f};
};

} // namespace Prisma::Audio::DSP
