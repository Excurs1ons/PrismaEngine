#pragma once

#include "AudioTypes.h"
#include "core/Timestep.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Audio {

// 音频设备抽象接口
/// 这是音频系统的核心抽象，不同的音频后端只需要实现这一个接口
class IAudioDevice {
public:
    virtual ~IAudioDevice()                                                          = default;
    virtual AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc = {})  = 0;
    virtual void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) = 0;
    virtual std::string GenerateDebugReport()                                        = 0;
    // ========== 初始化和关闭 ==========
    virtual AudioDeviceType GetDeviceType() const = 0;
    // 初始化音频设备
    virtual bool Initialize(const AudioDesc& desc) = 0;

    // 关闭音频设备
    virtual void Shutdown() = 0;

    // 检查设备是否已初始化
    virtual bool IsInitialized() const = 0;

    // 更新音频设备（每帧调用）
    virtual void Update(Prisma::Timestep ts) = 0;

    // ========== 设备信息 ==========

    // 设备信息
    struct DeviceInfo {
        std::string name;              // 设备名称
        std::string driver;            // 驱动名称
        std::string version;           // 版本号
        bool isDefault       = false;  // 是否为默认设备
        uint32_t maxVoices   = 0;      // 最大支持的并发音源数
        bool supports3D      = false;  // 是否支持3D音频
        bool supportsEffects = false;  // 是否支持音效
    };

    // 获取当前设备信息
    virtual DeviceInfo GetDeviceInfo() const = 0;

    // 获取所有可用设备列表
    virtual std::vector<DeviceInfo> GetAvailableDevices() const = 0;

    // 设置当前设备（如果支持）
    virtual bool SetDevice(const std::string&) {
        return false;
    }

    // ========== 基本播放控制 ==========

    // 播放音频
    virtual AudioVoiceId Play(const AudioClip& clip, const PlayDesc& desc = {}) = 0;

    // 停止播放
    virtual void Stop(AudioVoiceId voiceId) = 0;

    // 暂停播放
    virtual void Pause(AudioVoiceId voiceId) = 0;

    // 恢复播放
    virtual void Resume(AudioVoiceId voiceId) = 0;

    // 停止所有正在播放的音频
    virtual void StopAll() = 0;

    // 暂停所有正在播放的音频
    virtual void PauseAll() = 0;

    // 恢复所有暂停的音频
    virtual void ResumeAll() = 0;

    // ========== 实时控制 ==========

    // 设置音量
    virtual void SetVolume(AudioVoiceId voiceId, float volume) = 0;

    // 设置音调
    virtual void SetPitch(AudioVoiceId voiceId, float pitch) = 0;

    // 设置播放位置
    virtual void SetPlaybackPosition(AudioVoiceId voiceId, float time) = 0;

    // ========== 3D音频 ==========

    // 设置音频源3D位置
    virtual void SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) = 0;

    // 设置音频源3D速度
    virtual void SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) = 0;

    // 设置音频源3D direction
    virtual void SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) = 0;

    // 设置音频源3D属性
    virtual void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) = 0;

    // 设置监听器（听者）属性
    virtual void SetListener(const AudioListener& listener) = 0;

    // 设置距离模型
    virtual void SetDistanceModel(DistanceModel model) = 0;

    // 设置多普勒因子
    virtual void SetDopplerFactor(float factor) = 0;

    // 设置声速
    virtual void SetSpeedOfSound(float speed) = 0;

    // ========== 全局控制 ==========

    // 设置主音量
    virtual void SetMasterVolume(float volume) = 0;

    // 获取主音量
    virtual float GetMasterVolume() const = 0;

    // ========== 查询 ==========

    // 检查音频是否正在播放
    virtual bool IsPlaying(AudioVoiceId voiceId) = 0;

    // 检查音频是否已暂停
    virtual bool IsPaused(AudioVoiceId voiceId) = 0;

    // 检查音频是否已停止
    virtual bool IsStopped(AudioVoiceId voiceId) = 0;

    // 获取音频当前播放位置
    virtual float GetPlaybackPosition(AudioVoiceId voiceId) = 0;

    // 获取音频时长
    virtual float GetDuration(AudioVoiceId voiceId) = 0;

    // 获取音频Voice状态
    virtual VoiceState GetVoiceState(AudioVoiceId voiceId) = 0;

    // 获取当前正在播放的音频数量
    virtual uint32_t GetPlayingVoiceCount() const = 0;

    // ========== 音效（可选实现） ==========

    // 应用音效到音频源
    virtual bool ApplyEffect(AudioVoiceId, EffectType, const void*) {
        return false;
    }

    // 移除音频源的所有音效
    virtual void RemoveEffects(AudioVoiceId) {}

    // ========== 事件系统 ==========

    // 设置事件回调
    virtual void SetEventCallback(AudioEventCallback callback) = 0;

    // 移除事件回调
    virtual void RemoveEventCallback() = 0;

    // ========== 统计信息 ==========

    // 获取音频统计信息
    virtual AudioStats GetStats() const = 0;

    // 重置统计信息
    virtual void ResetStats() = 0;

    // ========== 调试功能 ==========

    // 开始性能分析
    virtual void BeginProfile() {}

    // 结束性能分析
    virtual std::string EndProfile() { return ""; }
};

}  // namespace Prisma::Audio
