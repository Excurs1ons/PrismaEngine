#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Prisma::Audio::DSP {

// ============================================================================
// SampleFormat - 采样格式枚举
// ============================================================================
enum class SampleFormat : uint8_t {
    Float32 = 0,
    Int16   = 1,
    Int24   = 2,
    Int32   = 3
};

// ============================================================================
// AudioFormatEx - 扩展音频格式描述
// ============================================================================
struct AudioFormatEx {
    uint32_t    sampleRate = 48000;   // 采样率 (Hz)
    uint16_t    channels   = 2;       // 声道数
    SampleFormat format     = SampleFormat::Float32; // 采样格式
    uint32_t    frames     = 0;       // 每声道帧数

    // 帧大小（字节）
    [[nodiscard]] uint16_t GetFrameSize() const {
        switch (format) {
            case SampleFormat::Float32: return channels * 4;
            case SampleFormat::Int16:   return channels * 2;
            case SampleFormat::Int24:   return channels * 3;
            case SampleFormat::Int32:   return channels * 4;
        }
        return channels * 4;
    }

    // 总数据大小（字节）
    [[nodiscard]] size_t GetDataSize() const {
        return static_cast<size_t>(frames) * GetFrameSize();
    }

    // 是否有效
    [[nodiscard]] bool IsValid() const {
        return sampleRate > 0 && channels > 0 && frames > 0;
    }
};

// ============================================================================
// AudioBuffer - Planar float32 音频缓冲
// ============================================================================
class AudioBuffer {
public:
    AudioBuffer() = default;

    AudioBuffer(uint32_t channels, uint32_t frames)
        : m_channels(channels)
        , m_frames(frames)
    {
        m_data.resize(static_cast<size_t>(channels) * frames, 0.0f);
    }

    // === 配置查询 ===

    [[nodiscard]] uint32_t GetChannels() const { return m_channels; }
    [[nodiscard]] uint32_t GetFrames()   const { return m_frames; }
    [[nodiscard]] bool     IsEmpty()     const { return m_data.empty(); }

    // === 数据访问 ===

    // 获取指定声道数据指针（planar 布局）
    [[nodiscard]] float* GetChannel(uint32_t ch) {
        if (ch >= m_channels) { return nullptr; }
        return m_data.data() + static_cast<size_t>(ch) * m_frames;
    }

    [[nodiscard]] const float* GetChannel(uint32_t ch) const {
        if (ch >= m_channels) { return nullptr; }
        return m_data.data() + static_cast<size_t>(ch) * m_frames;
    }

    // 获取原始数据指针
    [[nodiscard]] float*       GetData()       { return m_data.data(); }
    [[nodiscard]] const float* GetData() const { return m_data.data(); }

    // 获取数据大小（采样数）
    [[nodiscard]] size_t GetSampleCount() const {
        return static_cast<size_t>(m_channels) * m_frames;
    }

    // === 缓冲管理 ===

    // 重新配置缓冲大小
    void Resize(uint32_t channels, uint32_t frames) {
        if (channels == m_channels && frames == m_frames) { return; }
        m_channels = channels;
        m_frames   = frames;
        m_data.resize(static_cast<size_t>(channels) * frames, 0.0f);
    }

    // 清零所有采样
    void Clear() {
        std::fill(m_data.begin(), m_data.end(), 0.0f);
    }

    // 填充指定声道为固定值
    void Fill(float value) {
        std::fill(m_data.begin(), m_data.end(), value);
    }

    // 复制同配置缓冲
    bool CopyFrom(const AudioBuffer& src) {
        if (src.m_channels != m_channels || src.m_frames != m_frames) {
            return false;
        }
        std::copy(src.m_data.begin(), src.m_data.end(), m_data.begin());
        return true;
    }

    // 复制不同配置缓冲（自动转换）
    bool CopyFrom(const AudioBuffer& src, uint32_t dstChannel, uint32_t srcChannel) {
        if (dstChannel >= m_channels || srcChannel >= src.m_channels) {
            return false;
        }
        const uint32_t copyFrames = std::min(m_frames, src.m_frames);
        float*       dstData = GetChannel(dstChannel);
        const float* srcData = src.GetChannel(srcChannel);
        std::copy(srcData, srcData + copyFrames, dstData);
        return true;
    }

    // === 混合操作 ===

    // Mix: 将源缓冲以指定增益混合到本缓冲
    // dst[i] += src[i] * gain
    bool Mix(const AudioBuffer& src, float gain = 1.0f) {
        if (src.m_channels != m_channels || src.m_frames != m_frames) {
            return false;
        }
        const size_t count = m_data.size();
        for (size_t i = 0; i < count; ++i) {
            m_data[i] += src.m_data[i] * gain;
        }
        return true;
    }

    // Mix: 指定声道混合
    bool Mix(const AudioBuffer& src, float gain, uint32_t dstChannel, uint32_t srcChannel) {
        if (dstChannel >= m_channels || srcChannel >= src.m_channels) {
            return false;
        }
        const uint32_t mixFrames = std::min(m_frames, src.m_frames);
        float*       dstData = GetChannel(dstChannel);
        const float* srcData = src.GetChannel(srcChannel);
        for (uint32_t i = 0; i < mixFrames; ++i) {
            dstData[i] += srcData[i] * gain;
        }
        return true;
    }

    // 应用增益到所有声道
    void ApplyGain(float gain) {
        const size_t count = m_data.size();
        for (size_t i = 0; i < count; ++i) {
            m_data[i] *= gain;
        }
    }

    // 应用增益到指定声道
    void ApplyGain(float gain, uint32_t channel) {
        if (channel >= m_channels) { return; }
        float* ch = GetChannel(channel);
        for (uint32_t i = 0; i < m_frames; ++i) {
            ch[i] *= gain;
        }
    }

    // === 工具方法 ===

    // 查找最大绝对值（用于归一化/削波检测）
    [[nodiscard]] float FindPeak() const {
        float peak = 0.0f;
        for (float s : m_data) {
            float absVal = std::abs(s);
            if (absVal > peak) peak = absVal;
        }
        return peak;
    }

    // 查找指定声道的峰值
    [[nodiscard]] float FindPeak(uint32_t channel) const {
        if (channel >= m_channels) { return 0.0f; }
        const float* ch = GetChannel(channel);
        float peak = 0.0f;
        for (uint32_t i = 0; i < m_frames; ++i) {
            float absVal = std::abs(ch[i]);
            if (absVal > peak) peak = absVal;
        }
        return peak;
    }

    // 计算 RMS（均方根）
    [[nodiscard]] float ComputeRMS() const {
        if (m_data.empty()) { return 0.0f; }
        double sumSq = 0.0;
        for (float s : m_data) {
            sumSq += static_cast<double>(s) * s;
        }
        return static_cast<float>(std::sqrt(sumSq / m_data.size()));
    }

    // 硬削波（限制到 [-1, 1]）
    void Clamp() {
        for (float& s : m_data) {
            s = std::clamp(s, -1.0f, 1.0f);
        }
    }

private:
    std::vector<float> m_data;
    uint32_t m_channels = 0;
    uint32_t m_frames   = 0;
};

// ============================================================================
// AudioProcessContext - 音频处理上下文
// ============================================================================
struct AudioProcessContext {
    uint32_t sampleRate       = 48000;  // 采样率 (Hz)
    uint32_t framesPerBlock   = 512;    // 每处理块帧数
    double   currentTime      = 0.0;    // 当前时间（秒）
    uint64_t sampleIndex      = 0;      // 全局采样索引
    bool     isNonRealtime    = false;  // 测试模式标志（离线处理）
};

// ============================================================================
// ConvertFormat - 音频格式转换
// ============================================================================

namespace detail {

inline float Int16ToFloat(int16_t sample) {
    return static_cast<float>(sample) / 32768.0f;
}

inline int16_t FloatToInt16(float sample) {
    float clamped = std::clamp(sample, -1.0f, 1.0f);
    return static_cast<int16_t>(std::round(clamped * 32767.0f));
}

} // namespace detail

// 主转换函数
// 支持:
//   - int16 <-> float32
//   - mono <-> stereo (上混/下混)
// 返回 false 表示不支持的格式组合
inline bool ConvertFormat(const uint8_t* src, const AudioFormatEx& srcFmt,
                           uint8_t* dst, const AudioFormatEx& dstFmt,
                           uint32_t frameCount)
{
    if (!src || !dst) { return false; }
    if (frameCount == 0) { return true; }

    const uint32_t srcCh = srcFmt.channels;
    const uint32_t dstCh = dstFmt.channels;

    const bool srcIsFloat = (srcFmt.format == SampleFormat::Float32);
    const bool dstIsFloat = (dstFmt.format == SampleFormat::Float32);
    const bool srcIsInt16 = (srcFmt.format == SampleFormat::Int16);
    const bool dstIsInt16 = (dstFmt.format == SampleFormat::Int16);

    if (!((srcIsFloat && dstIsInt16) || (srcIsInt16 && dstIsFloat))) {
        if (srcFmt.format == dstFmt.format && srcIsFloat) {
            const float* srcF = reinterpret_cast<const float*>(src);
            float*       dstF = reinterpret_cast<float*>(dst);

            if (srcCh == dstCh) {
                std::memcpy(dst, src, static_cast<size_t>(frameCount) * srcCh * sizeof(float));
                return true;
            }

            for (uint32_t f = 0; f < frameCount; ++f) {
                for (uint32_t ch = 0; ch < dstCh; ++ch) {
                    if (srcCh == 1) {
                        dstF[f * dstCh + ch] = srcF[f];
                    } else if (dstCh == 1) {
                        float sum = 0.0f;
                        for (uint32_t s = 0; s < srcCh; ++s) {
                            sum += srcF[f * srcCh + s];
                        }
                        dstF[f] = sum / static_cast<float>(srcCh);
                    } else {
                        dstF[f * dstCh + ch] = (ch < srcCh) ? srcF[f * srcCh + ch] : 0.0f;
                    }
                }
            }
            return true;
        }
        if (srcFmt.format == dstFmt.format && srcIsInt16) {
            const int16_t* srcI = reinterpret_cast<const int16_t*>(src);
            int16_t*       dstI = reinterpret_cast<int16_t*>(dst);

            if (srcCh == dstCh) {
                std::memcpy(dst, src, static_cast<size_t>(frameCount) * srcCh * sizeof(int16_t));
                return true;
            }

            for (uint32_t f = 0; f < frameCount; ++f) {
                for (uint32_t ch = 0; ch < dstCh; ++ch) {
                    if (srcCh == 1) {
                        dstI[f * dstCh + ch] = srcI[f];
                    } else if (dstCh == 1) {
                        int32_t sum = 0;
                        for (uint32_t s = 0; s < srcCh; ++s) {
                            sum += srcI[f * srcCh + s];
                        }
                        dstI[f] = static_cast<int16_t>(sum / static_cast<int32_t>(srcCh));
                    } else {
                        dstI[f * dstCh + ch] = (ch < srcCh) ? srcI[f * srcCh + ch] : 0;
                    }
                }
            }
            return true;
        }
        return false;
    }

    // === int16 -> float32 ===
    if (srcIsInt16 && dstIsFloat) {
        const int16_t* srcI = reinterpret_cast<const int16_t*>(src);
        float*         dstF = reinterpret_cast<float*>(dst);

        for (uint32_t f = 0; f < frameCount; ++f) {
            for (uint32_t ch = 0; ch < dstCh; ++ch) {
                if (srcCh == dstCh) {
                    dstF[f * dstCh + ch] = detail::Int16ToFloat(srcI[f * srcCh + ch]);
                } else if (srcCh == 1) {
                    dstF[f * dstCh + ch] = detail::Int16ToFloat(srcI[f]);
                } else if (dstCh == 1) {
                    float sum = 0.0f;
                    for (uint32_t s = 0; s < srcCh; ++s) {
                        sum += detail::Int16ToFloat(srcI[f * srcCh + s]);
                    }
                    dstF[f] = sum / static_cast<float>(srcCh);
                } else {
                    dstF[f * dstCh + ch] = (ch < srcCh)
                        ? detail::Int16ToFloat(srcI[f * srcCh + ch])
                        : 0.0f;
                }
            }
        }
        return true;
    }

    // === float32 -> int16 ===
    if (srcIsFloat && dstIsInt16) {
        const float*   srcF = reinterpret_cast<const float*>(src);
        int16_t*       dstI = reinterpret_cast<int16_t*>(dst);

        for (uint32_t f = 0; f < frameCount; ++f) {
            for (uint32_t ch = 0; ch < dstCh; ++ch) {
                if (srcCh == dstCh) {
                    dstI[f * dstCh + ch] = detail::FloatToInt16(srcF[f * srcCh + ch]);
                } else if (srcCh == 1) {
                    dstI[f * dstCh + ch] = detail::FloatToInt16(srcF[f]);
                } else if (dstCh == 1) {
                    float sum = 0.0f;
                    for (uint32_t s = 0; s < srcCh; ++s) {
                        sum += srcF[f * srcCh + s];
                    }
                    dstI[f] = detail::FloatToInt16(sum / static_cast<float>(srcCh));
                } else {
                    dstI[f * dstCh + ch] = (ch < srcCh)
                        ? detail::FloatToInt16(srcF[f * srcCh + ch])
                        : 0;
                }
            }
        }
        return true;
    }

    return false;
}

} // namespace Prisma::Audio::DSP
