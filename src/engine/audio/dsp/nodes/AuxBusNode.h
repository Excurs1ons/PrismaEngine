#pragma once

#include <audio/dsp/AudioNode.h>

namespace Prisma::Audio::DSP {

class AuxBusNode : public AudioNode {
public:
    AuxBusNode() {
        AddInputPin("Send");
        AddOutputPin("Return");
        SetName("AuxBus");
        SetParameter("returnLevel", 1.0f);
        SetParameter("preFader", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        AudioBuffer* send = ReadInput("Send");
        if (!send) { output.Clear(); return; }

        float returnLevel = GetParameter("returnLevel");
        uint32_t ch = std::min(output.GetChannels(), send->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = send->GetChannel(c);
            float* dst = output.GetChannel(c);
            for (uint32_t f = 0; f < frames; ++f) {
                dst[f] = src[f] * returnLevel;
            }
        }
    }

    void Reset() override {}
};

} // namespace Prisma::Audio::DSP
