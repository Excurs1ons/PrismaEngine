#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <random>

namespace Prisma::Audio::DSP {

class OscillatorNode : public AudioNode {
public:
    enum class Waveform { Sine, Square, Saw, Triangle, Noise };

    OscillatorNode(Waveform wf = Waveform::Sine) : m_waveform(wf) {
        AddOutputPin("Output");
        AddInputPin("FreqMod");
        AddInputPin("AmpMod");
        SetName("Oscillator");
        SetParameter("frequency", 440.0f);
        SetParameter("amplitude", 1.0f);
        SetParameter("pulseWidth", 0.5f);
        SetParameter("waveform", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        float freq = GetParameter("frequency");
        float amp = GetParameter("amplitude");
        float pw = std::clamp(GetParameter("pulseWidth"), 0.05f, 0.95f);

        AudioBuffer* freqMod = ReadInput("FreqMod");
        AudioBuffer* ampMod = ReadInput("AmpMod");

        uint32_t channels = output.GetChannels();
        uint32_t frames = output.GetFrames();

        for (uint32_t f = 0; f < frames; ++f) {
            float modFreq = freqMod ? freqMod->GetChannel(0)[f] : 0.0f;
            float modAmp = ampMod ? ampMod->GetChannel(0)[f] : 0.0f;
            float currentFreq = std::max(0.1f, freq + modFreq * 1000.0f);
            float currentAmp = std::clamp(amp + modAmp, 0.0f, 1.0f);

            float phase = m_phase;

            for (uint32_t c = 0; c < channels; ++c) {
                float sample = 0.0f;
                switch (m_waveform) {
                    case Waveform::Sine:
                        sample = sinf(2.0f * M_PI * phase);
                        break;
                    case Waveform::Square:
                        sample = (phase < pw) ? 1.0f : -1.0f;
                        break;
                    case Waveform::Saw:
                        sample = 2.0f * phase - 1.0f;
                        break;
                    case Waveform::Triangle:
                        sample = 4.0f * fabsf(phase - 0.5f) - 1.0f;
                        break;
                    case Waveform::Noise:
                        sample = m_noiseDist(m_rng) * 2.0f - 1.0f;
                        break;
                }
                output.GetChannel(c)[f] = sample * currentAmp;
            }

            m_phase += currentFreq / ctx.sampleRate;
            if (m_phase >= 1.0f) m_phase -= 1.0f;
        }
    }

    void Reset() override { m_phase = 0.0f; }

private:
    Waveform m_waveform = Waveform::Sine;
    float m_phase = 0.0f;
    std::mt19937 m_rng{42};
    std::uniform_real_distribution<float> m_noiseDist{0.0f, 1.0f};
};

} // namespace Prisma::Audio::DSP
