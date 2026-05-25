#pragma once

#include <audio/dsp/AudioNode.h>
#include <cmath>
#include <algorithm>

namespace Prisma::Audio::DSP {

class ADSRNode : public AudioNode {
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    ADSRNode() {
        AddOutputPin("Output");
        AddInputPin("Gate");
        SetName("ADSR");
        SetParameter("attack", 0.1f);
        SetParameter("decay", 0.05f);
        SetParameter("sustain", 0.7f);
        SetParameter("release", 0.3f);
        SetParameter("gate", 0.0f);
    }

    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        float attackSec = std::max(0.001f, GetParameter("attack"));
        float decaySec = std::max(0.001f, GetParameter("decay"));
        float sustainLevel = std::clamp(GetParameter("sustain"), 0.0f, 1.0f);
        float releaseSec = std::max(0.001f, GetParameter("release"));

        AudioBuffer* gateInput = ReadInput("Gate");
        bool gateOn = gateInput ? (gateInput->GetChannel(0)[0] > 0.5f)
                                 : (GetParameter("gate") > 0.5f);

        if (gateOn && m_stage == Stage::Idle) {
            m_stage = Stage::Attack;
            m_currentLevel = 0.0f;
        } else if (!gateOn && (m_stage == Stage::Attack || m_stage == Stage::Decay || m_stage == Stage::Sustain)) {
            m_stage = Stage::Release;
            m_releaseStart = m_currentLevel;
        }

        float attackRate = 1.0f / (attackSec * ctx.sampleRate);
        float decayRate = 1.0f / (decaySec * ctx.sampleRate);
        float releaseRate = 1.0f / (releaseSec * ctx.sampleRate);

        uint32_t frames = output.GetFrames();
        uint32_t channels = output.GetChannels();

        for (uint32_t f = 0; f < frames; ++f) {
            switch (m_stage) {
                case Stage::Attack:
                    m_currentLevel += attackRate;
                    if (m_currentLevel >= 1.0f) {
                        m_currentLevel = 1.0f;
                        m_stage = Stage::Decay;
                    }
                    break;
                case Stage::Decay:
                    m_currentLevel -= decayRate;
                    if (m_currentLevel <= sustainLevel) {
                        m_currentLevel = sustainLevel;
                        m_stage = Stage::Sustain;
                    }
                    break;
                case Stage::Sustain:
                    m_currentLevel = sustainLevel;
                    break;
                case Stage::Release:
                    if (m_releaseStart > 0.0f) {
                        m_currentLevel -= releaseRate * m_releaseStart * 3.0f;
                    } else {
                        m_currentLevel -= releaseRate;
                    }
                    if (m_currentLevel <= 0.001f) {
                        m_currentLevel = 0.0f;
                        m_stage = Stage::Idle;
                    }
                    break;
                default:
                    m_currentLevel = 0.0f;
                    break;
            }

            for (uint32_t c = 0; c < channels; ++c)
                output.GetChannel(c)[f] = m_currentLevel;
        }
    }

    void Reset() override {
        m_stage = Stage::Idle;
        m_currentLevel = 0.0f;
    }

    Stage GetStage() const { return m_stage; }
    float GetCurrentLevel() const { return m_currentLevel; }
    void NoteOn() { m_stage = Stage::Attack; m_currentLevel = 0.0f; }
    void NoteOff() {
        if (m_stage != Stage::Idle) {
            m_stage = Stage::Release;
            m_releaseStart = m_currentLevel;
        }
    }

private:
    Stage m_stage = Stage::Idle;
    float m_currentLevel = 0.0f;
    float m_releaseStart = 0.0f;
};

} // namespace Prisma::Audio::DSP
