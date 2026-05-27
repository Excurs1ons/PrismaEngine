#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class DistortionNode : public AudioNode {
public:
    enum class Type {
        HardClip,
        SoftClip,
        HalfWave,
        FullWave,
        BitCrush,
        Foldback
    };

    DistortionNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("Distortion");
        SetParameter("drive", 1.0f);
        SetParameter("tone", 1.0f);
        SetParameter("mix", 1.0f);
        SetParameter("type", 0.0f);
        SetParameter("bitDepth", 8.0f);
    }

    void SetType(Type t) { SetParameter("type", static_cast<float>(t)); }

    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float drive = GetParameter("drive");
        float tone = GetParameter("tone");
        float mix = GetParameter("mix");
        int type = static_cast<int>(GetParameter("type"));
        float bitDepth = std::max(2.0f, GetParameter("bitDepth"));

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        // Simple 1-pole lowpass for tone control
        float lpfState = 0.0f;
        float lpfCoeff = tone * 0.5f;

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                float dry = src[f];
                float wet = dry * drive;

                switch (type) {
                    case 0: wet = HardClip(wet); break;
                    case 1: wet = SoftClip(wet); break;
                    case 2: wet = (wet > 0) ? wet : 0; break;
                    case 3: wet = std::abs(wet); break;
                    case 4: wet = BitCrush(wet, bitDepth); break;
                    case 5: wet = Foldback(wet, drive); break;
                }

                lpfState += lpfCoeff * (wet - lpfState);
                wet = lpfState;

                dst[f] = dry * (1.0f - mix) + wet * mix;
            }
        }
    }

    void Reset() override {}

private:
    static float HardClip(float s) {
        return std::clamp(s, -1.0f, 1.0f);
    }

    static float SoftClip(float s) {
        if (s > 1.5f) return 1.0f;
        if (s < -1.5f) return -1.0f;
        return s - (4.0f / 27.0f) * s * s * s;
    }

    static float BitCrush(float s, float bits) {
        float steps = std::pow(2.0f, bits - 1.0f);
        return std::round(s * steps) / steps;
    }

    static float Foldback(float s, float threshold) {
        if (s > threshold || s < -threshold) {
            s = std::abs(std::abs(std::fmod(s - threshold, threshold * 4.0f)) - threshold * 2.0f) - threshold;
        }
        return s;
    }
};

} // namespace Prisma::Audio::DSP
