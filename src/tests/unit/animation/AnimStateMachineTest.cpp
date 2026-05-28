#include <gtest/gtest.h>
#include "animation/AnimStateMachine.h"
#include <memory>

namespace Prisma {
namespace Animation {
namespace {

// ============================================================
// 辅助：创建带简单动画剪辑的状态
// ============================================================

struct TestClips {
    AnimationClip idleClip;
    AnimationClip walkClip;
    AnimationClip runClip;
    AnimationClip jumpClip;

    TestClips() {
        idleClip.SetName("idle");
        idleClip.SetDuration(2.0);
        KeyFrameChannel idleCh;
        idleCh.boneIndex = 0;
        idleCh.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
        idleCh.positionKeys.emplace_back(2.0, glm::dvec3(0.0));
        idleClip.AddChannel(idleCh);

        walkClip.SetName("walk");
        walkClip.SetDuration(1.0);
        KeyFrameChannel walkCh;
        walkCh.boneIndex = 0;
        walkCh.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
        walkCh.positionKeys.emplace_back(1.0, glm::dvec3(5.0));
        walkClip.AddChannel(walkCh);

        runClip.SetName("run");
        runClip.SetDuration(0.5);
        KeyFrameChannel runCh;
        runCh.boneIndex = 0;
        runCh.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
        runCh.positionKeys.emplace_back(0.5, glm::dvec3(10.0));
        runClip.AddChannel(runCh);

        jumpClip.SetName("jump");
        jumpClip.SetDuration(0.3);
    }
};

// ============================================================
// 状态添加与管理
// ============================================================

TEST(AnimStateMachineTest, AddStateReturnsIndex) {
    AnimStateMachine asm_;
    TestClips clips;

    uint32_t idx0 = asm_.AddState(AnimState("idle", &clips.idleClip));
    uint32_t idx1 = asm_.AddState(AnimState("walk", &clips.walkClip));

    EXPECT_EQ(idx0, 0u);
    EXPECT_EQ(idx1, 1u);
    EXPECT_EQ(asm_.GetStateCount(), 2u);
}

TEST(AnimStateMachineTest, GetStateProperties) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));

    const AnimState& state0 = asm_.GetState(0);
    EXPECT_EQ(state0.name, "idle");
    EXPECT_EQ(state0.clip, &clips.idleClip);

    const AnimState& state1 = asm_.GetState(1);
    EXPECT_EQ(state1.name, "walk");
    EXPECT_EQ(state1.clip, &clips.walkClip);
}

// ============================================================
// SetCurrentState 与默认状态
// ============================================================

TEST(AnimStateMachineTest, DefaultStateIsInvalid) {
    const AnimStateMachine asm_;
    EXPECT_EQ(asm_.GetCurrentStateIndex(), UINT32_MAX);
    EXPECT_EQ(asm_.GetCurrentStateName(), "");
    EXPECT_FALSE(asm_.IsTransitioning());
}

TEST(AnimStateMachineTest, SetCurrentStateSwitchesState) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));

    asm_.SetCurrentState(0);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);
    EXPECT_EQ(asm_.GetCurrentStateName(), "idle");

    asm_.SetCurrentState(1);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);
    EXPECT_EQ(asm_.GetCurrentStateName(), "walk");
}

TEST(AnimStateMachineTest, SetCurrentStateInvalidIgnored) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.SetCurrentState(0);

    // 尝试切换到无效索引
    asm_.SetCurrentState(999);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);

    asm_.SetCurrentState(UINT32_MAX);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);
}

// ============================================================
// 状态更新：基本采样
// ============================================================

TEST(AnimStateMachineTest, UpdateWithoutClip) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("empty", nullptr));
    asm_.SetCurrentState(0);

    std::vector<BoneTransform> output(1);
    EXPECT_NO_THROW(asm_.Update(0.1, output));
    // 没有 clip，output 应保持默认值
    EXPECT_EQ(output[0].position, glm::dvec3(0.0));
}

TEST(AnimStateMachineTest, UpdateSamplesClip) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.SetCurrentState(0);

    // SetCurrentState 设置 m_currentTime=0，Update 在累加前采样
    // 先用 tmp 推进时间到 0.5，再用第二个调用检测
    std::vector<BoneTransform> tmp(1);
    std::vector<BoneTransform> output(1);
    asm_.Update(0.5, tmp);
    asm_.Update(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(2.5));
}

TEST(AnimStateMachineTest, UpdateAccumulatesTime) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.SetCurrentState(0);

    std::vector<BoneTransform> tmp(1);
    std::vector<BoneTransform> output(1);

    // 推进到 t=0.25
    asm_.Update(0.25, tmp);
    asm_.Update(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(1.25));

    // 再推进到 t=0.5
    asm_.Update(0.25, tmp);
    asm_.Update(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(2.5));
}

// ============================================================
// 循环与非循环模式
// ============================================================

TEST(AnimStateMachineTest, LoopingWrapsAround) {
    AnimStateMachine asm_;
    TestClips clips;

    AnimState walkState("walk", &clips.walkClip);
    walkState.looping = true;
    asm_.AddState(walkState);
    asm_.SetCurrentState(0);

    // 推进到 t=2.5，循环模式应回到 t=0.5
    std::vector<BoneTransform> tmp(1);
    std::vector<BoneTransform> output(1);
    asm_.Update(2.5, tmp);
    asm_.Update(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(2.5));
}

TEST(AnimStateMachineTest, NonLoopingClampsToEnd) {
    AnimStateMachine asm_;
    TestClips clips;

    AnimState walkState("walk", &clips.walkClip);
    walkState.looping = false;
    asm_.AddState(walkState);
    asm_.SetCurrentState(0);

    // 推进到 t=2.0，非循环模式应 clamp 到 t=1.0
    std::vector<BoneTransform> tmp(1);
    std::vector<BoneTransform> output(1);
    asm_.Update(2.0, tmp);
    asm_.Update(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(5.0));
}

// ============================================================
// 条件触发状态转换
// ============================================================

TEST(AnimStateMachineTest, TransitionOnConditionTrue) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.SetCurrentState(0);

    bool shouldWalk = false;
    // 使用较长的 transitionDuration 避免 deltaTime 在同帧完成转换
    AnimTransition trans(1, [&]() { return shouldWalk; }, 0.5);
    asm_.AddTransition(0, trans);

    std::vector<BoneTransform> output(1);

    // 条件为 false，不转换
    asm_.Update(0.1, output);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);
    EXPECT_FALSE(asm_.IsTransitioning());

    // 条件变为 true，触发转换
    shouldWalk = true;
    asm_.Update(0.1, output);
    EXPECT_TRUE(asm_.IsTransitioning());
    EXPECT_GT(asm_.GetTransitionProgress(), 0.0);
}

TEST(AnimStateMachineTest, TransitionCompletes) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.SetCurrentState(0);

    AnimTransition trans(1, []() { return true; }, 0.2);
    asm_.AddTransition(0, trans);

    std::vector<BoneTransform> output(1);

    // 触发转换
    asm_.Update(0.0, output);
    EXPECT_TRUE(asm_.IsTransitioning());

    // 前进到转换完成
    asm_.Update(0.3, output);
    EXPECT_FALSE(asm_.IsTransitioning());
    EXPECT_EQ(asm_.GetTransitionProgress(), 1.0);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);
}

TEST(AnimStateMachineTest, TransitionBlendsOutput) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.SetCurrentState(0);

    // idle 在 t=0 时位置为 0，walk 在 t=0 时位置也为 0
    // 但 idle 前进到 t=0.1 后位置仍为 0，walk 在 t=0 时为 0
    // 所以混合结果 = 0
    AnimTransition trans(1, []() { return true; }, 0.4);
    asm_.AddTransition(0, trans);

    std::vector<BoneTransform> output(1);
    asm_.Update(0.0, output);

    // 转换了一半（0.2 / 0.4 = 0.5 progress）
    asm_.Update(0.2, output);
    EXPECT_NEAR(asm_.GetTransitionProgress(), 0.5, 1e-6);
}

// ============================================================
// 转换到自身
// ============================================================

TEST(AnimStateMachineTest, TransitionToSelfIgnored) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.SetCurrentState(0);

    // 转换到自身（索引 0）
    AnimTransition transSelf(0, []() { return true; });
    asm_.AddTransition(0, transSelf);

    std::vector<BoneTransform> output(1);
    asm_.Update(0.1, output);

    // 转换到自身应被忽略（target == current 时跳过）
    EXPECT_FALSE(asm_.IsTransitioning());
}

// ============================================================
// 无效转换处理
// ============================================================

TEST(AnimStateMachineTest, TransitionToInvalidStateIgnored) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.SetCurrentState(0);

    // 转换到不存在的状态
    AnimTransition transBad(42, []() { return true; });
    asm_.AddTransition(0, transBad);

    std::vector<BoneTransform> output(1);
    asm_.Update(0.1, output);

    EXPECT_FALSE(asm_.IsTransitioning());
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);
}

TEST(AnimStateMachineTest, TransitionFromNonExistentState) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));

    // 从索引 999 添加转换应该返回 UINT32_MAX
    AnimTransition trans(0, []() { return true; });
    uint32_t result = asm_.AddTransition(999, trans);
    EXPECT_EQ(result, UINT32_MAX);
}

// ============================================================
// Reset
// ============================================================

TEST(AnimStateMachineTest, ResetClearsAll) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.SetCurrentState(0);
    std::vector<BoneTransform> resetOutput(1);
    asm_.Update(0.5, resetOutput);

    asm_.Reset();

    EXPECT_EQ(asm_.GetCurrentStateIndex(), UINT32_MAX);
    EXPECT_EQ(asm_.GetCurrentStateName(), "");
    EXPECT_FALSE(asm_.IsTransitioning());
    EXPECT_EQ(asm_.GetTransitionProgress(), 0.0);
}

// ============================================================
// 多状态链式转换
// ============================================================

TEST(AnimStateMachineTest, ChainedTransitions) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.AddState(AnimState("run", &clips.runClip));
    asm_.SetCurrentState(0);

    int phase = 0;
    AnimTransition toWalk(1, [&]() { return phase >= 1; });
    AnimTransition toRun(2,  [&]() { return phase >= 2; });
    asm_.AddTransition(0, toWalk);
    asm_.AddTransition(1, toRun);

    std::vector<BoneTransform> output(1);

    // phase 0: idle
    asm_.Update(0.1, output);
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 0u);

    // phase 1: 触发 idle -> walk 转换
    phase = 1;
    asm_.Update(0.1, output);
    asm_.Update(0.5, output); // 完成转换
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);

    // phase 2: 触发 walk -> run 转换
    phase = 2;
    asm_.Update(0.1, output);
    asm_.Update(0.5, output); // 完成转换
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 2u);
}

// ============================================================
// State Speed 属性
// ============================================================

TEST(AnimStateMachineTest, StateSpeedAffectsSampling) {
    AnimStateMachine asm_;
    TestClips clips;

    AnimState fastState("fast", &clips.walkClip);
    fastState.speed = 2.0;
    asm_.AddState(fastState);
    asm_.SetCurrentState(0);

    std::vector<BoneTransform> tmp(1);
    std::vector<BoneTransform> output(1);
    asm_.Update(0.25, tmp);
    asm_.Update(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(2.5));
}

// ============================================================
// 多个条件的转换
// ============================================================

TEST(AnimStateMachineTest, FirstValidTransitionChosen) {
    AnimStateMachine asm_;
    TestClips clips;

    asm_.AddState(AnimState("idle", &clips.idleClip));
    asm_.AddState(AnimState("walk", &clips.walkClip));
    asm_.AddState(AnimState("run", &clips.runClip));
    asm_.SetCurrentState(0);

    // 两个转换都返回 true，应选择第一个添加的
    AnimTransition toWalk(1, []() { return true; });
    AnimTransition toRun(2,  []() { return true; });
    asm_.AddTransition(0, toWalk);
    asm_.AddTransition(0, toRun);

    std::vector<BoneTransform> output(1);
    asm_.Update(0.1, output);
    asm_.Update(0.5, output); // 完成转换

    // 应转换到第一个匹配的：walk
    EXPECT_EQ(asm_.GetCurrentStateIndex(), 1u);
}

} // namespace
} // namespace Animation
} // namespace Prisma
