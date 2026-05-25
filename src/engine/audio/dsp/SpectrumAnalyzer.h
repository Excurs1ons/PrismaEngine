#pragma once

#include "AudioNode.h"
#include "AudioMath.h"
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class SpectrumAnalyzer {
public:
    SpectrumAnalyzer(uint32_t fftSize = 2048)
        : m_fftSize(fftSize)
        , m_window(fftSize, 0.0f)
        , m_real(fftSize, 0.0f)
        , m_imag(fftSize, 0.0f)
    {
        // Hanning window
        for (uint32_t i = 0; i < fftSize; ++i) {
            m_window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (fftSize - 1)));
        }
    }

    struct Bin {
        float frequency;
        float magnitude;     // dB
        float phase;         // radians
    };

    void Process(const float* input, uint32_t frames, uint32_t sampleRate) {
        for (uint32_t i = 0; i < m_fftSize && i < frames; ++i) {
            m_real[i] = input[i] * m_window[i];
            m_imag[i] = 0.0f;
        }

        if (frames < m_fftSize) {
            std::fill(m_real.begin() + frames, m_real.end(), 0.0f);
        }

        FFT(m_real.data(), m_imag.data(), m_fftSize);

        m_bins.clear();
        uint32_t numBins = m_fftSize / 2;
        m_bins.reserve(numBins);

        for (uint32_t i = 0; i < numBins; ++i) {
            float mag = sqrtf(m_real[i] * m_real[i] + m_imag[i] * m_imag[i]);
            float magDb = (mag > 0.00001f) ? 20.0f * log10f(mag) : -100.0f;
            float phase = atan2f(m_imag[i], m_real[i]);
            float freq = static_cast<float>(i) * sampleRate / m_fftSize;

            m_bins.push_back({freq, magDb, phase});
        }
    }

    const std::vector<Bin>& GetBins() const { return m_bins; }

    float GetPeakMagnitude() const {
        float peak = -100.0f;
        for (auto& b : m_bins) peak = std::max(peak, b.magnitude);
        return peak;
    }

    float GetMagnitudeAt(float freq) const {
        float closest = -100.0f;
        float minDiff = 1e10f;
        for (auto& b : m_bins) {
            float diff = fabsf(b.frequency - freq);
            if (diff < minDiff) { minDiff = diff; closest = b.magnitude; }
        }
        return closest;
    }

private:
    void FFT(float* real, float* imag, uint32_t n) {
        uint32_t bits = static_cast<uint32_t>(log2f(n));
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t j = BitReverse(i, bits);
            if (j > i) {
                std::swap(real[i], real[j]);
                std::swap(imag[i], imag[j]);
            }
        }

        for (uint32_t len = 2; len <= n; len <<= 1) {
            float ang = 2.0f * M_PI / len;
            float wReal = cosf(ang);
            float wImag = -sinf(ang);

            for (uint32_t i = 0; i < n; i += len) {
                float curReal = 1.0f, curImag = 0.0f;
                for (uint32_t j = 0; j < len / 2; ++j) {
                    float tReal = curReal * real[i + j + len / 2] - curImag * imag[i + j + len / 2];
                    float tImag = curReal * imag[i + j + len / 2] + curImag * real[i + j + len / 2];

                    real[i + j + len / 2] = real[i + j] - tReal;
                    imag[i + j + len / 2] = imag[i + j] - tImag;
                    real[i + j] += tReal;
                    imag[i + j] += tImag;

                    float newReal = curReal * wReal - curImag * wImag;
                    curImag = curReal * wImag + curImag * wReal;
                    curReal = newReal;
                }
            }
        }
    }

    static uint32_t BitReverse(uint32_t x, uint32_t bits) {
        uint32_t y = 0;
        for (uint32_t i = 0; i < bits; ++i) {
            y = (y << 1) | (x & 1);
            x >>= 1;
        }
        return y;
    }

    uint32_t m_fftSize;
    std::vector<float> m_window;
    std::vector<float> m_real;
    std::vector<float> m_imag;
    std::vector<Bin> m_bins;
};

class SpectrumAnalyzerNode : public AudioNode {
public:
    SpectrumAnalyzerNode(uint32_t fftSize = 2048) : m_analyzer(fftSize) {
        AddInputPin("Input");
        SetName("SpectrumAnalyzer");
        SetParameter("fftSize", static_cast<float>(fftSize));
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        if (input->GetChannels() > 0) {
            m_analyzer.Process(input->GetChannel(0), input->GetFrames(), ctx.sampleRate);
        }

        output.CopyFrom(*input);
    }

    const SpectrumAnalyzer& GetAnalyzer() const { return m_analyzer; }
    SpectrumAnalyzer& GetAnalyzer() { return m_analyzer; }

    void Reset() override {}

private:
    SpectrumAnalyzer m_analyzer;
};

} // namespace Prisma::Audio::DSP
