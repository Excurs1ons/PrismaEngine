#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class LevelMeterNode : public AudioNode {
public:
    LevelMeterNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("LevelMeter");
    }

    struct MeterData {
        float peak = 0.0f;
        float rms = 0.0f;
        float peakDb = -100.0f;
        float rmsDb = -100.0f;
    };

    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        m_data.resize(ch);
        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            float peak = 0.0f;
            double sumSq = 0.0;

            for (uint32_t f = 0; f < frames; ++f) {
                float absVal = std::abs(src[f]);
                peak = std::max(peak, absVal);
                sumSq += src[f] * src[f];
                dst[f] = src[f];
            }

            m_data[c].peak = peak;
            m_data[c].rms = sqrtf(static_cast<float>(sumSq / frames));
            m_data[c].peakDb = (peak > 0.00001f) ? 20.0f * log10f(peak) : -100.0f;
            m_data[c].rmsDb = (m_data[c].rms > 0.00001f) ? 20.0f * log10f(m_data[c].rms) : -100.0f;
        }
    }

    const MeterData& GetChannelData(uint32_t ch) const {
        static MeterData empty;
        return ch < m_data.size() ? m_data[ch] : empty;
    }

    size_t GetChannelCount() const { return m_data.size(); }

    void Reset() override { m_data.clear(); }

private:
    std::vector<MeterData> m_data;
};

} // namespace Prisma::Audio::DSP
