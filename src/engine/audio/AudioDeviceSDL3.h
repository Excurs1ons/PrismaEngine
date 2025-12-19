#pragma once

#include "IAudioDevice.h"
#include <SDL3/SDL.h>
#include <unordered_map>
#include <array>
#include <mutex>

namespace Engine::Audio {

/// @brief SDL3音频设备实现
/// SDL3提供了跨平台的音频支持，适合简单的2D游戏音频需求
class AudioDeviceSDL3 : public IAudioDevice {
public:
    AudioDeviceSDL3();
    ~AudioDeviceSDL3() override;

    // IAudioDevice接口实现
    bool Initialize(const AudioDesc& desc) override;
    void Shutdown() override;
    bool IsInitialized() const override;
    void Update(float deltaTime) override;

    DeviceInfo GetDeviceInfo() const override;
    std::vector<DeviceInfo> GetAvailableDevices() const override;
    bool SetDevice(const std::string& deviceName) override;
    AudioDeviceType GetDeviceType() const override;

    // 播放控制
    AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc = {}) override;
    void Stop(AudioVoiceId voiceId) override;
    void Pause(AudioVoiceId voiceId) override;
    void Resume(AudioVoiceId voiceId) override;

    // 实时控制
    void SetVolume(AudioVoiceId voiceId, float volume) override;
    void SetPitch(AudioVoiceId voiceId, float pitch) override;
    void SetPlaybackPosition(AudioVoiceId voiceId, float time) override;

    // 3D音频（SDL3限制）
    void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) override;
    void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) override;
    void SetListener(const AudioListener& listener) override;
    void SetDistanceModel(DistanceModel model) override;

    // 全局控制
    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override;

    // 查询
    bool IsPlaying(AudioVoiceId voiceId) override;
    bool IsPaused(AudioVoiceId voiceId) override;
    bool IsStopped(AudioVoiceId voiceId) override;
    float GetPlaybackPosition(AudioVoiceId voiceId) override;
    float GetDuration(AudioVoiceId voiceId) override;
    uint32_t GetPlayingVoiceCount() const override;

    // 事件系统
    void SetEventCallback(AudioEventCallback callback) override;
    void RemoveEventCallback() override;

    // 统计
    AudioStats GetStats() const override;
    void ResetStats() override;

    // 调试
    void BeginProfile() override;
    std::string EndProfile() override;
    std::string GenerateDebugReport() override;

private:
    // ========== 内部结构 ==========

    /// @brief 正在播放的音频
    struct PlayingVoice {
        SDL_AudioStream* stream = nullptr;          // SDL音频流
        std::vector<uint8_t> audioData;            // 音频数据副本
        float volume = 1.0f;                        // 音量
        float pitch = 1.0f;                         // 音调（SDL3限制）
        bool looping = false;                       // 是否循环
        bool paused = false;                        // 是否暂停
        size_t currentPosition = 0;                 // 当前播放位置
        float duration = 0.0f;                      // 音频时长
        bool isActive = false;                      // 是否活跃
    };

    // ========== 初始化相关 ==========

    /// @brief 初始化SDL音频子系统
    bool InitializeSDLAudio();

    /// @brief 打开音频设备
    bool OpenAudioDevice(const AudioFormat& format);

    /// @brief 设置音频回调
    void SetupAudioCallback();

    /// @brief 释放所有资源
    void ReleaseAll();

    // ========== 音频流管理 ==========

    /// @brief 创建音频流
    SDL_AudioStream* CreateAudioStream(const AudioClip& clip);

    /// @brief 销毁音频流
    void DestroyAudioStream(SDL_AudioStream* stream);

    /// @brief 重置音频流位置
    void ResetStreamPosition(PlayingVoice& voice);

    // ========== 播放管理 ==========

    /// @brief 生成Voice ID
    AudioVoiceId GenerateVoiceId();

    /// @brief 查找音频流
    PlayingVoice* FindVoice(AudioVoiceId voiceId);

    /// @brief 删除音频流
    void RemoveVoice(AudioVoiceId voiceId);

    /// @brief 更新音频状态
    void UpdateVoiceStates();

    // ========== 音频回调 ==========

    /// @brief SDL音频回调函数
    static void AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

    /// @brief 处理音频数据回调
    void HandleAudioCallback(SDL_AudioStream* stream, int additional_amount, int total_amount);

    /// @brief 混合音频数据
    void MixAudio(float* output, const float* input, int samples, float volume);

    // ========== 3D音频模拟 ==========

    /// @brief 计算3D音频音量衰减
    float Calculate3DVolume(const Audio3DAttributes& spatial, const AudioListener& listener);

    /// @brief 计算声源相对于听者的角度
    float CalculateSourceAngle(const Audio3DAttributes& spatial, const AudioListener& listener);

    // ========== 事件触发 ==========

    /// @brief 触发音频事件
    void TriggerEvent(AudioEventType type, AudioVoiceId voiceId = INVALID_VOICE_ID,
                     const std::string& message = "");

    // ========== 成员变量 ==========

    // SDL音频相关
    SDL_AudioDeviceID m_deviceId = 0;
    SDL_AudioSpec m_audioSpec;
    SDL_AudioSpec m_obtainedSpec;

    // 活跃音频流
    std::unordered_map<AudioVoiceId, PlayingVoice> m_playingVoices;

    // 混音缓冲区
    std::vector<float> m_mixBuffer;

    // 配置
    AudioDesc m_desc;
    float m_masterVolume = 1.0f;

    // 3D音频监听器
    AudioListener m_listener;

    // 状态
    std::atomic<bool> m_initialized{false};
    mutable std::mutex m_mutex;

    // 统计
    mutable AudioStats m_stats;
    uint32_t m_totalSamplesProcessed = 0;

    // 事件回调
    AudioEventCallback m_eventCallback;

    // 下一个Voice ID
    std::atomic<uint32_t> m_nextVoiceId{1};

    // 帧统计
    uint32_t m_framesProcessed = 0;
    double m_lastUpdateTime = 0.0;
};

} // namespace Engine::Audio