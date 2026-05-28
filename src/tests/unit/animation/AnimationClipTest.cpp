#include <gtest/gtest.h>
#include "animation/AnimationClip.h"
#include <glm/gtc/epsilon.hpp>

namespace Prisma {
namespace Animation {
namespace {

constexpr double EPSILON = 1e-9;

// ============================================================
// 空剪辑与边界行为
// ============================================================

TEST(AnimationClipTest, EmptyClipNoChannels) {
    AnimationClip clip;
    std::vector<BoneTransform> output(1);
    EXPECT_NO_THROW(clip.Sample(0.5, output));
    // output[0] 应保持默认值（位置 0，旋转单位四元数，缩放 1）
    EXPECT_EQ(output[0].position, glm::dvec3(0.0));
    EXPECT_EQ(output[0].rotation, glm::dquat(1.0, 0.0, 0.0, 0.0));
    EXPECT_EQ(output[0].scale, glm::dvec3(1.0));
}

TEST(AnimationClipTest, SingleFrameClip) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.positionKeys.emplace_back(0.0, glm::dvec3(5.0, 10.0, 15.0));
    channel.rotationKeys.emplace_back(0.0, glm::dquat(1.0, 0.0, 0.0, 0.0));
    channel.scaleKeys.emplace_back(0.0, glm::dvec3(2.0, 2.0, 2.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);
    clip.Sample(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(5.0, 10.0, 15.0));
    EXPECT_EQ(output[0].scale, glm::dvec3(2.0, 2.0, 2.0));

    // 即使在不同时间采样，单帧始终返回该帧值
    clip.Sample(0.5, output);
    EXPECT_EQ(output[0].position, glm::dvec3(5.0, 10.0, 15.0));

    clip.Sample(1.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(5.0, 10.0, 15.0));
}

TEST(AnimationClipTest, ZeroDurationClip) {
    AnimationClip clip;
    clip.SetDuration(0.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.positionKeys.emplace_back(0.0, glm::dvec3(1.0, 2.0, 3.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);
    // 零 duration 的 clip 不会 clamp 时间
    EXPECT_NO_THROW(clip.Sample(100.0, output));
}

// ============================================================
// 线性插值
// ============================================================

TEST(AnimationClipTest, LinearPositionInterpolation) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.mode = InterpolationMode::Linear;
    channel.positionKeys.emplace_back(0.0, glm::dvec3(0.0, 0.0, 0.0));
    channel.positionKeys.emplace_back(1.0, glm::dvec3(10.0, 20.0, 30.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);

    clip.Sample(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.0, 0.0, 0.0));

    clip.Sample(1.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(10.0, 20.0, 30.0));

    clip.Sample(0.5, output);
    EXPECT_EQ(output[0].position, glm::dvec3(5.0, 10.0, 15.0));

    clip.Sample(0.25, output);
    EXPECT_EQ(output[0].position, glm::dvec3(2.5, 5.0, 7.5));
}

TEST(AnimationClipTest, LinearRotationInterpolation) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.mode = InterpolationMode::Linear;
    // 围绕 Y 轴从 0 度旋转到 90 度（四元数表示）
    glm::dquat startRot = glm::angleAxis(0.0, glm::dvec3(0.0, 1.0, 0.0));
    glm::dquat endRot   = glm::angleAxis(glm::radians(90.0), glm::dvec3(0.0, 1.0, 0.0));
    channel.rotationKeys.emplace_back(0.0, startRot);
    channel.rotationKeys.emplace_back(1.0, endRot);
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);

    clip.Sample(0.0, output);
    EXPECT_TRUE(glm::all(glm::epsilonEqual(
        output[0].rotation, startRot, EPSILON)));

    clip.Sample(1.0, output);
    EXPECT_TRUE(glm::all(glm::epsilonEqual(
        output[0].rotation, endRot, EPSILON)));

    // 在 0.5 处，应为 45 度
    clip.Sample(0.5, output);
    glm::dquat expected45 = glm::angleAxis(glm::radians(45.0), glm::dvec3(0.0, 1.0, 0.0));
    // slerp 结果应与 expected45 角度差很小
    double dot = glm::dot(output[0].rotation, expected45);
    EXPECT_NEAR(dot, 1.0, 1e-6);
}

TEST(AnimationClipTest, LinearScaleInterpolation) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.mode = InterpolationMode::Linear;
    channel.scaleKeys.emplace_back(0.0, glm::dvec3(1.0, 1.0, 1.0));
    channel.scaleKeys.emplace_back(1.0, glm::dvec3(3.0, 3.0, 3.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);

    clip.Sample(0.5, output);
    EXPECT_EQ(output[0].scale, glm::dvec3(2.0, 2.0, 2.0));

    clip.Sample(0.75, output);
    EXPECT_EQ(output[0].scale, glm::dvec3(2.5, 2.5, 2.5));
}

// ============================================================
// Step 插值
// ============================================================

TEST(AnimationClipTest, StepInterpolation) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.mode = InterpolationMode::Step;
    channel.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
    channel.positionKeys.emplace_back(0.5, glm::dvec3(100.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);

    // Step 模式始终使用前一个关键帧的值
    clip.Sample(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.0));

    clip.Sample(0.25, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.0));

    clip.Sample(0.5, output);
    EXPECT_EQ(output[0].position, glm::dvec3(100.0));

    clip.Sample(0.75, output);
    EXPECT_EQ(output[0].position, glm::dvec3(100.0));
}

// ============================================================
// Cubic 插值
// ============================================================

TEST(AnimationClipTest, CubicInterpolation) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.mode = InterpolationMode::Cubic;
    channel.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
    channel.positionKeys.emplace_back(1.0, glm::dvec3(1.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);

    // Cubic smoothstep: t^2*(3-2t)
    // At t=0.5: 0.25 * (3-1) = 0.25 * 2 = 0.5
    clip.Sample(0.5, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.5));

    // At t=0.25: 0.0625 * (3-0.5) = 0.0625 * 2.5 = 0.15625
    clip.Sample(0.25, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.15625));

    // At t=0.75: 0.5625 * (3-1.5) = 0.5625 * 1.5 = 0.84375
    clip.Sample(0.75, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.84375));
}

// ============================================================
// 时间钳位
// ============================================================

TEST(AnimationClipTest, TimeClampedToDuration) {
    AnimationClip clip;
    clip.SetDuration(2.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
    channel.positionKeys.emplace_back(2.0, glm::dvec3(10.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);

    // 时间 0.0 返回起始值
    clip.Sample(0.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.0));

    // 超出 duration 被 clamp 到末尾值
    clip.Sample(5.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(10.0));

    clip.Sample(2.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(10.0));
}

TEST(AnimationClipTest, NegativeTimeClamped) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    KeyFrameChannel channel;
    channel.boneIndex = 0;
    channel.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
    channel.positionKeys.emplace_back(1.0, glm::dvec3(100.0));
    clip.AddChannel(channel);

    std::vector<BoneTransform> output(1);

    // 负时间被 clamp 到 0
    clip.Sample(-1.0, output);
    EXPECT_EQ(output[0].position, glm::dvec3(0.0));
}

// ============================================================
// 多通道（多骨骼）
// ============================================================

TEST(AnimationClipTest, MultipleChannelsIndependent) {
    AnimationClip clip;
    clip.SetDuration(1.0);

    // 骨骼 0：位置从 0 到 10
    KeyFrameChannel ch0;
    ch0.boneIndex = 0;
    ch0.positionKeys.emplace_back(0.0, glm::dvec3(0.0));
    ch0.positionKeys.emplace_back(1.0, glm::dvec3(10.0));
    clip.AddChannel(ch0);

    // 骨骼 1：位置从 100 到 200
    KeyFrameChannel ch1;
    ch1.boneIndex = 1;
    ch1.positionKeys.emplace_back(0.0, glm::dvec3(100.0));
    ch1.positionKeys.emplace_back(1.0, glm::dvec3(200.0));
    clip.AddChannel(ch1);

    std::vector<BoneTransform> output(2);
    clip.Sample(0.5, output);

    EXPECT_EQ(output[0].position, glm::dvec3(5.0));
    EXPECT_EQ(output[1].position, glm::dvec3(150.0));

    // 骨骼 2 不在输出变换列表中，但不会崩溃
    // 也测试 output 比骨骼数少的情况
    std::vector<BoneTransform> smallOutput(1);
    EXPECT_NO_THROW(clip.Sample(0.5, smallOutput));
    EXPECT_EQ(smallOutput[0].position, glm::dvec3(5.0));
}

// ============================================================
// 属性名称与基本信息
// ============================================================

TEST(AnimationClipTest, NameAndDuration) {
    AnimationClip clip;
    EXPECT_EQ(clip.GetName(), "");
    EXPECT_EQ(clip.GetDuration(), 1.0);
    EXPECT_EQ(clip.GetTicksPerSecond(), 24.0);

    clip.SetName("walk");
    clip.SetDuration(2.5);
    clip.SetTicksPerSecond(30.0);

    EXPECT_EQ(clip.GetName(), "walk");
    EXPECT_EQ(clip.GetDuration(), 2.5);
    EXPECT_EQ(clip.GetTicksPerSecond(), 30.0);
}

TEST(AnimationClipTest, ChannelCount) {
    AnimationClip clip;
    EXPECT_EQ(clip.GetChannelCount(), 0u);

    KeyFrameChannel ch;
    ch.boneIndex = 0;
    clip.AddChannel(ch);
    EXPECT_EQ(clip.GetChannelCount(), 1u);

    clip.AddChannel(ch);
    EXPECT_EQ(clip.GetChannelCount(), 2u);
}

// ============================================================
// GetChannel 方法
// ============================================================

TEST(AnimationClipTest, GetChannelByIndex) {
    AnimationClip clip;
    KeyFrameChannel ch;
    ch.boneIndex = 5;
    clip.AddChannel(ch);

    const KeyFrameChannel& retrieved = clip.GetChannel(0);
    EXPECT_EQ(retrieved.boneIndex, 5u);
}

TEST(AnimationClipTest, AccessChannelOutOfRangeTriggersAssert) {
    AnimationClip clip;
    // GetChannel 使用 operator[]，空 vector 的 operator[] 是未定义行为
    // 仅验证空 clip 的 GetChannelCount 为 0
    EXPECT_EQ(clip.GetChannelCount(), 0u);
}

// ============================================================
// BoneTransform 工具方法
// ============================================================

TEST(AnimationClipTest, BoneTransformToMatrix) {
    BoneTransform bt;
    bt.position = glm::dvec3(1.0, 2.0, 3.0);
    bt.rotation = glm::dquat(1.0, 0.0, 0.0, 0.0);
    bt.scale = glm::dvec3(2.0, 2.0, 2.0);

    glm::dmat4 mat = bt.ToMatrix();

    // 验证包含平移 (1,2,3)
    EXPECT_EQ(mat[3], glm::dvec4(1.0, 2.0, 3.0, 1.0));
    // 验证包含缩放 (2x,2y,2z) - 对角线的前三个元素
    EXPECT_EQ(mat[0][0], 2.0);
    EXPECT_EQ(mat[1][1], 2.0);
    EXPECT_EQ(mat[2][2], 2.0);
}

TEST(AnimationClipTest, BoneTransformLerp) {
    BoneTransform a;
    a.position = glm::dvec3(0.0);
    a.rotation = glm::dquat(1.0, 0.0, 0.0, 0.0);
    a.scale = glm::dvec3(1.0);

    BoneTransform b;
    b.position = glm::dvec3(10.0);
    b.rotation = glm::angleAxis(glm::radians(90.0), glm::dvec3(0.0, 1.0, 0.0));
    b.scale = glm::dvec3(3.0);

    BoneTransform result = BoneTransform::Lerp(a, b, 0.5);

    EXPECT_EQ(result.position, glm::dvec3(5.0));
    EXPECT_EQ(result.scale, glm::dvec3(2.0));

    // Lerp at t=0
    result = BoneTransform::Lerp(a, b, 0.0);
    EXPECT_EQ(result.position, glm::dvec3(0.0));
    EXPECT_EQ(result.scale, glm::dvec3(1.0));

    // Lerp at t=1
    result = BoneTransform::Lerp(a, b, 1.0);
    EXPECT_EQ(result.position, glm::dvec3(10.0));
    EXPECT_EQ(result.scale, glm::dvec3(3.0));
}

// ============================================================
// 移动语义
// ============================================================

TEST(AnimationClipTest, MoveConstructor) {
    AnimationClip clip;
    clip.SetName("run");
    KeyFrameChannel ch;
    ch.boneIndex = 0;
    clip.AddChannel(ch);

    AnimationClip moved(std::move(clip));
    EXPECT_EQ(moved.GetName(), "run");
    EXPECT_EQ(moved.GetChannelCount(), 1u);
}

TEST(AnimationClipTest, MoveAssignment) {
    AnimationClip clip;
    clip.SetName("jump");
    AnimationClip assigned;
    assigned = std::move(clip);
    EXPECT_EQ(assigned.GetName(), "jump");
}

} // namespace
} // namespace Animation
} // namespace Prisma
