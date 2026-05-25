#pragma once

#include <audio/dsp/AudioNode.h>
#include <vector>
#include <algorithm>

namespace Prisma::Audio::DSP {

class ConvolutionReverbNode : public AudioNode {
public:
    ConvolutionReverbNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("ConvolutionReverb");
        SetParameter("mix", 0.33f);
        SetParameter("gain", 1.0f);
    }

    void LoadIR(const std::vector<float>& ir, uint32_t irSampleRate) {
        m_ir = ir;
        m_irSize = ir.size();
        m_irSampleRate = irSampleRate;
        m_inputBuf.assign(m_irSize + 4096, 0.0f);
        m_readPos = 0;
    }

    void LoadIR(const float* data, size_t size, uint32_t irSampleRate) {
        m_ir.assign(data, data + size);
        m_irSize = size;
        m_irSampleRate = irSampleRate;
        m_inputBuf.assign(m_irSize + 4096, 0.0f);
        m_readPos = 0;
    }

    bool HasIR() const { return m_irSize > 0; }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input || !HasIR()) { output.CopyFrom(*input ? *input : output); return; }

        float mix = std::clamp(GetParameter("mix"), 0.0f, 1.0f);
        float gain = GetParameter("gain");

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        if (m_inputBuf.size() < frames + m_irSize) {
            m_inputBuf.resize(frames + m_irSize, 0.0f);
        }

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);

            // Copy input to buffer
            for (uint32_t f = 0; f < frames; ++f) {
                m_inputBuf[(m_readPos + f) % m_inputBuf.size()] = src[f];
            }

            // Convolve: output[n] = sum(input[n - k] * ir[k])
            for (uint32_t f = 0; f < frames; ++f) {
                float wet = 0.0f;
                size_t base = (m_readPos + f + m_inputBuf.size()) % m_inputBuf.size();

                for (size_t k = 0; k < m_irSize && k < 8192; ++k) {
                    size_t idx = (base + m_inputBuf.size() - k) % m_inputBuf.size();
                    wet += m_inputBuf[idx] * m_ir[k];
                }

                wet *= gain;
                float dry = src[f];
                dst[f] = dry * (1.0f - mix) + wet * mix;
            }

            m_readPos = (m_readPos + frames) % m_inputBuf.size();
        }
    }

    void Reset() override {
        m_inputBuf.assign(m_inputBuf.size(), 0.0f);
        m_readPos = 0;
    }

private:
    std::vector<float> m_ir;
    std::vector<float> m_inputBuf;
    size_t m_irSize = 0;
    size_t m_readPos = 0;
    uint32_t m_irSampleRate = 48000;
};

} // namespace Prisma::Audio::DSP
