#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class SVFNode : public AudioNode {
public:
    enum class Mode { Lowpass, Highpass, Bandpass, Notch, Peak, Allpass };

    SVFNode() {
        AddInputPin("Input");
        AddOutputPin("Lowpass");
        AddOutputPin("Highpass");
        AddOutputPin("Bandpass");
        AddOutputPin("Notch");
        AddOutputPin("Peak");
        AddOutputPin("Allpass");
        SetName("SVF");
        SetParameter("cutoff", 1000.0f);
        SetParameter("resonance", 0.0f);
        SetParameter("mode", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float cutoff = std::clamp(GetParameter("cutoff"), 20.0f, ctx.sampleRate * 0.49f);
        float res = std::clamp(GetParameter("resonance"), 0.0f, 0.99f);

        float g = tanf(M_PI * cutoff / ctx.sampleRate);
        float k = 2.0f * res;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* lp = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                float v0 = src[f];
                float v3 = v0 - m_ic2eq[c];
                float v1 = a1 * m_ic1eq[c] + a2 * v3;
                float v2 = m_ic2eq[c] + a2 * m_ic1eq[c] + a3 * v3;

                m_ic1eq[c] = 2.0f * v1 - m_ic1eq[c];
                m_ic2eq[c] = 2.0f * v2 - m_ic2eq[c];

                float low = v2;
                float high = v0 - k * v1 - v2;
                float band = v1;
                float notch = low + high;
                float peak = low - high;
                float allpass = 2.0f * band - v0;

                int mode = static_cast<int>(GetParameter("mode")) % 6;
                switch (mode) {
                    case 0: lp[f] = low; break;
                    case 1: lp[f] = high; break;
                    case 2: lp[f] = band; break;
                    case 3: lp[f] = notch; break;
                    case 4: lp[f] = peak; break;
                    case 5: lp[f] = allpass; break;
                }
            }
        }
    }

    void Reset() override {
        m_ic1eq.assign(2, 0.0f);
        m_ic2eq.assign(2, 0.0f);
    }

private:
    float m_ic1eq[2] = {};
    float m_ic2eq[2] = {};
};

} // namespace Prisma::Audio::DSP
