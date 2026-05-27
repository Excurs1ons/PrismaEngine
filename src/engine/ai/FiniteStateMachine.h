#pragma once

#include "Export.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace Prisma {
namespace AI {

// ═══════════════════════════════════════════════════════════════════
// FSMState — 有限状态机状态基类
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API FSMState {
public:
    virtual ~FSMState() = default;

    /** 进入状态时调用 */
    virtual void OnEnter() {}

    /** 每帧更新 */
    virtual void OnUpdate([[maybe_unused]] double dt) {}

    /** 退出状态时调用 */
    virtual void OnExit() {}

    /** 获取状态名称 */
    virtual const char* GetName() const { return "FSMState"; }
};

// ═══════════════════════════════════════════════════════════════════
// FSM — 有限状态机
// ═══════════════════════════════════════════════════════════════════

class ENGINE_API FSM {
public:
    FSM() = default;
    ~FSM() { Shutdown(); }

    FSM(const FSM&) = delete;
    FSM& operator=(const FSM&) = delete;

    FSM(FSM&&) = default;
    FSM& operator=(FSM&&) = default;

    // ========== 状态注册 ==========

    /**
     * @brief 注册状态
     * @param name 状态名称
     * @param state 状态实例
     */
    void AddState(const std::string& name, std::unique_ptr<FSMState> state) {
        m_states[name] = std::move(state);
    }

    /** 检查状态是否已注册 */
    bool HasState(const std::string& name) const {
        return m_states.find(name) != m_states.end();
    }

    /** 获取状态数量 */
    size_t GetStateCount() const { return m_states.size(); }

    // ========== 状态转换 ==========

    /**
     * @brief 转换到指定状态
     * @param name 目标状态名称
     * @param reason 转换原因（日志用）
     * @return 是否成功转换
     */
    bool TransitionTo(const std::string& name, const char* reason = nullptr) {
        auto it = m_states.find(name);
        if (it == m_states.end()) return false;
        if (m_currentStateName == name) return true; // 已在目标状态

        // 退出当前状态
        if (m_currentState) {
            m_currentState->OnExit();
        }

        // 记录转换日志
        m_lastTransitionReason = reason ? reason : "";
        m_previousStateName = m_currentStateName;

        // 进入新状态
        m_currentStateName = name;
        m_currentState = it->second.get();
        m_currentState->OnEnter();

        return true;
    }

    /**
     * @brief 每帧更新状态机
     * @param dt 帧时间差（秒）
     */
    void Update(double dt) {
        if (m_currentState) {
            m_currentState->OnUpdate(dt);
        }
    }

    // ========== 查询 ==========

    /** 获取当前状态名称 */
    const std::string& GetCurrentStateName() const { return m_currentStateName; }

    /** 获取前一状态名称 */
    const std::string& GetPreviousStateName() const { return m_previousStateName; }

    /** 获取当前状态指针 */
    FSMState* GetCurrentState() { return m_currentState; }
    const FSMState* GetCurrentState() const { return m_currentState; }

    /** 检查当前是否在指定状态 */
    bool IsInState(const std::string& name) const {
        return m_currentStateName == name;
    }

    /** 获取最后一次转换原因 */
    const std::string& GetLastTransitionReason() const { return m_lastTransitionReason; }

    /** 重置状态机 */
    void Reset() {
        if (m_currentState) {
            m_currentState->OnExit();
        }
        m_currentState = nullptr;
        m_currentStateName.clear();
        m_previousStateName.clear();
        m_lastTransitionReason.clear();
    }

    /** 清理所有状态 */
    void Shutdown() {
        Reset();
        m_states.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<FSMState>> m_states;
    FSMState* m_currentState = nullptr;
    std::string m_currentStateName;
    std::string m_previousStateName;
    std::string m_lastTransitionReason;
};

} // namespace AI
} // namespace Prisma
