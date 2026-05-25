#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <array>

namespace Prisma::Audio::DSP {

class BiquadFilterNode : public AudioNode {
public:
    enum class Type { Lowpass, Highpass, Bandpass, Notch, Peaking, LowShelf, HighShelf };

    BiquadFilterNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("BiquadFilter");
        SetParameter("frequency", 1000.0f);
        SetParameter("q", 0.707f);
        SetParameter("gain", 0.0f);
        SetParameter("type", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        float freq = std::clamp(GetParameter("frequency"), 20.0f, ctx.sampleRate * 0.49f);
        float q = std::max(0.001f, GetParameter("q"));
        float gainDb = GetParameter("gain");
        int type = std::clamp(static_cast<int>(GetParameter("type")), 0, 6);

        float w0 = 2.0f * M_PI * freq / ctx.sampleRate;
        float cos_w0 = cosf(w0);
        float sin_w0 = sinf(w0);
        float alpha = sin_w0 / (2.0f * q);
        float A = powf(10.0f, gainDb / 40.0f);

        float b0, b1, b2, a0, a1, a2;

        switch (type) {
            case 0:
                b0 = (1.0f - cos_w0) * 0.5f; b1 = 1.0f - cos_w0; b2 = b0;
                a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
                break;
            case 1:
                b0 = (1.0f + cos_w0) * 0.5f; b1 = -(1.0f + cos_w0); b2 = b0;
                a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
                break;
            case 2:
                b0 = alpha; b1 = 0.0f; b2 = -alpha;
                a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
                break;
            case 3:
                b0 = 1.0f; b1 = -2.0f * cos_w0; b2 = 1.0f;
                a0 = 1.0f + alpha; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha;
                break;
            case 4:
                b0 = 1.0f + alpha * A; b1 = -2.0f * cos_w0; b2 = 1.0f - alpha * A;
                a0 = 1.0f + alpha / A; a1 = -2.0f * cos_w0; a2 = 1.0f - alpha / A;
                break;
            case 5:
                b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * sqrtf(A) * alpha);
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * sqrtf(A) * alpha);
                a0 = (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * sqrtf(A) * alpha;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
                a2 = (A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * sqrtf(A) * alpha;
                break;
            default:
                b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
                a0 = 1.0f; a1 = 0.0f; a2 = 0.0f;
        }

        float a0Inv = 1.0f / a0;

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            for (uint32_t f = 0; f < frames; ++f) {
                float x = src[f];
                float y = b0 * x + b1 * m_x1[c] + b2 * m_x2[c]
                        - a1 * m_y1[c] - a2 * m_y2[c];
                y *= a0Inv;

                m_x2[c] = m_x1[c];
                m_x1[c] = x;
                m_y2[c] = m_y1[c];
                m_y1[c] = y;

                dst[f] = y;
            }
        }
    }

    void Reset() override {
        m_x1 = m_x2 = m_y1 = m_y2 = {0.0f, 0.0f};
    }

private:
    float m_x1[2] = {}, m_x2[2] = {};
    float m_y1[2] = {}, m_y2[2] = {};
};

} // namespace Prisma::Audio::DSP
