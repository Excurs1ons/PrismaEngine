#pragma once

#include <audio/dsp/AudioNode.h>
#include <vector>
#include <cmath>

namespace Prisma::Audio::DSP {

class SampleRateConverterNode : public AudioNode {
public:
    SampleRateConverterNode(uint32_t inputRate = 48000, uint32_t outputRate = 48000)
        : m_inputRate(inputRate), m_outputRate(outputRate) {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("SampleRateConverter");
        SetParameter("quality", 1.0f);
    }

    void SetInputSampleRate(uint32_t rate) { m_inputRate = rate; }
    void SetOutputSampleRate(uint32_t rate) { m_outputRate = rate; m_phase = 0.0; }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        if (m_inputRate == m_outputRate) {
            output.CopyFrom(*input);
            return;
        }

        float quality = GetParameter("quality");
        uint32_t inCh = input->GetChannels();
        uint32_t outCh = output.GetChannels();
        uint32_t outFrames = output.GetFrames();

        float ratio = static_cast<float>(m_inputRate) / m_outputRate;
        const float* inData[8];
        for (uint32_t c = 0; c < std::min(inCh, (uint32_t)8); ++c)
            inData[c] = input->GetChannel(c);

        for (uint32_t f = 0; f < outFrames; ++f) {
            double readPos = m_phase;
            int32_t i0 = static_cast<int32_t>(readPos);
            float frac = static_cast<float>(readPos - i0);

            for (uint32_t c = 0; c < outCh && c < inCh; ++c) {
                float s0 = GetInputSample(inData[c], i0, input->GetFrames());
                float s1 = GetInputSample(inData[c], i0 + 1, input->GetFrames());

                if (quality > 0.5f) {
                    float sm1 = GetInputSample(inData[c], i0 - 1, input->GetFrames());
                    float s2 = GetInputSample(inData[c], i0 + 2, input->GetFrames());
                    output.GetChannel(c)[f] = HermiteInterp(sm1, s0, s1, s2, frac);
                } else {
                    output.GetChannel(c)[f] = s0 + (s1 - s0) * frac;
                }
            }

            m_phase += ratio;
        }
    }

    void Reset() override {
        m_phase = 0.0;
    }

private:
    static float GetInputSample(const float* buf, int32_t idx, uint32_t len) {
        if (idx < 0) return buf[0];
        if (idx >= static_cast<int32_t>(len)) return buf[len - 1];
        return buf[idx];
    }

    static float HermiteInterp(float ym1, float y0, float y1, float y2, float t) {
        float c0 = y0;
        float c1 = (y1 - ym1) * 0.5f;
        float c2 = ym1 - y0 * 2.5f + y1 * 2.0f - y2 * 0.5f;
        float c3 = (y2 - ym1) * 0.5f + (y0 - y1) * 1.5f;
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    uint32_t m_inputRate;
    uint32_t m_outputRate;
    double m_phase = 0.0;
};

} // namespace Prisma::Audio::DSP
