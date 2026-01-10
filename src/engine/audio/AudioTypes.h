#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <future>
#include <atomic>
#include <functional>
#include <chrono>

// 使用新系统的 AudioFormat 定义
#include "core/IAudioDriver.h"

namespace PrismaEngine::Audio {

// 前置声明
struct AudioClip;
struct PlayDesc;

// 音频Voice ID
using AudioVoiceId = uint32_t;
constexpr AudioVoiceId INVALID_VOICE_ID = 0xFFFFFFFF;

// 音频设备类型
enum class AudioDeviceType : char {
    Auto = -1,      // 自动选择
    OpenAL = 0,     // OpenAL (跨平台)
    XAudio2 = 1,    // XAudio2 (Windows)
    AAudio = 2,     // AAudio (Android)
    SDL3 = 3,       // SDL3 Audio (跨平台)
    Null = 4        // 空实现（静音）
};

// DeviceInfo 已在 IAudioDevice.h 中定义（新系统）
// AudioFormat 已在 core/IAudioDriver.h 中定义（新系统）

// 音频剪辑 - 统一的音频数据容器
struct AudioClip {
    AudioFormat format;
    std::vector<uint8_t> data;    // 原始PCM数据
    float duration = 0.0f;        // 持续时间（秒）
    std::string path;             // 来源路径（用于调试）

    // 便利方法
    [[nodiscard]] size_t GetSampleCount() const {
        return data.size() / (format.bitsPerSample / 8);
    }

    [[nodiscard]] size_t GetFrameCount() const {
        return GetSampleCount() / format.channels;
    }

    [[nodiscard]] size_t GetSizeInBytes() const { return data.size(); }
    [[nodiscard]] bool IsValid() const { return !data.empty(); }
};

// 3D音频属性
struct Audio3DAttributes {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    float direction[3] = {0.0f, 0.0f, 0.0f};  // 音源朝向

    // 距离衰减参数
    float minDistance = 1.0f;        // 最小距离（开始衰减）
    float maxDistance = 100.0f;      // 最大距离（完全静音）
    float rolloffFactor = 1.0f;      // 衰减系数

    // 锥形参数（方向性音源）
    float coneInnerAngle = 360.0f;   // 内锥角度（度）
    float coneOuterAngle = 360.0f;   // 外锥角度（度）
    float coneOuterGain = 0.0f;      // 外锥增益（0-1）
};

// 音频监听器（3D音频中的"耳朵"）
struct AudioListener {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float velocity[3] = {0.0f, 0.0f, 0.0f};
    float forward[3] = {0.0f, 0.0f, -1.0f};   // 前向量
    float up[3] = {0.0f, 1.0f, 0.0f};        // 上向量
};

// 播放描述符
struct PlayDesc {
    float volume = 1.0f;              // 音量倍数 (0-1)
    float pitch = 1.0f;               // 音调倍数 (0.5-2.0)
    bool loop = false;                // 是否循环
    bool is3D = false;                // 是否为3D音频
    Audio3DAttributes spatial;        // 3D属性（仅当is3D=true时有效）

    // 时间控制
    float startTime = 0.0f;           // 开始时间（秒）
    float endTime = -1.0f;            // 结束时间（-1表示播放到结尾）

    // 优先级（0=最高，255=最低）
    uint8_t priority = 128;
};

// 距离模型
enum class DistanceModel : int {
    None = 0,            // 无距离衰减
    Inverse = 1,         // 反比衰减
    InverseClamped = 2,  // 反比衰减（限制）
    Linear = 3,          // 线性衰减
    LinearClamped = 4,   // 线性衰减（限制）
    Exponential = 5,     // 指数衰减
    ExponentialClamped = 6 // 指数衰减（限制）
};

// 音效类型
enum class EffectType : int {
    None = 0,
    Reverb = 1,
    Chorus = 2,
    Distortion = 3,
    Echo = 4,
    Flanger = 5,
    FrequencyShifter = 6,
    VocalMorpher = 7,
    PitchShifter = 8,
    RingModulator = 9,
    Autowah = 10,
    Compressor = 11,
    Equalizer = 12
};

// 设备初始化描述
struct AudioDesc {
    AudioDeviceType deviceType = AudioDeviceType::Auto;
    AudioFormat outputFormat;

    // 设备配置
    std::string deviceName;          // 指定设备名（空=默认）
    uint32_t maxVoices = 256;        // 最大并发音源数
    uint32_t bufferSize = 512;       // 缓冲区大小（采样数）
    bool enableDebug = false;        // 是否启用调试
    bool enableHRTF = false;         // 是否启用HRTF（双耳音频）

    // 3D音频设置
    DistanceModel distanceModel = DistanceModel::InverseClamped;
    float dopplerFactor = 1.0f;      // 多普勒效应因子
    float speedOfSound = 343.3f;     // 声速（米/秒）

    // 音效设置
    bool enableEffects = false;      // 是否启用音效
    uint32_t maxEffects = 32;        // 最大同时音效数
};

// 音频Voice状态
enum class VoiceState : uint8_t {
    Stopped = 0,     // 停止
    Playing = 1,     // 播放中
    Paused = 2,      // 暂停
    Transitioning = 3 // 状态转换中（如淡入淡出）
};

// 音频统计信息
struct AudioStats {
    uint32_t activeVoices = 0;       // 当前活跃音源数
    uint32_t totalVoicesCreated = 0;  // 总创建的音源数
    uint32_t maxConcurrentVoices = 0; // 历史最大并发数
    uint64_t memoryUsage = 0;        // 内存使用量（字节）
    float cpuUsage = 0.0f;           // CPU使用率（百分比）

    // 性能指标
    float averageLatency = 0.0f;     // 平均延迟（毫秒）
    uint32_t dropouts = 0;           // 音频中断次数
    uint32_t underruns = 0;          // 缓冲区欠载次数
};

// 音频事件类型
enum class AudioEventType : uint8_t {
    VoiceStarted = 0,    // 音源开始播放
    VoiceStopped = 1,    // 音源停止
    VoicePaused = 2,     // 音源暂停
    VoiceResumed = 3,    // 音源恢复
    VoiceLooped = 4,     // 音源循环
    StreamBuffering = 5, // 流媒体缓冲
    DeviceLost = 6,      // 设备丢失
    DeviceRestored = 7   // 设备恢复
};

// 音频事件
struct AudioEvent {
    AudioEventType type;
    AudioVoiceId voiceId = INVALID_VOICE_ID;
    std::string message;  // 附加信息
    uint64_t timestamp;   // 事件时间戳
};

// 音频事件回调
using AudioEventCallback = std::function<void(const AudioEvent&)>;

// 异步加载任务
class LoadTask {
public:
    LoadTask(std::future<std::shared_ptr<AudioClip>>&& future, const std::string& path)
        : m_future(std::move(future)), m_path(path) {}

    bool IsReady() {
        return m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    std::shared_ptr<AudioClip> GetResult() {
        return m_future.get();
    }

    const std::string& GetPath() const { return m_path; }

private:
    std::future<std::shared_ptr<AudioClip>> m_future;
    std::string m_path;
};

} // namespace Engine::Audio