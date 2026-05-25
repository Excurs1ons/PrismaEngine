#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <array>

namespace Prisma::Audio::DSP {

class GraphicEQNode : public AudioNode {
public:
    static constexpr int kNumBands = 10;
    static constexpr float kFrequencies[kNumBands] = {
        31.5f, 63.0f, 125.0f, 250.0f, 500.0f,
        1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
    };

    GraphicEQNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("GraphicEQ");
        for (int i = 0; i < kNumBands; ++i) {
            SetParameter("band" + std::to_string(i), 0.0f);
        }
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            output.CopyFrom(*input);
            float* dst = output.GetChannel(c);

            for (int b = 0; b < kNumBands; ++b) {
                float gainDb = GetParameter("band" + std::to_string(b));
                if (fabsf(gainDb) < 0.5f) continue;

                ProcessBand(dst, frames, ctx.sampleRate, kFrequencies[b], gainDb, c);
            }
        }
    }

    void Reset() override {
        m_biquads = {};
    }

private:
    struct Biquad {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };
    Biquad m_biquads[kNumBands][2];

    void ProcessBand(float* buf, uint32_t frames, float sr, float freq, float gainDb, int ch) {
        float A = powf(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * M_PI * freq / sr;
        float alpha = sinf(w0) * 0.5f;

        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosf(w0);
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosf(w0);
        float a2 = 1.0f - alpha / A;
        float a0Inv = 1.0f / a0;

        auto& bq = m_biquads[0][ch];

        for (uint32_t f = 0; f < frames; ++f) {
            float x = buf[f];
            float y = b0 * x + b1 * bq.x1 + b2 * bq.x2 - a1 * bq.y1 - a2 * bq.y2;
            y *= a0Inv;
            bq.x2 = bq.x1; bq.x1 = x;
            bq.y2 = bq.y1; bq.y1 = y;
            buf[f] = y;
        }
    }
};

} // namespace Prisma::Audio::DSP
