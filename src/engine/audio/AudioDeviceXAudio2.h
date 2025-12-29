#pragma once

#include "IAudioDevice.h"
#include <mutex>
#include <wrl/client.h>
#include <xaudio2.h>
#include <memory>
#include <unordered_map>
#include <array>
#include <thread>
#include <atomic>


namespace Engine::Audio {

/// @brief XAudio2音频设备实现
/// XAudio2是Windows平台的高性能音频API，支持低延迟和硬件加速
class AudioDeviceXAudio2 : public IAudioDevice, public IXAudio2VoiceCallback {
public:
    AudioDeviceXAudio2();
    ~AudioDeviceXAudio2() override;

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

    // 3D音频
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

    // IXAudio2VoiceCallback接口
    void __stdcall OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
    void __stdcall OnVoiceProcessingPassEnd() override;
    void __stdcall OnStreamEnd() override;
    void __stdcall OnBufferStart(void* pBufferContext) override;
    void __stdcall OnBufferEnd(void* pBufferContext) override;
    void __stdcall OnLoopEnd(void* pBufferContext) override;
    void __stdcall OnVoiceError(void* pBufferContext, HRESULT Error) override;

private:
    // ========== 内部结构 ==========

    /// @brief 音频Voice状态
    struct Voice {
        IXAudio2SourceVoice* sourceVoice = nullptr;
        std::vector<uint8_t> audioData;       // 音频数据副本
        WAVEFORMATEX waveFormat;              // WAVE格式
        bool isActive = false;                // 是否活跃
        bool isLooping = false;               // 是否循环
        float volume = 1.0f;                  // 音量
        float pitch = 1.0f;                   // 音调
        float duration = 0.0f;                // 时长
        PlayDesc desc;                        // 播放描述
    };

    /// @brief 3D音频参数（XAudio2特定）
    // struct X3DAudioParameters {
    //     X3DAUDIO_DSP_SETTINGS dspSettings;
    //     X3DAUDIO_LISTENER listener;
    //     X3DAUDIO_EMITTER emitter;
    // };

    // ========== 初始化相关 ==========

    /// @brief 初始化XAudio2
    bool InitializeXAudio2();

    /// @brief 创建主控语音
    bool CreateMasteringVoice();

    /// @brief 初始化3D音频
    bool Initialize3DAudio();

    /// @brief 释放所有资源
    void ReleaseAll();

    // ========== 音频Voice管理 ==========

    /// @brief 分配音频源
    Voice* AllocateVoice();

    /// @brief 释放音频源
    void ReleaseVoice(Voice* voice);

    /// @brief 根据ID查找音频源
    Voice* FindVoice(AudioVoiceId voiceId);

    /// @brief 创建WAVE格式
    bool CreateWaveFormat(const AudioFormat& format, WAVEFORMATEX& waveFormat);

    /// @brief 提交音频缓冲区
    bool SubmitBuffer(Voice* voice, bool forceStart = false);

    // ========== 3D音频 ==========

    /// @brief 更新3D音频
    void Update3DAudio();

    /// @brief 应用3D效果到语音
    void Apply3DToVoice(Voice* voice);

    /// @brief 转换距离模型到X3DAUDIO_CURVE_TYPE
    // X3DAUDIO_CURVE_TYPE GetX3DAudioCurveType(DistanceModel model) const;

    // ========== 错误处理 ==========

    /// @brief 检查HRESULT错误
    bool CheckHResult(HRESULT hr, const std::string& operation);

    // ========== 事件触发 ==========

    /// @brief 触发音频事件
    void TriggerEvent(AudioEventType type, AudioVoiceId voiceId = INVALID_VOICE_ID, const std::string& message = "");

public:
    AudioVoiceId Play(const AudioClip& clip, const PlayDesc& desc) override;
    void StopAll() override;
    void PauseAll() override;
    void ResumeAll() override;
    void SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) override;
    void SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) override;
    void SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) override;
    void SetDopplerFactor(float factor) override;
    void SetSpeedOfSound(float speed) override;
    VoiceState GetVoiceState(AudioVoiceId voiceId) override;

private:
    // ========== 成员变量 ==========

    // XAudio2核心对象
    Microsoft::WRL::ComPtr<IXAudio2> m_xaudio2;
    IXAudio2MasteringVoice* m_masteringVoice = nullptr;

    // 3D音频相关
    // X3DAUDIO_HANDLE m_x3dAudioInstance;
    // X3DAUDIO_PARAMETERS m_x3dParams;
    uint32_t m_channelMask;
    float m_speedOfSound = 343.3f;

    // 音频Voice池
    static constexpr uint32_t MAX_VOICES = 256;
    std::array<Voice, MAX_VOICES> m_voicePool;
    std::vector<Voice*> m_availableVoices;

    // 活跃音频源映射
    std::unordered_map<AudioVoiceId, Voice*> m_activeVoices;

    // 配置和状态
    AudioDesc m_desc;
    float m_masterVolume = 1.0f;
    std::atomic<bool> m_initialized{false};
    mutable std::mutex m_mutex;

    // 统计
    mutable AudioStats m_stats;
    uint32_t m_totalBuffersSubmitted = 0;

    // 事件回调
    AudioEventCallback m_eventCallback;

    // 监听器
    AudioListener m_listener;

    // 下一个Voice ID
    std::atomic<uint32_t> m_nextVoiceId{1};

    // 用于Voice回调的映射
    std::unordered_map<void*, AudioVoiceId> m_bufferContextToVoiceId;
};

} // namespace Engine::Audio