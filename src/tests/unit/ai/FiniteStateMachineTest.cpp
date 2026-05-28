#include <gtest/gtest.h>
#include "ai/FiniteStateMachine.h"

namespace Prisma {
namespace AI {
namespace {

// ═══════════════════════════════════════════════════════════════════
// CounterState — 追踪回调的测试状态
// ═══════════════════════════════════════════════════════════════════

class CounterState final : public FSMState {
public:
    explicit CounterState(const char* name = "CounterState")
        : m_name(name) {}

    void OnEnter() override { m_enterCount++; }
    void OnUpdate(double dt) override {
        m_updateCount++;
        m_lastDt = dt;
    }
    void OnExit() override { m_exitCount++; }
    const char* GetName() const override { return m_name; }

    int GetEnterCount() const { return m_enterCount; }
    int GetUpdateCount() const { return m_updateCount; }
    int GetExitCount() const { return m_exitCount; }
    double GetLastDt() const { return m_lastDt; }

    void ResetCounts() {
        m_enterCount = 0;
        m_updateCount = 0;
        m_exitCount = 0;
        m_lastDt = 0.0;
    }

private:
    const char* m_name;
    int m_enterCount = 0;
    int m_updateCount = 0;
    int m_exitCount = 0;
    double m_lastDt = 0.0;
};

// ═══════════════════════════════════════════════════════════════════
// 状态注册与基本属性
// ═══════════════════════════════════════════════════════════════════

TEST(FSM_StateRegistration, AddState_IncreasesCount) {
    FSM fsm;
    EXPECT_EQ(fsm.GetStateCount(), 0);

    fsm.AddState("Idle", std::make_unique<FSMState>());
    EXPECT_EQ(fsm.GetStateCount(), 1);

    fsm.AddState("Walk", std::make_unique<FSMState>());
    EXPECT_EQ(fsm.GetStateCount(), 2);
}

TEST(FSM_StateRegistration, HasState_AfterAdd_ReturnsTrue) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());

    EXPECT_TRUE(fsm.HasState("Idle"));
    EXPECT_FALSE(fsm.HasState("Walk"));
}

TEST(FSM_StateRegistration, HasState_NoState_ReturnsFalse) {
    FSM fsm;
    EXPECT_FALSE(fsm.HasState("NonExistent"));
}

TEST(FSM_StateRegistration, AddDuplicateState_Replaces) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<CounterState>("First"));
    fsm.AddState("Idle", std::make_unique<CounterState>("Second"));

    EXPECT_EQ(fsm.GetStateCount(), 1);
    EXPECT_TRUE(fsm.HasState("Idle"));
}

TEST(FSM_StateRegistration, GetCurrentState_InitiallyEmpty) {
    FSM fsm;
    EXPECT_EQ(fsm.GetCurrentState(), nullptr);
    EXPECT_TRUE(fsm.GetCurrentStateName().empty());
    EXPECT_TRUE(fsm.GetPreviousStateName().empty());
}

// ═══════════════════════════════════════════════════════════════════
// 状态转换
// ═══════════════════════════════════════════════════════════════════

TEST(FSM_Transitions, TransitionTo_ValidState_ReturnsTrue) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.AddState("Walk", std::make_unique<FSMState>());

    EXPECT_TRUE(fsm.TransitionTo("Idle"));
    EXPECT_TRUE(fsm.TransitionTo("Walk"));
}

TEST(FSM_Transitions, TransitionTo_InvalidState_ReturnsFalse) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.AddState("Walk", std::make_unique<FSMState>());

    EXPECT_FALSE(fsm.TransitionTo("NonExistent"));
}

TEST(FSM_Transitions, TransitionTo_SameState_ReturnsTrue) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.TransitionTo("Idle");

    // 已在 Idle 状态，再次转换应返回 true
    EXPECT_TRUE(fsm.TransitionTo("Idle"));
}

TEST(FSM_Transitions, IsInState_AfterTransition_ReturnsCorrectValue) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.AddState("Walk", std::make_unique<FSMState>());

    fsm.TransitionTo("Idle");
    EXPECT_TRUE(fsm.IsInState("Idle"));
    EXPECT_FALSE(fsm.IsInState("Walk"));

    fsm.TransitionTo("Walk");
    EXPECT_FALSE(fsm.IsInState("Idle"));
    EXPECT_TRUE(fsm.IsInState("Walk"));
}

TEST(FSM_Transitions, PreviousState_TracksLastState) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.AddState("Walk", std::make_unique<FSMState>());
    fsm.AddState("Run", std::make_unique<FSMState>());

    fsm.TransitionTo("Idle");
    EXPECT_TRUE(fsm.GetPreviousStateName().empty());

    fsm.TransitionTo("Walk");
    EXPECT_EQ(fsm.GetPreviousStateName(), "Idle");

    fsm.TransitionTo("Run");
    EXPECT_EQ(fsm.GetPreviousStateName(), "Walk");
}

// ═══════════════════════════════════════════════════════════════════
// 状态回调 (OnEnter / OnUpdate / OnExit)
// ═══════════════════════════════════════════════════════════════════

TEST(FSM_Callbacks, OnEnter_CalledOnTransition) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto walk = std::make_unique<CounterState>("Walk");

    auto* rawIdle = idle.get();
    auto* rawWalk = walk.get();

    fsm.AddState("Idle", std::move(idle));
    fsm.AddState("Walk", std::move(walk));

    fsm.TransitionTo("Idle");
    EXPECT_EQ(rawIdle->GetEnterCount(), 1);
    EXPECT_EQ(rawWalk->GetEnterCount(), 0);

    fsm.TransitionTo("Walk");
    EXPECT_EQ(rawWalk->GetEnterCount(), 1);
}

TEST(FSM_Callbacks, OnExit_CalledOnTransition) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto walk = std::make_unique<CounterState>("Walk");

    auto* rawIdle = idle.get();
    fsm.AddState("Idle", std::move(idle));
    fsm.AddState("Walk", std::move(walk));

    fsm.TransitionTo("Idle");
    EXPECT_EQ(rawIdle->GetExitCount(), 0);

    fsm.TransitionTo("Walk");
    EXPECT_EQ(rawIdle->GetExitCount(), 1);
}

TEST(FSM_Callbacks, OnUpdate_CalledEachFrame) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto* rawIdle = idle.get();
    fsm.AddState("Idle", std::move(idle));

    fsm.TransitionTo("Idle");
    EXPECT_EQ(rawIdle->GetUpdateCount(), 0);

    fsm.Update(0.016);
    EXPECT_EQ(rawIdle->GetUpdateCount(), 1);

    fsm.Update(0.032);
    EXPECT_EQ(rawIdle->GetUpdateCount(), 2);
}

TEST(FSM_Callbacks, OnUpdate_PassesDeltaTime) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto* rawIdle = idle.get();
    fsm.AddState("Idle", std::move(idle));

    fsm.TransitionTo("Idle");
    fsm.Update(0.016);
    EXPECT_DOUBLE_EQ(rawIdle->GetLastDt(), 0.016);

    fsm.Update(0.042);
    EXPECT_DOUBLE_EQ(rawIdle->GetLastDt(), 0.042);
}

TEST(FSM_Callbacks, OnEnter_NotCalledWhenAlreadyInState) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto* rawIdle = idle.get();
    fsm.AddState("Idle", std::move(idle));

    fsm.TransitionTo("Idle");
    EXPECT_EQ(rawIdle->GetEnterCount(), 1);

    // 再次转换到同一个状态不应触发 OnEnter
    fsm.TransitionTo("Idle");
    EXPECT_EQ(rawIdle->GetEnterCount(), 1);
}

TEST(FSM_Callbacks, FullTransitionLifecycle) {
    // 验证完整的状态生命周期：Idle -> Walk -> Run
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto walk = std::make_unique<CounterState>("Walk");
    auto run  = std::make_unique<CounterState>("Run");

    auto* rawIdle = idle.get();
    auto* rawWalk = walk.get();
    auto* rawRun  = run.get();

    fsm.AddState("Idle", std::move(idle));
    fsm.AddState("Walk", std::move(walk));
    fsm.AddState("Run",  std::move(run));

    // Idle 进入
    fsm.TransitionTo("Idle");
    EXPECT_EQ(rawIdle->GetEnterCount(), 1);
    EXPECT_EQ(rawIdle->GetExitCount(),  0);

    // Idle -> Walk: Idle 退出, Walk 进入
    fsm.TransitionTo("Walk");
    EXPECT_EQ(rawIdle->GetExitCount(), 1);
    EXPECT_EQ(rawWalk->GetEnterCount(), 1);

    // Walk -> Run: Walk 退出, Run 进入
    fsm.TransitionTo("Run");
    EXPECT_EQ(rawWalk->GetExitCount(), 1);
    EXPECT_EQ(rawRun->GetEnterCount(), 1);

    // Run 还未退出
    EXPECT_EQ(rawRun->GetExitCount(), 0);
}

// ═══════════════════════════════════════════════════════════════════
// 非法转换处理
// ═══════════════════════════════════════════════════════════════════

TEST(FSM_IllegalTransitions, UnregisteredState_ReturnsFalse) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.TransitionTo("Idle");

    EXPECT_FALSE(fsm.TransitionTo("NonExistent"));
}

TEST(FSM_IllegalTransitions, InvalidTransition_KeepsCurrentState) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.AddState("Walk", std::make_unique<FSMState>());
    fsm.TransitionTo("Idle");

    // 尝试非法转换
    fsm.TransitionTo("NonExistent");

    // 应保持在 Idle 状态
    EXPECT_TRUE(fsm.IsInState("Idle"));
    EXPECT_EQ(fsm.GetCurrentStateName(), "Idle");
}

TEST(FSM_IllegalTransitions, InvalidTransition_DoesNotCallExit) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto* rawIdle = idle.get();
    fsm.AddState("Idle", std::move(idle));
    fsm.TransitionTo("Idle");

    int exitBefore = rawIdle->GetExitCount();
    fsm.TransitionTo("NonExistent");
    EXPECT_EQ(rawIdle->GetExitCount(), exitBefore);
}

TEST(FSM_IllegalTransitions, TransitionToNonexistent_FromNoState) {
    FSM fsm;
    // 没有注册任何状态，尝试转换
    EXPECT_FALSE(fsm.TransitionTo("AnyState"));
    EXPECT_EQ(fsm.GetCurrentState(), nullptr);
}

// ═══════════════════════════════════════════════════════════════════
// 状态机生命周期
// ═══════════════════════════════════════════════════════════════════

TEST(FSM_Lifecycle, Reset_ClearsStateAndCallsExit) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto* rawIdle = idle.get();
    fsm.AddState("Idle", std::move(idle));
    fsm.TransitionTo("Idle");

    fsm.Reset();

    EXPECT_EQ(fsm.GetCurrentState(), nullptr);
    EXPECT_TRUE(fsm.GetCurrentStateName().empty());
    EXPECT_TRUE(fsm.GetPreviousStateName().empty());
    EXPECT_EQ(rawIdle->GetExitCount(), 1);
}

TEST(FSM_Lifecycle, Shutdown_ClearsAllStatesAndCallsExit) {
    FSM fsm;
    auto idle = std::make_unique<CounterState>("Idle");
    auto walk = std::make_unique<CounterState>("Walk");
    auto* rawIdle = idle.get();
    fsm.AddState("Idle", std::move(idle));
    fsm.AddState("Walk", std::move(walk));
    fsm.TransitionTo("Idle");

    fsm.Shutdown();

    EXPECT_EQ(fsm.GetStateCount(), 0);
    EXPECT_EQ(rawIdle->GetExitCount(), 1);
}

TEST(FSM_Lifecycle, Update_WithoutTransition_DoesNothing) {
    FSM fsm;
    // 不转换到任何状态，Update 不应崩溃
    EXPECT_NO_THROW(fsm.Update(0.016));
    EXPECT_NO_THROW(fsm.Update(0.032));
}

TEST(FSM_Lifecycle, GetCurrentState_NullAfterShutdown) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<CounterState>("Idle"));
    fsm.TransitionTo("Idle");
    fsm.Shutdown();

    EXPECT_EQ(fsm.GetCurrentState(), nullptr);
}

// ═══════════════════════════════════════════════════════════════════
// 转换原因 (reason) 日志
// ═══════════════════════════════════════════════════════════════════

TEST(FSM_Transitions, TransitionReason_StoredAndRetrieved) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.AddState("Walk", std::make_unique<FSMState>());

    fsm.TransitionTo("Walk", "player_input");
    EXPECT_EQ(fsm.GetLastTransitionReason(), "player_input");
}

TEST(FSM_Transitions, TransitionReason_UpdatesOnEachTransition) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.AddState("Walk", std::make_unique<FSMState>());
    fsm.AddState("Run", std::make_unique<FSMState>());

    fsm.TransitionTo("Idle", "startup");
    EXPECT_EQ(fsm.GetLastTransitionReason(), "startup");

    fsm.TransitionTo("Walk", "player_input");
    EXPECT_EQ(fsm.GetLastTransitionReason(), "player_input");

    fsm.TransitionTo("Run", "sprint_pressed");
    EXPECT_EQ(fsm.GetLastTransitionReason(), "sprint_pressed");
}

TEST(FSM_Transitions, TransitionReason_Nullptr_StoresEmpty) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());

    fsm.TransitionTo("Idle", nullptr);
    EXPECT_TRUE(fsm.GetLastTransitionReason().empty());
}

TEST(FSM_Transitions, FailedTransition_DoesNotUpdateReason) {
    FSM fsm;
    fsm.AddState("Idle", std::make_unique<FSMState>());
    fsm.TransitionTo("Idle", "startup");

    fsm.TransitionTo("DoesNotExist", "invalid");
    EXPECT_EQ(fsm.GetLastTransitionReason(), "startup");
}

} // namespace
} // namespace AI
} // namespace Prisma
