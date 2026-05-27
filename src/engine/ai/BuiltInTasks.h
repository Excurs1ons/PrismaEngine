#pragma once

#include "Export.h"
#include "ai/BehaviorTree.h"
#include "ai/Blackboard.h"
#include "Engine.h"
#include "Logger.h"
#include "navigation/NavigationSystem.h"
#include "navigation/NavAgentComponent.h"
#include "animation/AnimationSystem.h"
#include "animation/AnimComponent.h"
#include "animation/AnimStateMachine.h"

#include <string>
#include <glm/glm.hpp>

namespace Prisma {
namespace AI {

// ═══════════════════════════════════════════════════════════════════
// MoveTo — 移动到目标位置
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API MoveTo : public BTTask {
public:
    explicit MoveTo(std::string targetPosKey = "TargetPosition",
                    double tolerance = 0.5)
        : m_targetPosKey(std::move(targetPosKey))
        , m_tolerance(tolerance) {}

    Status Execute(Blackboard& blackboard, double dt) override {
        glm::dvec3* targetPos = blackboard.TryGet<glm::dvec3>(m_targetPosKey);
        if (!targetPos) {
            LOG_WARNING("AI", "MoveTo: 未找到目标位置键 '{}'", m_targetPosKey);
            return Status::Failure;
        }

        Navigation::NavAgentComponent* navAgent =
            blackboard.TryGetValue<Navigation::NavAgentComponent*>("NavAgent");
        if (!navAgent || !navAgent->IsValid()) {
            LOG_WARNING("AI", "MoveTo: 未找到有效的导航智能体组件");
            return Status::Failure;
        }

        Navigation::NavAgent& agent = navAgent->GetAgent();

        auto* navSystem = Engine::Get().GetNavigationSystem();
        if (!navSystem || !navSystem->HasNavMesh()) {
            return Status::Running;
        }

        if (!m_hasTarget) {
            agent.MoveTo(*targetPos,
                         navSystem->GetNavMesh(),
                         &navSystem->GetPathFinder(),
                         &navSystem->GetPathSmoother());
            m_hasTarget = true;
        }

        agent.Update(dt, navSystem->GetNavMesh());

        if (agent.HasArrived()) {
            m_hasTarget = false;
            agent.Stop();
            return Status::Success;
        }

        if (agent.IsStuck()) {
            m_hasTarget = false;
            agent.Stop();
            LOG_WARNING("AI", "MoveTo: 智能体卡住，放弃移动");
            return Status::Failure;
        }

        blackboard.Set<glm::dvec3>("CurrentPosition", agent.GetPosition());

        return Status::Running;
    }

    void Reset() override {
        m_hasTarget = false;
    }

    const char* GetName() const override { return "MoveTo"; }

private:
    std::string m_targetPosKey;
    double m_tolerance = 0.5;
    bool m_hasTarget = false;
};

// ═══════════════════════════════════════════════════════════════════
// Wait — 等待指定时长
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API Wait : public BTTask {
public:
    explicit Wait(double duration)
        : m_duration(duration) {}

    Status Execute(Blackboard& /*blackboard*/, double dt) override {
        m_elapsed += dt;
        if (m_elapsed >= m_duration) {
            m_elapsed = 0.0;
            return Status::Success;
        }
        return Status::Running;
    }

    void Reset() override {
        m_elapsed = 0.0;
    }

    const char* GetName() const override { return "Wait"; }

private:
    double m_duration = 0.0;
    double m_elapsed = 0.0;
};

// ═══════════════════════════════════════════════════════════════════
// PlayAnimation — 播放动画（触发式，播放后立即返回成功）
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API PlayAnimation : public BTTask {
public:
    explicit PlayAnimation(std::string animNameKey = "AnimationName")
        : m_animNameKey(std::move(animNameKey)) {}

    Status Execute(Blackboard& blackboard, double /*dt*/) override {
        std::string* animName = blackboard.TryGet<std::string>(m_animNameKey);
        if (!animName) {
            LOG_WARNING("AI", "PlayAnimation: 未找到动画名称键 '{}'", m_animNameKey);
            return Status::Failure;
        }

        if (m_hasStarted && m_playedAnimationName == *animName) {
            // 已在播放该动画，返回成功
            return Status::Success;
        }

        Animation::AnimComponent* animComp =
            blackboard.TryGetValue<Animation::AnimComponent*>("AnimComponent");
        if (!animComp || !animComp->stateMachine) {
            LOG_WARNING("AI", "PlayAnimation: 未找到有效的动画组件");
            return Status::Failure;
        }

        Animation::AnimStateMachine& asm_ = *animComp->stateMachine;

        // 在状态机中查找指定名称的状态
        uint32_t stateIndex = UINT32_MAX;
        for (uint32_t i = 0; i < static_cast<uint32_t>(asm_.GetStateCount()); ++i) {
            const auto& state = asm_.GetState(i);
            if (state.name == *animName) {
                stateIndex = i;
                break;
            }
        }

        if (stateIndex == UINT32_MAX) {
            LOG_WARNING("AI", "PlayAnimation: 未找到名为 '{}' 的动画状态", *animName);
            return Status::Failure;
        }

        // 设置为当前状态
        asm_.SetCurrentState(stateIndex);
        m_hasStarted = true;
        m_playedAnimationName = *animName;

        return Status::Success;
    }

    void Reset() override {
        m_hasStarted = false;
        m_playedAnimationName.clear();
    }

    const char* GetName() const override { return "PlayAnimation"; }

private:
    std::string m_animNameKey;
    bool m_hasStarted = false;
    std::string m_playedAnimationName;
};

// ═══════════════════════════════════════════════════════════════════
// DebugLog — 打印黑板调试信息
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API DebugLog : public BTTask {
public:
    explicit DebugLog(std::string message)
        : m_message(std::move(message)) {}

    Status Execute(Blackboard& blackboard, double /*dt*/) override {
        std::string resolved = ResolvePlaceholders(m_message, blackboard);
        LOG_INFO("AI", "{}", resolved);
        return Status::Success;
    }

    const char* GetName() const override { return "DebugLog"; }

private:
    std::string m_message;

    static std::string ResolvePlaceholders(const std::string& message,
                                           Blackboard& blackboard) {
        std::string result = message;
        size_t pos = 0;
        while ((pos = result.find('{', pos)) != std::string::npos) {
            size_t endPos = result.find('}', pos);
            if (endPos == std::string::npos) break;

            std::string key = result.substr(pos + 1, endPos - pos - 1);

            std::string value;
            if (auto* strVal = blackboard.TryGet<std::string>(key)) {
                value = *strVal;
            } else if (auto* floatVal = blackboard.TryGet<float>(key)) {
                value = std::to_string(*floatVal);
            } else if (auto* doubleVal = blackboard.TryGet<double>(key)) {
                value = std::to_string(*doubleVal);
            } else if (auto* intVal = blackboard.TryGet<int>(key)) {
                value = std::to_string(*intVal);
            } else {
                value = "[unknown:" + key + "]";
            }

            result.replace(pos, endPos - pos + 1, value);
            pos += value.length();
        }
        return result;
    }
};

} // namespace AI
} // namespace Prisma
