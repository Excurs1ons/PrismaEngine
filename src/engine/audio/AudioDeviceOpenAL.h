#pragma once

#include "IAudioDevice.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <unordered_map>
#include <array>
#include <mutex>

namespace Engine::Audio {

/// @brief OpenAL音频设备实现
/// OpenAL是跨平台的3D音频API，特别适合游戏音频
class AudioDeviceOpenAL : public IAudioDevice {
public:
    AudioDeviceOpenAL();
    ~AudioDeviceOpenAL() override;

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

private:
    // ========== 内部结构 ==========

    /// @brief 音频Voice状态
    struct Voice {
        ALuint sourceId = 0;              // OpenAL源ID
        ALuint bufferId = 0;              // OpenAL缓冲区ID
        bool isActive = false;            // 是否活跃
        bool isLooping = false;           // 是否循环
        float basePitch = 1.0f;           // 基础音调
        float baseVolume = 1.0f;          // 基础音量
        AudioClip clip;                   // 音频剪辑
        PlayDesc desc;                    // 播放描述
        float playbackPosition = 0.0f;    // 播放位置
    };

    // ========== 初始化相关 ==========

    /// @brief 初始化OpenAL设备
    bool InitializeDevice(const std::string& deviceName);

    /// @brief 初始化上下文
    bool InitializeContext();

    /// @brief 初始化音频源池
    bool InitializeVoicePool(uint32_t maxVoices);

    /// @brief 初始化EFX（音效扩展）
    bool InitializeEFX();

    /// @brief 释放所有资源
    void ReleaseAll();

    // ========== 音频源管理 ==========

    /// @brief 分配音频源
    Voice* AllocateVoice();

    /// @brief 释放音频源
    void ReleaseVoice(Voice* voice);

    /// @brief 根据ID查找音频源
    Voice* FindVoice(AudioVoiceId voiceId);

    /// @brief 获取OpenAL格式
    ALenum GetOpenALFormat(const AudioFormat& format) const;

    /// @brief 创建或获取缓冲区
    ALuint GetOrCreateBuffer(const AudioClip& clip);

    /// @brief 更新音频源状态
    void UpdateVoiceState(Voice* voice);

    /// @brief 处理已结束的音频源
    void ProcessFinishedVoices();

    // ========== 3D音频辅助 ==========

    /// @brief 应用3D属性到音频源
    void Apply3DAttributes(Voice* voice);

    /// @brief 转换距离模型
    ALenum GetOpenALDistanceModel(DistanceModel model) const;

    // ========== EFX音效 ==========

    /// @brief 创建音效槽
    ALuint CreateEffectSlot();

    /// @brief 创建音效
    ALuint CreateEffect(EffectType type);

    /// @brief 应用混响效果
    bool ApplyReverbEffect(AudioVoiceId voiceId, const void* params);

    // ========== 错误处理 ==========

    /// @brief 检查并记录OpenAL错误
    bool CheckOpenALError(const std::string& operation);

    /// @brief 获取错误描述
    std::string GetOpenALErrorString(ALenum error);

    // ========== 事件触发 ==========

    /// @brief 触发音频事件
    void TriggerEvent(AudioEventType type, AudioVoiceId voiceId = INVALID_VOICE_ID,
                     const std::string& message = "");

    // ========== 成员变量 ==========

    // OpenAL核心对象
    ALCdevice* m_device = nullptr;
    ALCcontext* m_context = nullptr;

    // 音频源池
    static constexpr uint32_t MAX_VOICES = 256;
    std::array<Voice, MAX_VOICES> m_voicePool;
    std::vector<Voice*> m_availableVoices;

    // 活跃音频源映射
    std::unordered_map<AudioVoiceId, Voice*> m_activeVoices;

    // 音频缓冲区缓存
    std::unordered_map<const AudioClip*, ALuint> m_bufferCache;

    // 配置
    AudioDesc m_desc;
    float m_masterVolume = 1.0f;

    // 状态
    std::atomic<bool> m_initialized{false};
    mutable std::mutex m_mutex;

    // 统计
    mutable AudioStats m_stats;
    bool m_profilingEnabled = false;
    double m_profileStartTime = 0.0;

    // 事件回调
    AudioEventCallback m_eventCallback;

    // EFX相关
    bool m_hasEFX = false;
    LPALGENEFFECTS alGenEffects = nullptr;
    LPALDELETEEFFECTS alDeleteEffects = nullptr;
    LPALEFFECTI alEffecti = nullptr;
    LPALEFFECTF alEffectf = nullptr;

    // 下一个Voice ID
    std::atomic<uint32_t> m_nextVoiceId{1};
};

} // namespace Engine::Audio