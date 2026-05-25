#pragma once

#include <cmath>
#include <algorithm>
#include <cstdint>

namespace Prisma::Audio::DSP {

// ============================================================================
// ParameterBlock - 可平滑插值的音频参数容器
// ============================================================================
class ParameterBlock {
public:
    ParameterBlock() = default;

    explicit ParameterBlock(float value)
        : m_value(value)
        , m_target(value)
        , m_ramping(false)
    {
    }

    /// 立即设置参数值（无平滑）
    void SetValue(float value) {
        m_value  = value;
        m_target = value;
        m_ramping = false;
    }

    /// 设置参数目标值，启动平滑过渡
    void SetSmooth(float value, uint32_t rampFrames) {
        if (rampFrames == 0) {
            SetValue(value);
            return;
        }
        m_start       = m_value;
        m_target      = value;
        m_rampFrames  = rampFrames;
        m_rampIndex   = 0;
        m_ramping     = true;
    }

    /// 获取当前值（若处于平滑过渡中，推进一帧插值）
    float GetValue() {
        if (m_ramping) {
            if (m_rampIndex < m_rampFrames) {
                float t = static_cast<float>(m_rampIndex)
                        / static_cast<float>(m_rampFrames);
                // 余弦插值，使过渡更平滑
                float cosT = (1.0f - std::cos(t * 3.14159265f)) * 0.5f;
                m_value = m_start + (m_target - m_start) * cosT;
                ++m_rampIndex;
            } else {
                m_value   = m_target;
                m_ramping = false;
            }
        }
        return m_value;
    }

    /// 获取当前值（不推进插值，只读查询）
    float PeekValue() const { return m_value; }

    /// 获取目标值
    float GetTarget() const { return m_target; }

    /// 是否正在平滑过渡中
    bool IsRamping() const { return m_ramping; }

    /// 重置到指定值（立即，清空平滑状态）
    void Reset(float value = 0.0f) {
        m_value     = value;
        m_target    = value;
        m_ramping   = false;
        m_rampIndex = 0;
        m_rampFrames = 0;
    }

private:
    float    m_value      = 0.0f;
    float    m_target     = 0.0f;
    float    m_start      = 0.0f;
    bool     m_ramping    = false;
    uint32_t m_rampIndex  = 0;
    uint32_t m_rampFrames = 0;
};

} // namespace Prisma::Audio::DSP
