#pragma once

#include "Export.h"
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Prisma {
namespace Animation {

/// 单个骨骼在某个时间点的变换（分解为位置/旋转/缩放）
struct BoneTransform {
    glm::dvec3 position{0.0};
    glm::dquat rotation{1.0, 0.0, 0.0, 0.0};
    glm::dvec3 scale{1.0};

    /// 构造变换矩阵
    glm::dmat4 ToMatrix() const {
        glm::dmat4 t = glm::translate(glm::dmat4(1.0), position);
        glm::dmat4 r = glm::toMat4(rotation);
        glm::dmat4 s = glm::scale(glm::dmat4(1.0), scale);
        return t * r * s;
    }

    static BoneTransform Lerp(const BoneTransform& a, const BoneTransform& b, double t) {
        BoneTransform result;
        result.position = glm::mix(a.position, b.position, t);
        result.rotation = glm::slerp(a.rotation, b.rotation, t);
        result.scale    = glm::mix(a.scale, b.scale, t);
        return result;
    }
};

/// 泛型关键帧
template<typename T>
struct KeyFrame {
    double time = 0.0;
    T value{};

    KeyFrame() = default;
    KeyFrame(double t, const T& v) : time(t), value(v) {}
};

/// 插值模式
enum class InterpolationMode {
    Linear, ///< 线性插值
    Step,   ///< 步进（使用前一帧值）
    Cubic   ///< 三次平滑插值（smoothstep）
};

/// 通道：包含单个骨骼的所有位置/旋转/缩放关键帧
struct KeyFrameChannel {
    uint32_t boneIndex = UINT32_MAX;
    InterpolationMode mode = InterpolationMode::Linear;

    std::vector<KeyFrame<glm::dvec3>> positionKeys;
    std::vector<KeyFrame<glm::dquat>> rotationKeys;
    std::vector<KeyFrame<glm::dvec3>> scaleKeys;

    /// 在给定时间采样该通道，输出到 BoneTransform
    void Sample(double time, BoneTransform& outTransform) const;
};

/// 动画剪辑
class ENGINE_API AnimationClip {
public:
    AnimationClip() = default;
    ~AnimationClip() = default;

    AnimationClip(const AnimationClip&) = default;
    AnimationClip& operator=(const AnimationClip&) = default;
    AnimationClip(AnimationClip&&) noexcept = default;
    AnimationClip& operator=(AnimationClip&&) noexcept = default;

    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    double GetDuration() const { return m_duration; }
    void SetDuration(double duration) { m_duration = duration; }

    double GetTicksPerSecond() const { return m_ticksPerSecond; }
    void SetTicksPerSecond(double tps) { m_ticksPerSecond = tps; }

    /// 添加通道
    uint32_t AddChannel(const KeyFrameChannel& channel);

    /// 获取通道
    KeyFrameChannel& GetChannel(uint32_t index);
    const KeyFrameChannel& GetChannel(uint32_t index) const;

    /// 通道数量
    size_t GetChannelCount() const { return m_channels.size(); }

    /// 在给定时间采样动画
    /// outputTransforms 按骨骼索引索引，函数只填充有通道的骨骼
    void Sample(double time, std::vector<BoneTransform>& outputTransforms) const;

    /// 获取所有通道
    const std::vector<KeyFrameChannel>& GetChannels() const { return m_channels; }

private:
    std::string m_name;
    double m_duration = 1.0;
    double m_ticksPerSecond = 24.0;
    std::vector<KeyFrameChannel> m_channels;
};

// ============================================================================
// 内联实现
// ============================================================================

/// 辅助：查找包围给定时间的一对关键帧
template<typename T>
inline bool FindKeyframePair(
    const std::vector<KeyFrame<T>>& keys,
    double time,
    const KeyFrame<T>*& prev,
    const KeyFrame<T>*& next,
    double& t)
{
    if (keys.empty()) return false;

    if (keys.size() == 1 || time <= keys.front().time) {
        prev = next = &keys.front();
        t = 0.0;
        return true;
    }

    if (time >= keys.back().time) {
        prev = next = &keys.back();
        t = 0.0;
        return true;
    }

    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (time >= keys[i].time && time < keys[i + 1].time) {
            prev = &keys[i];
            next = &keys[i + 1];
            double range = next->time - prev->time;
            t = (range > 0.0) ? (time - prev->time) / range : 0.0;
            return true;
        }
    }

    return false;
}

inline void KeyFrameChannel::Sample(double time, BoneTransform& outTransform) const {
    // 采样位置
    if (!positionKeys.empty()) {
        const KeyFrame<glm::dvec3>* prev = nullptr;
        const KeyFrame<glm::dvec3>* next = nullptr;
        double t = 0.0;
        if (FindKeyframePair(positionKeys, time, prev, next, t)) {
            if (mode == InterpolationMode::Step || prev == next) {
                outTransform.position = prev->value;
            } else if (mode == InterpolationMode::Cubic) {
                // Smooth step: t^2 * (3 - 2t)
                double st = t * t * (3.0 - 2.0 * t);
                outTransform.position = glm::mix(prev->value, next->value, st);
            } else {
                outTransform.position = glm::mix(prev->value, next->value, t);
            }
        }
    }

    // 采样旋转
    if (!rotationKeys.empty()) {
        const KeyFrame<glm::dquat>* prev = nullptr;
        const KeyFrame<glm::dquat>* next = nullptr;
        double t = 0.0;
        if (FindKeyframePair(rotationKeys, time, prev, next, t)) {
            if (mode == InterpolationMode::Step || prev == next) {
                outTransform.rotation = prev->value;
            } else if (mode == InterpolationMode::Cubic) {
                double st = t * t * (3.0 - 2.0 * t);
                outTransform.rotation = glm::slerp(prev->value, next->value, st);
            } else {
                outTransform.rotation = glm::slerp(prev->value, next->value, t);
            }
        }
    }

    // 采样缩放
    if (!scaleKeys.empty()) {
        const KeyFrame<glm::dvec3>* prev = nullptr;
        const KeyFrame<glm::dvec3>* next = nullptr;
        double t = 0.0;
        if (FindKeyframePair(scaleKeys, time, prev, next, t)) {
            if (mode == InterpolationMode::Step || prev == next) {
                outTransform.scale = prev->value;
            } else if (mode == InterpolationMode::Cubic) {
                double st = t * t * (3.0 - 2.0 * t);
                outTransform.scale = glm::mix(prev->value, next->value, st);
            } else {
                outTransform.scale = glm::mix(prev->value, next->value, t);
            }
        }
    }
}

inline uint32_t AnimationClip::AddChannel(const KeyFrameChannel& channel) {
    uint32_t index = static_cast<uint32_t>(m_channels.size());
    m_channels.push_back(channel);
    return index;
}

inline KeyFrameChannel& AnimationClip::GetChannel(uint32_t index) {
    return m_channels[index];
}

inline const KeyFrameChannel& AnimationClip::GetChannel(uint32_t index) const {
    return m_channels[index];
}

inline void AnimationClip::Sample(double time, std::vector<BoneTransform>& outputTransforms) const {
    if (m_duration > 0.0) {
        time = glm::clamp(time, 0.0, m_duration);
    }

    for (const auto& channel : m_channels) {
        if (channel.boneIndex < outputTransforms.size()) {
            channel.Sample(time, outputTransforms[channel.boneIndex]);
        }
    }
}

} // namespace Animation
} // namespace Prisma
