#pragma once

#include <audio/dsp/AudioNode.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

namespace Prisma::Audio::DSP {

class MixerManagerNode : public AudioNode {
public:
    MixerManagerNode() {
        AddInputPin("MasterInput");
        AddOutputPin("MasterOutput");
        SetName("MixerManager");
        SetParameter("masterVolume", 1.0f);
    }

    struct Channel {
        std::string name;
        float volume = 1.0f;
        float pan = 0.0f;
        bool mute = false;
        bool solo = false;
        float auxSendLevel = 0.0f;
    };

    Channel* AddChannel(const std::string& name) {
        m_channels.push_back({name});
        AddInputPin(name + "Input");
        return &m_channels.back();
    }

    Channel* GetChannel(const std::string& name) {
        for (auto& ch : m_channels)
            if (ch.name == name) return &ch;
        return nullptr;
    }

    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        uint32_t channels = output.GetChannels();
        uint32_t frames = output.GetFrames();
        output.Clear();

        bool anySolo = false;
        for (auto& ch : m_channels) if (ch.solo) anySolo = true;

        for (size_t i = 0; i < m_channels.size(); ++i) {
            auto& ch = m_channels[i];
            AudioBuffer* chInput = ReadInput(ch.name + "Input");
            if (!chInput) continue;
            if (ch.mute) continue;
            if (anySolo && !ch.solo) continue;

            float leftGain = ch.volume * (ch.pan <= 0.0f ? 1.0f : 1.0f - ch.pan);
            float rightGain = ch.volume * (ch.pan >= 0.0f ? 1.0f : 1.0f + ch.pan);

            uint32_t inCh = std::min(channels, chInput->GetChannels());
            for (uint32_t c = 0; c < inCh; ++c) {
                const float* src = chInput->GetChannel(c);
                float* dst = output.GetChannel(c);
                float g = (c == 0) ? leftGain : rightGain;
                for (uint32_t f = 0; f < frames; ++f) {
                    dst[f] += src[f] * g;
                }
            }
        }

        float masterVol = GetParameter("masterVolume");
        if (masterVol != 1.0f) {
            for (uint32_t c = 0; c < channels; ++c) {
                float* dst = output.GetChannel(c);
                for (uint32_t f = 0; f < frames; ++f) dst[f] *= masterVol;
            }
        }
    }

    void Reset() override {}

    size_t GetChannelCount() const { return m_channels.size(); }

private:
    std::vector<Channel> m_channels;
};

} // namespace Prisma::Audio::DSP
