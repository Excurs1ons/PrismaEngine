#pragma once

#include <audio/dsp/AudioNode.h>
#include <audio/dsp/nodes/OscillatorNode.h>
#include <audio/dsp/nodes/ADSRNode.h>
#include <audio/dsp/nodes/SVFNode.h>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Prisma::Audio::DSP {

struct SynthVoice {
    uint32_t id = 0;
    uint8_t note = 69;
    float velocity = 1.0f;
    float frequency = 440.0f;
    bool active = false;
    uint64_t startFrame = 0;
};

class SynthVoiceManager : public AudioNode {
public:
    SynthVoiceManager() {
        AddOutputPin("Output");
        SetName("SynthVoiceManager");
        SetParameter("maxVoices", 16.0f);
        SetParameter("volume", 1.0f);
        SetParameter("unison", 1.0f);
        SetParameter("detune", 0.0f);
    }

    uint32_t NoteOn(uint8_t note, float velocity = 1.0f) {
        uint32_t maxVoices = static_cast<uint32_t>(GetParameter("maxVoices"));

        // Find inactive voice or oldest active
        uint32_t voiceId = 0;
        for (auto& v : m_voices) {
            if (!v.active) { voiceId = v.id; break; }
        }
        if (voiceId == 0 && m_voices.size() < maxVoices) {
            voiceId = m_voices.size() + 1;
            m_voices.push_back({voiceId, note, velocity, FreqFromNote(note)});
        }
        if (voiceId == 0) return 0;

        auto& voice = m_voices[voiceId - 1];
        voice.active = true;
        voice.note = note;
        voice.velocity = velocity;
        voice.frequency = FreqFromNote(note);
        voice.startFrame = m_totalFrames;

        // Trigger ADSR if connected
        // In a full implementation, we'd route to per-voice ADSR nodes
        return voiceId;
    }

    void NoteOff(uint8_t note) {
        for (auto& v : m_voices) {
            if (v.active && v.note == note) {
                v.active = false;
            }
        }
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        float vol = GetParameter("volume");
        float unison = std::max(1.0f, GetParameter("unison"));
        float detune = GetParameter("detune");

        output.Clear();
        m_totalFrames += ctx.frames;

        uint32_t channels = output.GetChannels();
        uint32_t frames = output.GetFrames();

        if (m_scratch.size() < channels * frames) {
            m_scratch.resize(channels * frames);
        }

        for (auto& voice : m_voices) {
            if (!voice.active) continue;

            std::fill(m_scratch.begin(), m_scratch.end(), 0.0f);

            for (uint32_t u = 0; u < static_cast<uint32_t>(unison); ++u) {
                float detuneOffset = (u == 0) ? 0.0f : ((u % 2 == 0 ? 1 : -1) * detune * ((u + 1) / 2));
                float freq = voice.frequency * powf(2.0f, detuneOffset / 1200.0f);

                for (uint32_t f = 0; f < frames; ++f) {
                    float phase = fmodf(m_phase[u] + freq * f / ctx.sampleRate, 1.0f);
                    float sample = sinf(2.0f * M_PI * phase);
                    float amp = voice.velocity * vol / (unison > 1.0f ? unison : 1.0f);

                    for (uint32_t c = 0; c < channels; ++c) {
                        m_scratch[c * frames + f] += sample * amp;
                    }
                }
                m_phase[u] = fmodf(m_phase[u] + freq * frames / ctx.sampleRate, 1.0f);
            }

            for (uint32_t c = 0; c < channels; ++c) {
                float* dst = output.GetChannel(c);
                for (uint32_t f = 0; f < frames; ++f) {
                    dst[f] += m_scratch[c * frames + f];
                }
            }
        }
    }

    void Reset() override {
        for (auto& v : m_voices) v.active = false;
        std::fill(m_phase.begin(), m_phase.end(), 0.0f);
    }

private:
    static float FreqFromNote(uint8_t note) {
        return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f);
    }

    std::vector<SynthVoice> m_voices;
    std::vector<float> m_scratch;
    std::array<float, 8> m_phase = {};
    uint64_t m_totalFrames = 0;
};

} // namespace Prisma::Audio::DSP
