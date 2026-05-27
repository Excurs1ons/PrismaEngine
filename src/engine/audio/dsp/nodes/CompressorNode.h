#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class CompressorNode : public AudioNode {
public:
    CompressorNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("Compressor");
        SetParameter("threshold", -24.0f);
        SetParameter("ratio", 4.0f);
        SetParameter("attack", 0.003f);
        SetParameter("release", 0.100f);
        SetParameter("makeup", 0.0f);
        SetParameter("knee", 6.0f);
        SetParameter("mix", 1.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float thresholdDb = GetParameter("threshold");
        float ratio = std::max(1.0f, GetParameter("ratio"));
        float attackMs = std::max(0.001f, GetParameter("attack"));
        float releaseMs = std::max(0.010f, GetParameter("release"));
        float makeupDb = GetParameter("makeup");
        float knee = std::max(0.0f, GetParameter("knee"));
        float mix = std::clamp(GetParameter("mix"), 0.0f, 1.0f);

        float attackCoeff = expf(-1.0f / (attackMs * ctx.sampleRate / 1000.0f));
        float releaseCoeff = expf(-1.0f / (releaseMs * ctx.sampleRate / 1000.0f));

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                float dry = src[f];
                float absVal = std::abs(dry);
                float levelDb = (absVal > 0.0001f) ? 20.0f * log10f(absVal) : -100.0f;

                // Soft knee compression
                float reductionDb = 0.0f;
                if (levelDb > thresholdDb - knee * 0.5f) {
                    float overDb = levelDb - thresholdDb;
                    if (knee > 0.0f && overDb < knee) {
                        float kneeX = overDb / knee;
                        reductionDb = (1.0f - 1.0f / ratio) * (kneeX * kneeX) * knee * 0.5f;
                    } else {
                        reductionDb = (overDb) * (1.0f - 1.0f / ratio);
                    }
                }

                // Envelope following
                float targetGain = powf(10.0f, -reductionDb / 20.0f);
                if (targetGain < m_envelope) {
                    m_envelope += (targetGain - m_envelope) * (1.0f - attackCoeff);
                } else {
                    m_envelope += (targetGain - m_envelope) * (1.0f - releaseCoeff);
                }

                float makeupGain = powf(10.0f, makeupDb / 20.0f);
                float wet = dry * m_envelope * makeupGain;

                dst[f] = dry * (1.0f - mix) + wet * mix;
            }
        }
    }

    void Reset() override {
        m_envelope = 1.0f;
    }

private:
    float m_envelope = 1.0f;
};

} // namespace Prisma::Audio::DSP
