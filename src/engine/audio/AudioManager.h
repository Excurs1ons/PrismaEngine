#pragma once

#include "AudioTypes.h"
#include "IAudioDevice.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>

namespace PrismaEngine::Audio {

// 前置声明
class IAudioLoader;
class LoadTask;

/// @brief 统一的音频管理器
/// 这是音频系统的主入口，管理音频设备的生命周期和提供便利接口
class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    // ========== 初始化和关闭 ==========

    /// @brief 初始化音频系统
    /// @param desc 音频系统描述
    /// @return 是否初始化成功
    bool Initialize(const AudioDesc& desc = {});

    /// @brief 关闭音频系统
    void Shutdown();

    /// @brief 检查是否已初始化
    bool IsInitialized() const;

    /// @brief 更新音频系统（每帧调用）
    /// @param deltaTime 时间增量（秒）
    void Update(float deltaTime);

    // ========== 基本播放接口（便利方法）==========

    /// @brief 播放音频
    /// @param path 音频文件路径
    /// @param volume 音量 (0-1)
    /// @param volume 是否循环
    /// @return 音频Voice ID
    AudioVoiceId Play(const std::string& path, float volume = 1.0f, bool loop = false);

    /// @brief 播放3D音频
    /// @param path 音频文件路径
    /// @param position 位置 (x, y, z)
    /// @param volume 音量 (0-1)
    /// @param volume 是否循环
    /// @return 音频Voice ID
    AudioVoiceId Play3D(const std::string& path, const float position[3],
                       float volume = 1.0f, bool loop = false);

    /// @brief 播放音频（完整控制）
    /// @param path 音频文件路径
    /// @param desc 播放描述
    /// @return 音频Voice ID
    AudioVoiceId Play(const std::string& path, const PlayDesc& desc);

    /// @brief 直接播放音频剪辑
    /// @param clip 音频剪辑
    /// @param desc 播放描述
    /// @return 音频Voice ID
    AudioVoiceId PlayClip(const AudioClip& clip, const PlayDesc& desc = {});

    // ========== 播放控制 ==========

    /// @brief 停止音频
    /// @param voiceId 音频Voice ID
    void Stop(AudioVoiceId voiceId);

    /// @brief 暂停音频
    /// @param voiceId 音频Voice ID
    void Pause(AudioVoiceId voiceId);

    /// @brief 恢复音频
    /// @param voiceId 音频Voice ID
    void Resume(AudioVoiceId voiceId);

    /// @brief 停止所有音频
    void StopAll();

    /// @brief 暂停所有音频
    void PauseAll();

    /// @brief 恢复所有音频
    void ResumeAll();

    // ========== 实时控制 ==========

    /// @brief 设置音量
    /// @param voiceId 音频Voice ID
    /// @param volume 音量 (0-1)
    void SetVolume(AudioVoiceId voiceId, float volume);

    /// @brief 设置音调
    /// @param voiceId 音频Voice ID
    /// @param pitch 音调倍数 (0.5-2.0)
    void SetPitch(AudioVoiceId voiceId, float pitch);

    /// @brief 设置播放位置
    /// @param voiceId 音频Voice ID
    /// @param time 时间位置（秒）
    void SetPlaybackPosition(AudioVoiceId voiceId, float time);

    // ========== 3D音频 ==========

    /// @brief 设置音频源3D位置
    /// @param voiceId 音频Voice ID
    /// @param x X坐标
    /// @param y Y坐标
    /// @param z Z坐标
    void SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z);

    /// @brief 设置音频源3D属性
    /// @param voiceId 音频Voice ID
    /// @param attributes 3D属性
    void SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes);

    /// @brief 设置监听器（听者）
    /// @param listener 监听器属性
    void SetListener(const AudioListener& listener);

    // ========== 全局控制 ==========

    /// @brief 设置主音量
    /// @param volume 主音量 (0-1)
    void SetMasterVolume(float volume);

    /// @brief 获取主音量
    float GetMasterVolume() const;

    /// @brief 设置距离模型
    /// @param model 距离模型
    void SetDistanceModel(DistanceModel model);

    // ========== 资源管理 ==========

    /// @brief 加载音频剪辑
    /// @param path 音频文件路径
    /// @param async 是否异步加载
    /// @return 音频剪辑智能指针
    std::shared_ptr<AudioClip> LoadClip(const std::string& path, bool async = false);

    /// @brief 异步加载音频剪辑
    /// @param path 音频文件路径
    /// @return 加载任务
    LoadTask LoadClipAsync(const std::string& path);

    /// @brief 卸载音频剪辑
    /// @param path 音频文件路径
    void UnloadClip(const std::string& path);

    /// @brief 预加载音频
    /// @param paths 音频文件路径列表
    void Preload(const std::vector<std::string>& paths);

    // ========== 查询 ==========

    /// @brief 检查音频是否正在播放
    /// @param voiceId 音频Voice ID
    bool IsPlaying(AudioVoiceId voiceId);

    /// @brief 检查音频是否已暂停
    /// @param voiceId 音频Voice ID
    bool IsPaused(AudioVoiceId voiceId);

    /// @brief 检查音频是否已停止
    /// @param voiceId 音频Voice ID
    bool IsStopped(AudioVoiceId voiceId);

    /// @brief 获取音频播放位置
    /// @param voiceId 音频Voice ID
    /// @return 播放位置（秒）
    float GetPlaybackPosition(AudioVoiceId voiceId);

    /// @brief 获取音频时长
    /// @param voiceId 音频Voice ID
    /// @return 时长（秒）
    float GetDuration(AudioVoiceId voiceId);

    /// @brief 获取当前正在播放的音频数量
    uint32_t GetPlayingVoiceCount() const;

    // ========== 设备信息 ==========

    /// @brief 获取当前设备信息
    DeviceInfo GetDeviceInfo() const;

    /// @brief 获取所有可用设备
    std::vector<DeviceInfo> GetAvailableDevices() const;

    /// @brief 设置音频设备
    /// @param deviceName 设备名称
    /// @return 是否成功
    bool SetDevice(const std::string& deviceName);

    /// @brief 获取音频设备类型
    AudioDeviceType GetDeviceType() const;

    // ========== 事件系统 ==========

    /// @brief 设置事件回调
    /// @param callback 回调函数
    void SetEventCallback(AudioEventCallback callback);

    /// @brief 移除事件回调
    void RemoveEventCallback();

    // ========== 统计信息 ==========

    /// @brief 获取音频统计信息
    AudioStats GetStats() const;

    /// @brief 重置统计信息
    void ResetStats();

    // ========== 调试功能 ==========

    /// @brief 开始性能分析
    void BeginProfile();

    /// @brief 结束性能分析
    /// @return 性能报告
    std::string EndProfile();

    /// @brief 生成调试报告
    /// @return 调试信息
    std::string GenerateDebugReport();

    // ========== 访问设备（高级用户）==========

    /// @brief 获取音频设备
    /// @warning 谨慎使用，可能绕过管理器的某些功能
    IAudioDevice* GetDevice() const;

private:
    // ========== 内部方法 ==========

    /// @brief 创建音频设备
    /// @param deviceType 设备类型
    /// @return 设备指针
    std::unique_ptr<IAudioDevice> CreateDevice(AudioDeviceType deviceType);

    /// @brief 自动选择最佳设备
    /// @param hint 提示的设备类型
    /// @return 选择的实际设备类型
    AudioDeviceType SelectBestDevice(AudioDeviceType hint);

    /// @brief 处理音频事件
    void HandleAudioEvent(const AudioEvent& event);

    /// @brief 清理已完成的Voice
    void CleanupFinishedVoices();

    /// @brief 格式化文件路径
    std::string FormatPath(const std::string& path);

    // ========== 成员变量 ==========

    // 音频设备
    std::unique_ptr<IAudioDevice> m_device;
    AudioDeviceType m_currentDevice = AudioDeviceType::Auto;

    // 音频加载器
    std::unique_ptr<IAudioLoader> m_loader;

    // 音频剪辑缓存
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> m_clipCache;

    // 活跃Voice列表（用于快速查找）
    std::unordered_map<AudioVoiceId, std::string> m_voiceToClip;

    // 配置
    AudioDesc m_desc;

    // 状态
    std::atomic<bool> m_initialized{false};
    mutable std::mutex m_mutex;

    // 统计
    mutable AudioStats m_stats;

    // 事件处理
    AudioEventCallback m_eventCallback;
};

// ========== 便利函数 ==========

/// @brief 获取全局音频管理器单例
/// @note 这是一个便利函数，建议使用非单例版本以便更好地控制生命周期
AudioManager& GetAudioManager();

} // namespace Engine::Audio