#pragma once

#include "IAudioDevice.h"
#include <SDL3/SDL.h>
#include <unordered_map>
#include <array>
#include <mutex>
#include <atomic>

namespace Prisma::Audio {

/// @brief SDL3音频设备实现
/// SDL3提供了跨平台的音频支持，适合简单的2D游戏和基本的音频需求
class AudioDeviceSDL3 : public IAudioDevice {
public:
    AudioDeviceSDL3();
    ~AudioDeviceSDL3() override;

    // IAudioDevice接口实现
    bool Initialize(const AudioDesc& desc) override;
    void Shutdown() override;
    bool IsInitialized() const override;
    void Update(Prisma::Timestep ts) override;

    DeviceInfo GetDeviceInfo() const override;
    std::vector<DeviceInfo> GetAvailableDevices() const override;
    bool SetDevice(const std::string& deviceName) override;
    AudioDeviceType GetDeviceType() const override { return AudioDeviceType::SDL3; }

    AudioVoiceId Play(const AudioClip& clip, const PlayDesc& desc = {}) override;
    AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc = {}) override;
    void Stop(AudioVoiceId voiceId) override;
    void Pause(AudioVoiceId voiceId) override;
    void Resume(AudioVoiceId voiceId) override;
    void StopAll() override;
    void PauseAll() override;
    void ResumeAll() override;

    void SetVolume(AudioVoiceId voiceId, float volume) override;
    void SetPitch(AudioVoiceId voiceId, float pitch) override;
    void SetPlaybackPosition(AudioVoiceId voiceId, float time) override;

    void SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) override;
    void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) override;
    void SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) override;
    void SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) override;
    void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) override;
    void SetListener(const AudioListener& listener) override;

    void SetDistanceModel(DistanceModel model) override;
    void SetDopplerFactor(float factor) override;
    void SetSpeedOfSound(float speed) override;

    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override;

    bool IsPlaying(AudioVoiceId voiceId) override;
    bool IsPaused(AudioVoiceId voiceId) override;
    bool IsStopped(AudioVoiceId voiceId) override;
    float GetPlaybackPosition(AudioVoiceId voiceId) override;
    float GetDuration(AudioVoiceId voiceId) override;
    VoiceState GetVoiceState(AudioVoiceId voiceId) override;
    uint32_t GetPlayingVoiceCount() const override;

    void SetEventCallback(AudioEventCallback callback) override;
    void RemoveEventCallback() override;

    AudioStats GetStats() const override;
    void ResetStats() override;

    void BeginProfile() override;
    std::string EndProfile() override;
    std::string GenerateDebugReport() override;

private:
    struct PlayingVoice {
        AudioVoiceId id;
        const AudioClip* clip;
        SDL_AudioStream* stream;
        VoiceState state;
        float volume;
        float pitch;
        bool looping;
        bool isActive;
        float position[3];
        
        // SDL3 Specific
        SDL_AudioSpec spec;
    };

    void RemoveVoice(AudioVoiceId voiceId);
    void UpdateVoiceStates();
    void TriggerEvent(AudioEventType type, AudioVoiceId voiceId);
    void ResetStreamPosition(PlayingVoice& voice);

    bool m_initialized = false;
    SDL_AudioDeviceID m_deviceId = 0;
    AudioDesc m_desc;
    float m_masterVolume = 1.0f;
    AudioListener m_listener;
    AudioStats m_stats;

    // 正在播放的Voices
    std::unordered_map<AudioVoiceId, PlayingVoice> m_playingVoices;
    mutable std::mutex m_mutex;

    // 事件回调
    AudioEventCallback m_eventCallback;

    // 下一个Voice ID
    std::atomic<uint32_t> m_nextVoiceId{1};

    // 帧统计
    uint32_t m_framesProcessed = 0;
    double m_lastUpdateTime = 0.0;
};

} // namespace Prisma::Audio
