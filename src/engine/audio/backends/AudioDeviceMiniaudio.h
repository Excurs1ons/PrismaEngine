#pragma once

#include "IAudioDevice.h"
#include "AudioNode.h"
#include "audio/dsp/EffectProcessor.h"

#include <miniaudio.h>

namespace Prisma::Audio {

class AudioDeviceMiniaudio : public IAudioDevice {
public:
    AudioDeviceMiniaudio() = default;
    ~AudioDeviceMiniaudio() override { Shutdown(); }

    // ========== 初始化和关闭 ==========
    bool Initialize(const AudioDesc& desc) override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_initialized; }

    // ========== 更新 ==========
    void Update(Prisma::Timestep ts) override;

    // ========== 设备信息 ==========
    IAudioDevice::DeviceInfo GetDeviceInfo() const override;
    AudioDeviceType GetDeviceType() const override;
    std::vector<IAudioDevice::DeviceInfo> GetAvailableDevices() const override;

    // ========== 基本播放控制 ==========
    AudioVoiceId Play(const AudioClip& clip, const PlayDesc& desc) override;
    AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc) override;
    void Stop(AudioVoiceId voiceId) override;
    void Pause(AudioVoiceId voiceId) override;
    void PauseAll() override;
    void Resume(AudioVoiceId voiceId) override;
    void ResumeAll() override;
    void StopAll() override;

    // ========== 实时控制 ==========
    void SetVolume(AudioVoiceId voiceId, float volume) override;
    void SetPitch(AudioVoiceId voiceId, float pitch) override;
    void SetPlaybackPosition(AudioVoiceId voiceId, float time) override;
    float GetPlaybackPosition(AudioVoiceId voiceId) override;
    float GetDuration(AudioVoiceId voiceId) override;

    // ========== 查询 ==========
    bool IsPlaying(AudioVoiceId voiceId) override;
    bool IsPaused(AudioVoiceId voiceId) override;
    bool IsStopped(AudioVoiceId voiceId) override;
    VoiceState GetVoiceState(AudioVoiceId voiceId) override;
    uint32_t GetPlayingVoiceCount() const override;

    // ========== 3D音频 ==========
    void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) override;
    void SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) override;
    void SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) override;
    void SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) override;
    void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) override;
    void SetListener(const AudioListener& listener) override;

    // ========== Miniaudio 扩展 3D 方法 ==========
    void SetConeAngles(AudioVoiceId voiceId, float innerAngle, float outerAngle);
    void SetRolloffFactor(AudioVoiceId voiceId, float factor);

    // ========== 全局控制 ==========
    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override { return m_masterVolume; }
    void SetDistanceModel(DistanceModel model) override;
    void SetDopplerFactor(float factor) override;
    void SetSpeedOfSound(float speed) override;

    // ========== 事件系统 ==========
    void SetEventCallback(AudioEventCallback callback) override;
    void RemoveEventCallback() override;

    // ========== 统计信息 ==========
    AudioStats GetStats() const override;
    void ResetStats() override;

    // ========== 音效 ==========
    bool ApplyEffect(AudioVoiceId voiceId, EffectType type, const void* params) override;
    void RemoveEffects(AudioVoiceId voiceId) override;

    // ========== 调试功能 ==========
    std::string GenerateDebugReport() override;

    // ========== DSP 图绑定（Miniaudio 特有）==========
    void SetAudioGraph(DSP::AudioGraph* graph) { m_graph = graph; }
    DSP::AudioGraph* GetAudioGraph() const { return m_graph; }

private:
    static void OnSendAudio(ma_device* pDevice, void* pOutput,
                            const void* pInput, ma_uint32 frameCount);

    void FireEvent(AudioEventType type, AudioVoiceId voiceId, const std::string& msg = "");

    struct Voice {
        std::shared_ptr<AudioClip> clip;
        PlayDesc desc;
        VoiceState state = VoiceState::Stopped;
        size_t readCursor = 0;

        // 3D 音频属性
        float position[3] = {0.0f, 0.0f, 0.0f};
        float velocity[3] = {0.0f, 0.0f, 0.0f};
        float direction[3] = {0.0f, 0.0f, 1.0f};
        float coneInnerAngle = 360.0f;
        float coneOuterAngle = 360.0f;
        float coneOuterGain = 0.0f;
        float rolloffFactor = 1.0f;

        // 音效状态
        EffectType effectType = EffectType::None;
        uint8_t effectParams[128] = {}; // 存储 EffectParams 联合体 (最大~120字节)
        uint32_t effectParamsSize = 0;
        DSP::EffectState effectState;
    };

    ma_device m_device = {};
    ma_device_config m_config = {};
    mutable ma_mutex m_mutex;
    bool m_initialized = false;

    DSP::AudioGraph* m_graph = nullptr;
    std::unique_ptr<DSP::AudioBuffer> m_mixBuffer;

    std::unordered_map<AudioVoiceId, Voice> m_voices;
    AudioVoiceId m_nextVoiceId = 1;
    float m_masterVolume = 1.0f;
    AudioDesc m_desc;

    // 3D 音频状态
    AudioListener m_listener;
    DistanceModel m_distanceModel = DistanceModel::InverseClamped;
    float m_dopplerFactor = 1.0f;
    float m_speedOfSound = 343.3f;

    AudioEventCallback m_eventCallback;
};

} // namespace Prisma::Audio
