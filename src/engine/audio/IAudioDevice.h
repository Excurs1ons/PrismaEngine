#pragma once

#include "AudioTypes.h"
#include <string>
#include <vector>
#include <memory>

namespace Engine::Audio {

/// @brief 音频设备抽象接口
/// 这是音频系统的核心抽象，不同的音频后端只需要实现这一个接口
class IAudioDevice {
public:
    virtual ~IAudioDevice() = default;
    virtual AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc = {}) = 0;
    virtual void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) = 0;
    virtual std::string GenerateDebugReport() = 0;
    // ========== 初始化和关闭 ==========
    virtual AudioDeviceType GetDeviceType() const = 0;
    /// @brief 初始化音频设备
    /// @param desc 设备初始化描述
    /// @return 是否初始化成功
    virtual bool Initialize(const AudioDesc& desc) = 0;

    /// @brief 关闭音频设备
    virtual void Shutdown() = 0;

    /// @brief 检查设备是否已初始化
    /// @return 是否已初始化
    virtual bool IsInitialized() const = 0;

    /// @brief 更新音频设备（每帧调用）
    /// @param deltaTime 时间增量（秒）
    virtual void Update(float deltaTime) = 0;

    // ========== 设备信息 ==========

    /// @brief 设备信息
    struct DeviceInfo {
        std::string name;         // 设备名称
        std::string driver;       // 驱动名称
        std::string version;      // 版本号
        bool isDefault = false;   // 是否为默认设备
        uint32_t maxVoices = 0;   // 最大支持的并发音源数
        bool supports3D = false;  // 是否支持3D音频
        bool supportsEffects = false; // 是否支持音效
    };

    /// @brief 获取当前设备信息
    /// @return 设备信息
    virtual DeviceInfo GetDeviceInfo() const = 0;

    /// @brief 获取所有可用设备列表
    /// @return 设备信息列表
    virtual std::vector<DeviceInfo> GetAvailableDevices() const = 0;

    /// @brief 设置当前设备（如果支持）
    /// @param deviceName 设备名称
    /// @return 是否设置成功
    virtual bool SetDevice(const std::string& deviceName) { return false; }

    // ========== 基本播放控制 ==========

    /// @brief 播放音频
    /// @param clip 音频剪辑
    /// @param desc 播放描述
    /// @return 音频Voice ID，失败返回INVALID_VOICE_ID
    virtual AudioVoiceId Play(const AudioClip& clip, const PlayDesc& desc = {}) = 0;

    /// @brief 停止播放
    /// @param voiceId 音频Voice ID
    virtual void Stop(AudioVoiceId voiceId) = 0;

    /// @brief 暂停播放
    /// @param voiceId 音频Voice ID
    virtual void Pause(AudioVoiceId voiceId) = 0;

    /// @brief 恢复播放
    /// @param voiceId 音频Voice ID
    virtual void Resume(AudioVoiceId voiceId) = 0;

    /// @brief 停止所有正在播放的音频
    virtual void StopAll() = 0;

    /// @brief 暂停所有正在播放的音频
    virtual void PauseAll() = 0;

    /// @brief 恢复所有暂停的音频
    virtual void ResumeAll() = 0;

    // ========== 实时控制 ==========

    /// @brief 设置音量
    /// @param voiceId 音频Voice ID
    /// @param volume 音量 (0.0 - 1.0)
    virtual void SetVolume(AudioVoiceId voiceId, float volume) = 0;

    /// @brief 设置音调
    /// @param voiceId 音频Voice ID
    /// @param pitch 音调倍数 (0.5 - 2.0)
    virtual void SetPitch(AudioVoiceId voiceId, float pitch) = 0;

    /// @brief 设置播放位置
    /// @param voiceId 音频Voice ID
    /// @param time 时间位置（秒）
    virtual void SetPlaybackPosition(AudioVoiceId voiceId, float time) = 0;

    // ========== 3D音频 ==========

    /// @brief 设置音频源3D位置
    /// @param voiceId 音频Voice ID
    /// @param position 位置 (x, y, z)
    virtual void SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) = 0;

    /// @brief 设置音频源3D速度
    /// @param voiceId 音频Voice ID
    /// @param velocity 速度 (vx, vy, vz)
    virtual void SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) = 0;

    /// @brief 设置音频源3D方向
    /// @param voiceId 音频Voice ID
    /// @param direction 方向 (dx, dy, dz)
    virtual void SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) = 0;

    /// @brief 设置音频源3D属性
    /// @param voiceId 音频Voice ID
    /// @param attributes 3D属性
    virtual void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) = 0;

    /// @brief 设置监听器（听者）属性
    /// @param listener 监听器属性
    virtual void SetListener(const AudioListener& listener) = 0;

    /// @brief 设置距离模型
    /// @param model 距离模型
    virtual void SetDistanceModel(DistanceModel model) = 0;

    /// @brief 设置多普勒因子
    /// @param factor 多普勒因子 (0.0 - 2.0)
    virtual void SetDopplerFactor(float factor) = 0;

    /// @brief 设置声速
    /// @param speed 声速（米/秒）
    virtual void SetSpeedOfSound(float speed) = 0;

    // ========== 全局控制 ==========

    /// @brief 设置主音量
    /// @param volume 主音量 (0.0 - 1.0)
    virtual void SetMasterVolume(float volume) = 0;

    /// @brief 获取主音量
    /// @return 主音量
    virtual float GetMasterVolume() const = 0;

    // ========== 查询 ==========

    /// @brief 检查音频是否正在播放
    /// @param voiceId 音频Voice ID
    /// @return 是否正在播放
    virtual bool IsPlaying(AudioVoiceId voiceId) = 0;

    /// @brief 检查音频是否已暂停
    /// @param voiceId 音频Voice ID
    /// @return 是否已暂停
    virtual bool IsPaused(AudioVoiceId voiceId) = 0;

    /// @brief 检查音频是否已停止
    /// @param voiceId 音频Voice ID
    /// @return 是否已停止
    virtual bool IsStopped(AudioVoiceId voiceId) = 0;

    /// @brief 获取音频当前播放位置
    /// @param voiceId 音频Voice ID
    /// @return 播放位置（秒），失败返回-1.0f
    virtual float GetPlaybackPosition(AudioVoiceId voiceId) = 0;

    /// @brief 获取音频时长
    /// @param voiceId 音频Voice ID
    /// @return 时长（秒），失败返回-1.0f
    virtual float GetDuration(AudioVoiceId voiceId) = 0;

    /// @brief 获取音频Voice状态
    /// @param voiceId 音频Voice ID
    /// @return Voice状态
    virtual VoiceState GetVoiceState(AudioVoiceId voiceId) = 0;

    /// @brief 获取当前正在播放的音频数量
    /// @return 音频数量
    virtual uint32_t GetPlayingVoiceCount() const = 0;

    // ========== 音效（可选实现） ==========

    /// @brief 应用音效到音频源
    /// @param voiceId 音频Voice ID
    /// @param effectType 音效类型
    /// @param params 音效参数
    /// @return 是否成功
    virtual bool ApplyEffect(AudioVoiceId voiceId, EffectType effectType, const void* params) { return false; }

    /// @brief 移除音频源的所有音效
    /// @param voiceId 音频Voice ID
    virtual void RemoveEffects(AudioVoiceId voiceId) {}

    // ========== 事件系统 ==========

    /// @brief 设置事件回调
    /// @param callback 回调函数
    virtual void SetEventCallback(AudioEventCallback callback) = 0;

    /// @brief 移除事件回调
    virtual void RemoveEventCallback() = 0;

    // ========== 统计信息 ==========

    /// @brief 获取音频统计信息
    /// @return 统计信息
    virtual AudioStats GetStats() const = 0;

    /// @brief 重置统计信息
    virtual void ResetStats() = 0;

    // ========== 调试功能 ==========

    /// @brief 开始性能分析
    virtual void BeginProfile() {}

    /// @brief 结束性能分析
    /// @return 性能报告
    virtual std::string EndProfile() { return ""; }
};

} // namespace Engine::Audio