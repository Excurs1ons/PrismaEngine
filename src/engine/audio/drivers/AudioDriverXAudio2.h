#pragma once

#if defined(_WIN32) && defined(PRISMA_ENABLE_AUDIO_XAUDIO2)

#include "core/IAudioDriver.h"
#include <xaudio2.h>
#include <atomic>
#include <unordered_map>
#include <mutex>

namespace PrismaEngine::Audio {

/// @brief XAudio2 音频驱动 (Windows 原生)
class AudioDriverXAudio2 : public IAudioDriver {
public:
    AudioDriverXAudio2();
    ~AudioDriverXAudio2() override;

    // IAudioDriver 接口
    const char* GetName() const override { return "XAudio2"; }

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
    static constexpr uint32_t MAX_BUFFERS = 16;        // 每个源最大缓冲区数
    static constexpr uint32_t MAX_SOURCES = 256;        // 最大音频源数
    static constexpr uint32_t SAMPLE_RATE = 48000;      // 默认采样率
    static constexpr uint16_t_CHANNELS = 2;             // 默认立体声
    static constexpr uint16_t BITS_PER_SAMPLE = 16;     // 16位深度

    // ========== 内部结构 ==========

    /// @brief XAudio2 音频源封装
    struct XAudioSource {
        IXAudio2SourceVoice* voice = nullptr;      // XAudio2 源语音
        XAUDIO2_BUFFER buffers[MAX_BUFFERS];       // 缓冲区数组
        uint32_t bufferIndex = 0;                  // 当前缓冲区索引
        uint32_t queuedBuffers = 0;                // 已排队缓冲区数
        bool looping = false;                      // 是否循环
        float duration = 0.0f;                     // 音频时长（秒）
        std::vector<uint8_t> audioData;            // 保存音频数据指针
    };

    /// @brief 引用计数的音频数据
    struct AudioBufferData {
        std::vector<uint8_t> data;
    };

    // ========== 初始化 ==========

    bool CreateXAudio2();
    bool CreateMasteringVoice();
    void ReleaseResources();

    // ========== 源管理 ==========

    XAudioSource* GetSource(SourceId sourceId);
    const XAudioSource* GetSource(SourceId sourceId) const;
    SourceId GenerateSourceId();

    // ========== 回调处理 ==========

    static void WINAPI VoiceCallback(IXAudio2VoiceCallback* callback, void* pBuffer);
    void OnBufferEnd(SourceId sourceId);

    // ========== 成员变量 ==========

    // XAudio2 核心
    IXAudio2* m_xaudio2 = nullptr;
    IXAudio2MasteringVoice* m_masteringVoice = nullptr;

    // 音频格式
    AudioFormat m_format;

    // 音频源池
    std::unordered_map<SourceId, XAudioSource> m_sources;
    std::atomic<SourceId> m_nextSourceId{1};
    std::atomic<uint32_t> m_activeSourceCount{0};

    // 音量控制
    float m_masterVolume = 1.0f;

    // 回调
    BufferEndCallback m_bufferEndCallback = nullptr;
    void* m_callbackUserData = nullptr;

    // 同步
    mutable std::mutex m_mutex;

    // 状态
    std::atomic<bool> m_initialized{false};
};

/// @brief 创建 XAudio2 驱动实例
inline std::unique_ptr<IAudioDriver> CreateXAudio2Driver() {
    return std::make_unique<AudioDriverXAudio2>();
}

} // namespace PrismaEngine::Audio

#endif // _WIN32 && PRISMA_ENABLE_AUDIO_XAUDIO2
