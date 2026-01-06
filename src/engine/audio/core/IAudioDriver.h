#pragma once

#include <cstdint>
#include <cstring>
#include <memory>

namespace PrismaEngine::Audio {

/// @brief 音频格式
struct AudioFormat {
    uint32_t sampleRate;      // 采样率 (Hz)
    uint16_t channels;        // 声道数 (1=单声道, 2=立体声)
    uint16_t bitsPerSample;   // 位深度 (16, 24, 32)

    AudioFormat(uint32_t sr = 48000, uint16_t ch = 2, uint16_t bps = 16)
        : sampleRate(sr), channels(ch), bitsPerSample(bps) {}

    // 帧大小（字节）
    uint16_t GetFrameSize() const { return channels * bitsPerSample / 8; }

    // 每秒字节数
    uint32_t GetBytesPerSecond() const { return sampleRate * GetFrameSize(); }
};

/// @brief 音频缓冲区
struct AudioBuffer {
    const uint8_t* data;      // 音频数据指针
    size_t size;              // 数据大小（字节）
    size_t frames;            // 帧数
    AudioFormat format;       // 音频格式

    AudioBuffer() : data(nullptr), size(0), frames(0), format() {}
};

/// @brief 音频源状态
enum class SourceState : uint8_t {
    Stopped,
    Playing,
    Paused
};

/// @brief 音频驱动抽象接口
/// 职责：与平台原生音频API交互，提供最底层的音频播放能力
///
/// 设计原则：
/// 1. 只处理与系统音频API的直接交互
/// 2. 不涉及3D音频、音效等高级功能（由上层实现）
/// 3. 接口简洁，易于跨平台实现
class IAudioDriver {
public:
    virtual ~IAudioDriver() = default;

    /// @brief 获取驱动名称
    virtual const char* GetName() const = 0;

    /// @brief 初始化音频驱动
    /// @param format 期望的音频格式（驱动可能调整）
    /// @return 实际使用的音频格式，失败返回默认格式
    virtual AudioFormat Initialize(const AudioFormat& format) = 0;

    /// @brief 关闭音频驱动
    virtual void Shutdown() = 0;

    /// @brief 检查是否已初始化
    virtual bool IsInitialized() const = 0;

    /// @brief 获取实际使用的音频格式
    virtual AudioFormat GetFormat() const = 0;

    // ========== 音频源管理 ==========

    /// @brief 音频源ID
    using SourceId = uint32_t;
    static constexpr SourceId InvalidSource = 0;

    /// @brief 创建音频源
    /// @return 音频源ID，失败返回 InvalidSource
    virtual SourceId CreateSource() = 0;

    /// @brief 销毁音频源
    virtual void DestroySource(SourceId sourceId) = 0;

    /// @brief 检查音频源是否有效
    virtual bool IsSourceValid(SourceId sourceId) const = 0;

    // ========== 播放控制 ==========

    /// @brief 将音频数据入队播放
    /// @param sourceId 音频源ID
    /// @param buffer 音频缓冲区
    /// @return 是否成功
    virtual bool QueueBuffer(SourceId sourceId, const AudioBuffer& buffer) = 0;

    /// @brief 开始播放
    /// @param sourceId 音频源ID
    /// @param loop 是否循环播放
    /// @return 是否成功
    virtual bool Play(SourceId sourceId, bool loop = false) = 0;

    /// @brief 停止播放
    virtual void Stop(SourceId sourceId) = 0;

    /// @brief 暂停播放
    virtual void Pause(SourceId sourceId) = 0;

    /// @brief 恢复播放
    virtual void Resume(SourceId sourceId) = 0;

    /// @brief 获取音频源状态
    virtual SourceState GetState(SourceId sourceId) const = 0;

    // ========== 实时控制 ==========

    /// @brief 设置音量 (0.0 - 1.0)
    virtual void SetVolume(SourceId sourceId, float volume) = 0;

    /// @brief 设置播放位置（秒）
    virtual void SetPosition(SourceId sourceId, float seconds) = 0;

    /// @brief 获取当前播放位置（秒）
    virtual float GetPosition(SourceId sourceId) const = 0;

    // ========== 全局控制 ==========

    /// @brief 设置主音量 (0.0 - 1.0)
    virtual void SetMasterVolume(float volume) = 0;

    /// @brief 获取主音量
    virtual float GetMasterVolume() const = 0;

    // ========== 缓冲区回调 ==========

    /// @brief 缓冲区结束回调函数类型
    /// @param sourceId 音频源ID
    /// @param userData 用户数据
    using BufferEndCallback = void (*)(SourceId sourceId, void* userData);

    /// @brief 设置缓冲区结束回调
    /// @param callback 回调函数
    /// @param userData 用户数据
    virtual void SetBufferEndCallback(BufferEndCallback callback, void* userData) = 0;

    // ========== 查询 ==========

    /// @brief 获取当前活跃的音频源数量
    virtual uint32_t GetActiveSourceCount() const = 0;

    /// @brief 获取支持的缓冲区数量
    virtual uint32_t GetMaxBuffers() const = 0;
};

// ========== 驱动工厂函数类型 ==========

/// @brief 驱动创建函数类型
using DriverCreateFunc = std::unique_ptr<IAudioDriver>(*)();

} // namespace PrismaEngine::Audio
