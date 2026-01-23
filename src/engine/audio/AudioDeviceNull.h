#pragma once

#include "IAudioDevice.h"
#include "AudioTypes.h"
#include <mutex>

namespace PrismaEngine::Audio {

/// @brief 空音频设备实现
/// 用于测试和调试，不产生任何声音
class AudioDeviceNull : public IAudioDevice {
public:
    AudioDeviceNull();
    ~AudioDeviceNull() override;

    // ========== IAudioDevice 接口实现 ==========

    bool Initialize(const AudioDesc& desc) override;
    void Shutdown() override;
    bool IsInitialized() const override;

    // 播放控制
    AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc) override;
    AudioVoiceId Play(const AudioClip& clip, const PlayDesc& desc) override;
    void Stop(AudioVoiceId voiceId) override;
    void Pause(AudioVoiceId voiceId) override;
    void PauseAll() override;
    void Resume(AudioVoiceId voiceId) override;
    void ResumeAll() override;
    void StopAll() override;

    // 实时控制
    void SetVolume(AudioVoiceId voiceId, float volume) override;
    void SetPitch(AudioVoiceId voiceId, float pitch) override;
    void SetPlaybackPosition(AudioVoiceId voiceId, float time) override;

    // 3D音频
    void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) override;
    void SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) override;
    void SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) override;
    void SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) override;
    void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) override;
    void SetListener(const AudioListener& listener) override;

    // 全局控制
    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override;
    void SetDistanceModel(DistanceModel model) override;
    void SetDopplerFactor(float factor) override;
    void SetSpeedOfSound(float speed) override;

    // 查询
    bool IsPlaying(AudioVoiceId voiceId) override;
    bool IsPaused(AudioVoiceId voiceId) override;
    bool IsStopped(AudioVoiceId voiceId) override;
    float GetPlaybackPosition(AudioVoiceId voiceId) override;
    float GetDuration(AudioVoiceId voiceId) override;
    VoiceState GetVoiceState(AudioVoiceId voiceId) override;
    uint32_t GetPlayingVoiceCount() const override;

    // 设备信息
    IAudioDevice::DeviceInfo GetDeviceInfo() const override;
    std::vector<IAudioDevice::DeviceInfo> GetAvailableDevices() const override;
    bool SetDevice(const std::string& deviceName) override;
    AudioDeviceType GetDeviceType() const override;

    // 事件系统
    void SetEventCallback(AudioEventCallback callback) override;
    void RemoveEventCallback() override;

    // 统计信息
    AudioStats GetStats() const override;
    void ResetStats() override;

    // 调试
    void BeginProfile() override;
    std::string EndProfile() override;
    std::string GenerateDebugReport() override;

    // 更新
    void Update(float deltaTime) override;

private:
    // 内部 Voice 状态
    struct InternalVoiceState {
        AudioVoiceId id;
        bool playing;
        bool paused;
        bool looping;
        float volume;
        float pitch;
        float position;
        float duration;
        float velocity[3] = {0, 0, 0};
        float direction[3] = {0, 0, 1};
        PlayDesc desc;
    };

    // 生成新的Voice ID
    AudioVoiceId GenerateVoiceId();

    // 查找Voice
    InternalVoiceState* FindVoice(AudioVoiceId voiceId);

    // 成员变量
    bool m_initialized = false;
    float m_masterVolume = 1.0f;
    DistanceModel m_distanceModel = DistanceModel::InverseClamped;
    float m_dopplerFactor = 1.0f;
    float m_speedOfSound = 343.0f;
    AudioListener m_listener;
    AudioStats m_stats;
    AudioEventCallback m_eventCallback;

    // Voice管理
    std::unordered_map<AudioVoiceId, InternalVoiceState> m_voices;
    AudioVoiceId m_nextVoiceId = 1;

    // 线程安全
    mutable std::mutex m_mutex;
};

} // namespace Engine::Audio