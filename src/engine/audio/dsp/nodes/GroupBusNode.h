#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>

namespace Prisma::Audio::DSP {

class GroupBusNode : public AudioNode {
public:
    GroupBusNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        AddOutputPin("AuxSend");
        SetName("GroupBus");
        SetParameter("volume", 1.0f);
        SetParameter("pan", 0.0f);
        SetParameter("mute", 0.0f);
        SetParameter("auxSendLevel", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input || GetParameter("mute") > 0.5f) {
            output.Clear();
            return;
        }

        float volume = GetParameter("volume");
        float pan = std::clamp(GetParameter("pan"), -1.0f, 1.0f);

        float leftGain = volume * (pan <= 0.0f ? 1.0f : 1.0f - pan);
        float rightGain = volume * (pan >= 0.0f ? 1.0f : 1.0f + pan);

        uint32_t frames = output.GetFrames();

        if (output.GetChannels() >= 2 && input->GetChannels() >= 2) {
            const float* srcL = input->GetChannel(0);
            const float* srcR = input->GetChannel(1);
            float* dstL = output.GetChannel(0);
            float* dstR = output.GetChannel(1);
            for (uint32_t f = 0; f < frames; ++f) {
                dstL[f] = srcL[f] * leftGain;
                dstR[f] = srcR[f] * rightGain;
            }
        } else if (input->GetChannels() >= 1) {
            const float* src = input->GetChannel(0);
            for (uint32_t c = 0; c < output.GetChannels(); ++c) {
                float* dst = output.GetChannel(c);
                float g = (c == 0) ? leftGain : rightGain;
                for (uint32_t f = 0; f < frames; ++f) dst[f] = src[f] * g;
            }
        }
    }

    void Reset() override {}
};

} // namespace Prisma::Audio::DSP
