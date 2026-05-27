#pragma once

#include "Export.h"
#include "AnimationClip.h"
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <limits>

namespace Prisma {
namespace Animation {

class ENGINE_API AnimState {
public:
    std::string  name;
    AnimationClip* clip = nullptr;
    double       speed = 1.0;
    bool         looping = true;
    double       blendInTime = 0.1;
    double       blendOutTime = 0.1;

    AnimState() = default;
    AnimState(const std::string& stateName, AnimationClip* animClip)
        : name(stateName), clip(animClip) {}
};

using TransitionCondition = std::function<bool()>;

class ENGINE_API AnimTransition {
public:
    std::string         name;
    uint32_t            targetStateIndex = UINT32_MAX;
    TransitionCondition condition;
    double              transitionDuration = 0.25;

    AnimTransition() = default;
    AnimTransition(uint32_t target, TransitionCondition cond, double duration = 0.25)
        : targetStateIndex(target), condition(std::move(cond)), transitionDuration(duration) {}
};

class ENGINE_API AnimStateMachine {
public:
    AnimStateMachine() = default;
    ~AnimStateMachine() = default;

    AnimStateMachine(const AnimStateMachine&) = delete;
    AnimStateMachine& operator=(const AnimStateMachine&) = delete;
    AnimStateMachine(AnimStateMachine&&) noexcept = default;
    AnimStateMachine& operator=(AnimStateMachine&&) noexcept = default;

    uint32_t AddState(const AnimState& state);
    uint32_t AddTransition(uint32_t fromState, const AnimTransition& transition);

    void SetCurrentState(uint32_t stateIndex);
    uint32_t GetCurrentStateIndex() const { return m_currentStateIndex; }

    void Update(double deltaTime, std::vector<BoneTransform>& outputTransforms);

    AnimState& GetState(uint32_t index);
    const AnimState& GetState(uint32_t index) const;
    size_t GetStateCount() const { return m_states.size(); }

    bool IsTransitioning() const { return m_isTransitioning; }
    double GetTransitionProgress() const { return m_transitionProgress; }
    const std::string& GetCurrentStateName() const;

    void Reset();

private:
    bool EvaluateTransitions(uint32_t& outTarget, double& outDuration);

    void SampleState(uint32_t stateIndex, double time,
                     std::vector<BoneTransform>& outTransforms) const;

    std::vector<AnimState> m_states;
    std::vector<std::vector<AnimTransition>> m_transitions;

    uint32_t m_currentStateIndex = UINT32_MAX;
    uint32_t m_previousStateIndex = UINT32_MAX;

    double m_currentTime = 0.0;
    double m_previousTime = 0.0;

    bool   m_isTransitioning = false;
    double m_transitionProgress = 0.0;
    double m_transitionDuration = 0.0;

    std::vector<BoneTransform> m_blendBuffer;
};

// ============================================================================
// 内联实现
// ============================================================================

inline uint32_t AnimStateMachine::AddState(const AnimState& state) {
    uint32_t index = static_cast<uint32_t>(m_states.size());
    m_states.push_back(state);
    m_transitions.emplace_back();
    return index;
}

inline uint32_t AnimStateMachine::AddTransition(
    uint32_t fromState, const AnimTransition& transition)
{
    if (fromState >= m_states.size()) return UINT32_MAX;
    m_transitions[fromState].push_back(transition);
    return static_cast<uint32_t>(m_transitions[fromState].size() - 1);
}

inline void AnimStateMachine::SetCurrentState(uint32_t stateIndex) {
    if (stateIndex >= m_states.size()) return;
    m_previousStateIndex = m_currentStateIndex;
    m_currentStateIndex = stateIndex;
    m_currentTime = 0.0;
    m_isTransitioning = false;
    m_transitionProgress = 0.0;
}

inline AnimState& AnimStateMachine::GetState(uint32_t index) {
    return m_states[index];
}

inline const AnimState& AnimStateMachine::GetState(uint32_t index) const {
    return m_states[index];
}

inline const std::string& AnimStateMachine::GetCurrentStateName() const {
    if (m_currentStateIndex < m_states.size())
        return m_states[m_currentStateIndex].name;
    static const std::string s_empty;
    return s_empty;
}

inline void AnimStateMachine::Reset() {
    m_currentStateIndex = UINT32_MAX;
    m_previousStateIndex = UINT32_MAX;
    m_currentTime = 0.0;
    m_previousTime = 0.0;
    m_isTransitioning = false;
    m_transitionProgress = 0.0;
    m_blendBuffer.clear();
}

inline bool AnimStateMachine::EvaluateTransitions(
    uint32_t& outTarget, double& outDuration)
{
    if (m_currentStateIndex >= m_states.size()) return false;

    for (const auto& trans : m_transitions[m_currentStateIndex]) {
        if (trans.condition && trans.condition()) {
            outTarget = trans.targetStateIndex;
            outDuration = trans.transitionDuration;
            return true;
        }
    }
    return false;
}

inline void AnimStateMachine::SampleState(
    uint32_t stateIndex, double time,
    std::vector<BoneTransform>& outTransforms) const
{
    if (stateIndex >= m_states.size()) return;
    const auto& state = m_states[stateIndex];
    if (!state.clip) return;

    double effectiveTime = time * state.speed;
    double duration = state.clip->GetDuration();

    if (duration > 0.0) {
        if (state.looping) {
            effectiveTime = glm::mod(effectiveTime, duration);
        } else {
            effectiveTime = glm::clamp(effectiveTime, 0.0, duration);
        }
    }

    state.clip->Sample(effectiveTime, outTransforms);
}

inline void AnimStateMachine::Update(
    double deltaTime, std::vector<BoneTransform>& outputTransforms)
{
    if (m_currentStateIndex >= m_states.size()) return;

    if (!m_isTransitioning) {
        uint32_t targetState = UINT32_MAX;
        double transitionDuration = 0.0;
        if (EvaluateTransitions(targetState, transitionDuration)) {
            if (targetState < m_states.size() && targetState != m_currentStateIndex) {
                m_previousStateIndex = m_currentStateIndex;
                m_previousTime = m_currentTime;
                m_currentStateIndex = targetState;
                m_currentTime = 0.0;
                m_isTransitioning = true;
                m_transitionProgress = 0.0;
                m_transitionDuration = std::max(transitionDuration, 0.001);
            }
        }
    }

    if (m_isTransitioning) {
        m_transitionProgress += deltaTime / m_transitionDuration;
        if (m_transitionProgress >= 1.0) {
            m_transitionProgress = 1.0;
            m_isTransitioning = false;
        }

        size_t boneCount = outputTransforms.size();
        if (m_blendBuffer.size() != boneCount) {
            m_blendBuffer.resize(boneCount);
        }

        std::fill(outputTransforms.begin(), outputTransforms.end(), BoneTransform{});
        std::fill(m_blendBuffer.begin(), m_blendBuffer.end(), BoneTransform{});

        double prevTime = m_previousTime;
        if (m_previousStateIndex < m_states.size()) {
            const auto& prevState = m_states[m_previousStateIndex];
            if (prevState.clip && prevState.clip->GetDuration() > 0.0) {
                prevTime += deltaTime;
                if (prevState.looping) {
                    prevTime = glm::mod(prevTime, prevState.clip->GetDuration());
                } else {
                    prevTime = glm::clamp(prevTime, 0.0, prevState.clip->GetDuration());
                }
            }
        }
        m_previousTime = prevTime;

        SampleState(m_previousStateIndex, m_previousTime, m_blendBuffer);
        SampleState(m_currentStateIndex,  m_currentTime,  outputTransforms);

        double blendT = m_transitionProgress;
        for (size_t i = 0; i < boneCount; ++i) {
            outputTransforms[i] = BoneTransform::Lerp(
                m_blendBuffer[i], outputTransforms[i], blendT);
        }

        m_currentTime += deltaTime;

    } else {
        std::fill(outputTransforms.begin(), outputTransforms.end(), BoneTransform{});
        SampleState(m_currentStateIndex, m_currentTime, outputTransforms);
        m_currentTime += deltaTime;
    }
}

} // namespace Animation
} // namespace Prisma
