#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>

namespace Prisma::Audio::DSP {

class MasterBusNode : public AudioNode {
public:
    MasterBusNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("MasterBus");
        SetParameter("volume", 1.0f);
        SetParameter("mute", 0.0f);
        SetParameter("solo", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input || GetParameter("mute") > 0.5f) {
            output.Clear();
            return;
        }

        float volume = GetParameter("volume");
        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);
            for (uint32_t f = 0; f < frames; ++f) {
                dst[f] = src[f] * volume;
            }
        }
    }

    void Reset() override {}
};

} // namespace Prisma::Audio::DSP
