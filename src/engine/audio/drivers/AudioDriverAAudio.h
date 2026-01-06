#pragma once

#if defined(__ANDROID__) && defined(PRISMA_ENABLE_AUDIO_AAUDIO)

#include "core/IAudioDriver.h"
#include <aaudio/AAudio.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace PrismaEngine::Audio {

/// @brief AAudio 音频驱动 (Android 原生, API 26+)
/// AAudio 是 Android 8.1 引入的高性能原生音频 API
class AudioDriverAAudio : public IAudioDriver {
public:
    AudioDriverAAudio();
    ~AudioDriverAAudio() override;

    // IAudioDriver 接口
    const char* GetName() const override { return "AAudio"; }

    AudioFormat Initialize(const AudioFormat& format) override;
    void Shutdown() override;
    bool IsInitialized() const override;
    AudioFormat GetFormat() const override { return m_format; }

    SourceId CreateSource() override;
    void DestroySource(SourceId sourceId) override;
    bool IsSourceValid(SourceId sourceId) const override;

    bool QueueBuffer(SourceId sourceId, const AudioBuffer& buffer) override;
    bool Play(SourceId sourceId, bool loop) override;
    void Stop(SourceId sourceId) override;
    void Pause(SourceId sourceId) override;
    void Resume(SourceId sourceId) override;
    SourceState GetState(SourceId sourceId) const override;

    void SetVolume(SourceId sourceId, float volume) override;
    void SetPosition(SourceId sourceId, float seconds) override;
    float GetPosition(SourceId sourceId) const override;

    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override { return m_masterVolume; }

    void SetBufferEndCallback(BufferEndCallback callback, void* userData) override;

    uint32_t GetActiveSourceCount() const override { return m_activeSourceCount; }
    uint32_t GetMaxBuffers() const override { return MAX_BUFFERS; }

private:
    // ========== 常量 ==========
    static constexpr uint32_t MAX_BUFFERS = 4;         // 每个源最大缓冲区数
    static constexpr uint32_t MAX_SOURCES = 32;         // 最大音频源数
    static constexpr uint32_t FRAME_COUNT = 192;        // 每帧采样数 (低延迟)
    static constexpr uint32_t SAMPLE_RATE = 48000;     // 默认采样率
    static constexpr uint16_t CHANNELS = 2;             // 立体声

    // ========== 内部结构 ==========

    /// @brief AAudio 音频源
    struct AAudioSource {
        std::vector<uint8_t> audioData;          // 音频数据
        size_t dataSize = 0;                     // 数据大小
        size_t readPos = 0;                      // 读位置
        size_t writePos = 0;                     // 写位置
        float volume = 1.0f;                     // 音量
        float pitch = 1.0f;                      // 音调
        bool looping = false;                    // 循环播放
        bool paused = false;                     // 暂停
        bool playing = false;                    // 播放中
        SourceState state = SourceState::Stopped;
    };

    // ========== 初始化 ==========

    bool BuildAudioStream();
    void ReleaseResources();

    // ========== 数据流处理 ==========

    // AAudio 数据回调
    static aaudio_data_callback_result_t DataCallback(
        AAudioStream* stream,
        void* userData,
        void* audioData,
        int32_t numFrames);

    // AAudio 错误回调
    static void ErrorCallback(
        AAudioStream* stream,
        void* userData,
        aaudio_result_t error);

    // 内部数据处理
    aaudio_data_callback_result_t ProcessAudio(
        void* audioData,
        int32_t numFrames);

    void MixSource(float* output, int32_t numFrames, AAudioSource& source);
    void ConvertAndClamp(float sample, int16_t* output);

    // ========== 源管理 ==========

    AAudioSource* GetSource(SourceId sourceId);
    const AAudioSource* GetSource(SourceId sourceId) const;
    SourceId GenerateSourceId();

    // ========== 成员变量 ==========

    // AAudio 流
    AAudioStream* m_stream = nullptr;

    // 音频格式
    AudioFormat m_format;

    // 音频源池
    std::vector<AAudioSource*> m_sources;
    std::atomic<SourceId> m_nextSourceId{1};
    std::atomic<uint32_t> m_activeSourceCount{0};
    bool m_sourceUsed[MAX_SOURCES] = {false};

    // 混音缓冲区
    std::vector<float> m_mixBuffer;
    std::vector<int16_t> m_outputBuffer;

    // 音量控制
    float m_masterVolume = 1.0f;

    // 回调
    BufferEndCallback m_bufferEndCallback = nullptr;
    void* m_callbackUserData = nullptr;

    // 同步
    mutable std::mutex m_mutex;

    // 状态
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_running{false};
};

/// @brief 创建 AAudio 驱动实例
inline std::unique_ptr<IAudioDriver> CreateAAudioDriver() {
    return std::make_unique<AudioDriverAAudio>();
}

} // namespace PrismaEngine::Audio

#endif // __ANDROID__ && PRISMA_ENABLE_AUDIO_AAUDIO
