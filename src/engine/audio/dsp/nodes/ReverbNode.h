#pragma once

#include <audio/dsp/AudioNode.h>
#include <vector>
#include <cmath>
#include <array>

namespace Prisma::Audio::DSP {

class ReverbNode : public AudioNode {
public:
    ReverbNode() {
        AddInputPin("Input");
        AddOutputPin("Output");
        SetName("Reverb");
        SetParameter("roomSize", 0.7f);
        SetParameter("damping", 0.5f);
        SetParameter("width", 1.0f);
        SetParameter("mix", 0.33f);
        SetParameter("preDelay", 0.03f);
    }

    void InitializeFilter(uint32_t sampleRate) {
        if (m_initialized) return;

        // Schroeder-Moorer reverb parameters
        m_preDelaySamples = static_cast<uint32_t>(0.03f * sampleRate);
        m_combs = {
            {1557, 0.84f}, {1617, 0.84f}, {1491, 0.83f}, {1422, 0.83f},
            {1277, 0.84f}, {1356, 0.83f}, {1188, 0.83f}, {1116, 0.84f}
        };
        m_allpasses = {
            {225, 0.5f}, {341, 0.5f}, {441, 0.5f}, {556, 0.5f}
        };

        size_t maxSize = 0;
        for (auto& c : m_combs) maxSize = std::max(maxSize, c.delay + sampleRate / 2);
        for (auto& a : m_allpasses) maxSize = std::max(maxSize, a.delay + sampleRate / 2);
        maxSize += m_preDelaySamples + sampleRate / 2;

        m_bufL.resize(maxSize, 0.0f);
        m_bufR.resize(maxSize, 0.0f);
        m_initialized = true;
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }

        InitializeFilter(ctx.sampleRate);

        float roomSize = std::clamp(GetParameter("roomSize"), 0.0f, 1.0f);
        float damping = std::clamp(GetParameter("damping"), 0.0f, 1.0f);
        float width = std::clamp(GetParameter("width"), 0.0f, 1.0f);
        float mix = std::clamp(GetParameter("mix"), 0.0f, 1.0f);

        // Scale feedback by room size
        for (auto& c : m_combs) c.feedback = (0.8f + roomSize * 0.19f);

        uint32_t ch = std::min(output.GetChannels(), input->GetChannels());
        uint32_t frames = output.GetFrames();

        for (uint32_t c = 0; c < ch; ++c) {
            const float* src = input->GetChannel(c);
            float* dst = output.GetChannel(c);
            auto& buf = (c == 0) ? m_bufL : m_bufR;

            for (uint32_t f = 0; f < frames; ++f) {
                float dry = src[f];

                // Pre-delay
                buf[m_writePos] = dry;
                size_t preRead = (m_writePos + buf.size() - m_preDelaySamples) % buf.size();
                float wet = buf[preRead];

                // Parallel comb filters
                float combOut = 0.0f;
                for (auto& comb : m_combs) {
                    size_t readPos = (m_writePos + buf.size() - comb.delay) % buf.size();
                    float sample = buf[readPos];
                    float filtered = m_lpfState + damping * (sample - m_lpfState);
                    m_lpfState = filtered;
                    buf[m_writePos] = wet + filtered * comb.feedback;
                    combOut += sample;
                }
                combOut /= m_combs.size();

                // Series all-pass filters
                float apOut = combOut;
                for (auto& ap : m_allpasses) {
                    size_t readPos = (m_writePos + buf.size() - ap.delay) % buf.size();
                    float sample = buf[readPos];
                    buf[m_writePos] = apOut + sample * ap.feedback;
                    apOut = -apOut + sample * (1.0f - ap.feedback);
                }

                float stereoWet = (c == 0)
                    ? apOut * (1.0f + width * 0.5f)
                    : apOut * (1.0f - width * 0.5f);

                dst[f] = dry * (1.0f - mix) + stereoWet * mix;
                m_writePos = (m_writePos + 1) % buf.size();
            }
        }
    }

    void Reset() override {
        m_bufL.assign(m_bufL.size(), 0.0f);
        m_bufR.assign(m_bufR.size(), 0.0f);
        m_writePos = 0;
        m_lpfState = 0.0f;
    }

private:
    struct CombFilter { size_t delay; float feedback; };
    struct AllpassFilter { size_t delay; float feedback; };

    std::vector<float> m_bufL, m_bufR;
    size_t m_writePos = 0;
    size_t m_preDelaySamples = 0;
    float m_lpfState = 0.0f;
    bool m_initialized = false;

    std::vector<CombFilter> m_combs;
    std::vector<AllpassFilter> m_allpasses;
};

} // namespace Prisma::Audio::DSP
