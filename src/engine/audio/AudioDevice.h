#pragma once

#include "AudioTypes.h"
#include "core/IAudioDriver.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace PrismaEngine::Audio {

/// @brief 高层音频设备
///
/// 职责：
/// - 使用 IAudioDriver 与平台原生音频API交互
/// - 实现 3D 音频、音效等高级功能
/// - 管理音频源的生命周期和状态
///
/// 设计模式：策略模式
/// - IAudioDriver 是底层策略
/// - AudioDevice 是上下文，提供高级功能
class AudioDevice {
public:
    AudioDevice();
    ~AudioDevice();

    // ========== 初始化 ==========

    /// @brief 初始化音频设备
    /// @param desc 初始化描述
    /// @return 是否成功
    bool Initialize(const AudioDesc& desc = {});

    /// @brief 关闭音频设备
    void Shutdown();

    /// @brief 检查是否已初始化
    bool IsInitialized() const { return m_initialized.load(); }

    /// @brief 更新音频设备（每帧调用）
    void Update(float deltaTime);

    /// @brief 获取设备信息
    DeviceInfo GetDeviceInfo() const;

    // ========== 播放控制 ==========

    /// @brief 播放音频剪辑
    /// @param clip 音频剪辑
    /// @param desc 播放描述
    /// @return Voice ID，失败返回 INVALID_VOICE_ID
    AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc = {});

    /// @brief 停止播放
    void Stop(AudioVoiceId voiceId);

    /// @brief 暂停播放
    void Pause(AudioVoiceId voiceId);

    /// @brief 恢复播放
    void Resume(AudioVoiceId voiceId);

    /// @brief 停止所有
    void StopAll();

    /// @brief 暂停所有
    void PauseAll();

    /// @brief 恢复所有
    void ResumeAll();

    // ========== 实时控制 ==========

    /// @brief 设置音量
    void SetVolume(AudioVoiceId voiceId, float volume);

    /// @brief 设置音调
    void SetPitch(AudioVoiceId voiceId, float pitch);

    /// @brief 设置播放位置
    void SetPlaybackPosition(AudioVoiceId voiceId, float time);

    /// @brief 获取播放位置
    float GetPlaybackPosition(AudioVoiceId voiceId);

    /// @brief 获取音频时长
    float GetDuration(AudioVoiceId voiceId);

    // ========== 3D 音频 ==========

    /// @brief 设置 3D 位置
    void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z);

    /// @brief 设置 3D 位置（数组版本）
    void SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]);

    /// @brief 设置 3D 速度
    void SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]);

    /// @brief 设置 3D 方向
    void SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]);

    /// @brief 设置 3D 属性
    void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes);

    /// @brief 设置监听器
    void SetListener(const AudioListener& listener);

    /// @brief 设置距离模型
    void SetDistanceModel(DistanceModel model);

    /// @brief 设置多普勒因子
    void SetDopplerFactor(float factor);

    /// @brief 设置声速
    void SetSpeedOfSound(float speed);

    // ========== 全局控制 ==========

    /// @brief 设置主音量
    void SetMasterVolume(float volume);

    /// @brief 获取主音量
    float GetMasterVolume() const;

    // ========== 查询 ==========

    /// @brief 检查是否正在播放
    bool IsPlaying(AudioVoiceId voiceId);

    /// @brief 检查是否已暂停
    bool IsPaused(AudioVoiceId voiceId);

    /// @brief 检查是否已停止
    bool IsStopped(AudioVoiceId voiceId);

    /// @brief 获取 Voice 状态
    VoiceState GetVoiceState(AudioVoiceId voiceId);

    /// @brief 获取当前播放数量
    uint32_t GetPlayingVoiceCount() const;

    // ========== 事件系统 ==========

    /// @brief 设置事件回调
    void SetEventCallback(AudioEventCallback callback);

    /// @brief 移除事件回调
    void RemoveEventCallback();

    // ========== 统计 ==========

    /// @brief 获取统计信息
    AudioStats GetStats() const;

    /// @brief 重置统计信息
    void ResetStats();

    // ========== 调试 ==========

    /// @brief 生成调试报告
    std::string GenerateDebugReport();

private:
    // ========== 内部结构 ==========

    /// @brief Voice 内部状态
    struct Voice {
        IAudioDriver::SourceId driverSourceId;
        AudioClip clip;
        PlayDesc desc;
        VoiceState state = VoiceState::Stopped;
        float playbackPosition = 0.0f;
        bool is3D = false;

        // 3D 属性（用于软件混音）
        Audio3DAttributes spatial3D;
    };

    // ========== 驱动管理 ==========

    /// @brief 创建音频驱动
    /// @param deviceType 设备类型
    /// @return 驱动实例
    std::unique_ptr<IAudioDriver> CreateDriver(AudioDeviceType deviceType);

    // ========== 3D 音频计算 ==========

    /// @brief 计算 3D 音频衰减后的音量
    float Calculate3DVolume(const Audio3DAttributes& spatial);

    /// @brief 计算立体声平移
    void CalculatePan(const float position[3], float& left, float& right);

    // ========== Voice 管理 ==========

    /// @brief 生成 Voice ID
    AudioVoiceId GenerateVoiceId();

    /// @brief 查找 Voice
    Voice* FindVoice(AudioVoiceId voiceId);
    const Voice* FindVoice(AudioVoiceId voiceId) const;

    /// @brief 触发事件
    void TriggerEvent(AudioEventType type, AudioVoiceId voiceId = INVALID_VOICE_ID,
                     const std::string& message = "");

    /// @brief 驱动回调
    static void OnBufferEnd(IAudioDriver::SourceId sourceId, void* userData);

    // ========== 成员变量 ==========

    // 底层驱动
    std::unique_ptr<IAudioDriver> m_driver;

    // Voice 池
    std::unordered_map<AudioVoiceId, Voice> m_voices;
    std::atomic<AudioVoiceId> m_nextVoiceId{1};
    std::atomic<uint32_t> m_playingCount{0};

    // 配置
    AudioDesc m_desc;
    float m_masterVolume = 1.0f;

    // 3D 音频
    AudioListener m_listener;
    DistanceModel m_distanceModel = DistanceModel::InverseClamped;
    float m_dopplerFactor = 1.0f;
    float m_speedOfSound = 343.3f;

    // 事件
    AudioEventCallback m_eventCallback;

    // 统计
    mutable AudioStats m_stats;

    // 同步
    mutable std::mutex m_mutex;

    // 状态
    std::atomic<bool> m_initialized{false};

    // 映射：驱动 SourceId -> Voice ID
    std::unordered_map<IAudioDriver::SourceId, AudioVoiceId> m_sourceToVoice;
};

} // namespace PrismaEngine::Audio
