#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <DirectXMath.h>

namespace Engine {
namespace Audio {

// 音频格式
enum class AudioFormat {
    Unknown,
    PCM,
    Vorbis,
    MP3,
    WAV,
    FLAC
};

// 音频采样率
enum class SampleRate {
    Hz22050 = 22050,
    Hz44100 = 44100,
    Hz48000 = 48000,
    Hz96000 = 96000
};

// 音频声道数
enum class AudioChannels {
    Mono = 1,
    Stereo = 2,
    Quad = 4,
    FivePointOne = 6,
    SevenPointOne = 8
};

// 音频数据
struct AudioData {
    AudioFormat format = AudioFormat::PCM;
    SampleRate sampleRate = SampleRate::Hz44100;
    AudioChannels channels = AudioChannels::Stereo;
    uint32_t bitsPerSample = 16;
    std::vector<uint8_t> rawData;
    uint32_t size = 0;
    float duration = 0.0f;
};

// 音频状态
enum class AudioState {
    Stopped,
    Playing,
    Paused,
    FadingIn,
    FadingOut
};

// 音频源
class AudioSource {
public:
    AudioSource();
    ~AudioSource();

    // 音频控制
    void Play();
    void Pause();
    void Stop();
    void Resume();

    // 属性设置
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetPan(float pan); // -1.0 (left) to 1.0 (right)
    void SetLoop(bool loop);

    // 3D音频
    void Set3DPosition(const DirectX::XMFLOAT3& position);
    void Set3DVelocity(const DirectX::XMFLOAT3& velocity);
    void Set3DDistance(float minDistance, float maxDistance);
    void Set3DCone(float innerAngle, float outerAngle, float outerVolume);

    // 获取状态
    AudioState GetState() const { return m_state; }
    float GetVolume() const { return m_volume; }
    float GetPitch() const { return m_pitch; }
    float GetPan() const { return m_pan; }
    bool IsLooping() const { return m_loop; }

    // 获取播放进度
    float GetPlaybackTime() const;
    void SetPlaybackTime(float time);

    // 音频数据
    void SetAudioData(std::shared_ptr<const AudioData> data);
    std::shared_ptr<const AudioData> GetAudioData() const { return m_audioData; }

private:
    // 音频数据
    std::shared_ptr<const AudioData> m_audioData;

    // 状态
    AudioState m_state = AudioState::Stopped;
    float m_volume = 1.0f;
    float m_pitch = 1.0f;
    float m_pan = 0.0f;
    bool m_loop = false;

    // 播放控制
    uint32_t m_currentSample = 0;
    float m_playbackTime = 0.0f;

    // 3D属性
    DirectX::XMFLOAT3 m_position = DirectX::XMFLOAT3(0, 0, 0);
    DirectX::XMFLOAT3 m_velocity = DirectX::XMFLOAT3(0, 0, 0);
    float m_minDistance = 1.0f;
    float m_maxDistance = 100.0f;
    float m_innerConeAngle = DirectX::XM_PIDIV2;
    float m_outerConeAngle = DirectX::XM_PI;
    float m_outerConeVolume = 0.0f;

    // 内部缓冲区句柄
    void* m_bufferHandle = nullptr;
    void* m_sourceHandle = nullptr;
};

// 音频监听器（3D音频）
class AudioListener {
public:
    AudioListener();

    // 位置和朝向
    void SetPosition(const DirectX::XMFLOAT3& position);
    void SetVelocity(const DirectX::XMFLOAT3& velocity);
    void SetOrientation(const DirectX::XMFLOAT3& forward, const DirectX::XMFLOAT3& up);

    // 获取属性
    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
    const DirectX::XMFLOAT3& GetVelocity() const { return m_velocity; }
    const DirectX::XMFLOAT3& GetForward() const { return m_forward; }
    const DirectX::XMFLOAT3& GetUp() const { return m_up; }

private:
    DirectX::XMFLOAT3 m_position = DirectX::XMFLOAT3(0, 0, 0);
    DirectX::XMFLOAT3 m_velocity = DirectX::XMFLOAT3(0, 0, 0);
    DirectX::XMFLOAT3 m_forward = DirectX::XMFLOAT3(0, 0, -1);
    DirectX::XMFLOAT3 m_up = DirectX::XMFLOAT3(0, 1, 0);
};

// 音频管理器
class AudioManager {
public:
    static AudioManager& GetInstance();

    // 初始化
    bool Initialize(int maxSources = 256);
    void Shutdown();

    // 音频资源管理
    std::shared_ptr<const AudioData> LoadAudio(const std::string& filePath);
    void UnloadAudio(const std::string& filePath);
    void UnloadAllAudio();

    // 音频源管理
    std::shared_ptr<AudioSource> CreateSource();
    void DestroySource(std::shared_ptr<AudioSource> source);

    // 快速播放
    std::shared_ptr<AudioSource> PlayAudio(const std::string& filePath,
                                          float volume = 1.0f,
                                          bool loop = false);

    // 3D音频
    std::shared_ptr<AudioSource> PlayAudio3D(const std::string& filePath,
                                           const DirectX::XMFLOAT3& position,
                                           float volume = 1.0f,
                                           bool loop = false);

    // 监听器
    const AudioListener& GetListener() const { return m_listener; }
    void SetListener(const AudioListener& listener) { m_listener = listener; }

    // 主音量
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }

    // 音频设备
    struct DeviceInfo {
        std::string name;
        std::string driver;
        bool isDefault = false;
        int maxSources = 0;
    };

    std::vector<DeviceInfo> GetAvailableDevices() const;
    bool SetDevice(const std::string& deviceName);
    std::string GetCurrentDevice() const { return m_currentDevice; }

    // 更新（每帧调用）
    void Update();

    // 音频设置
    struct AudioSettings {
        SampleRate sampleRate = SampleRate::Hz44100;
        AudioChannels channels = AudioChannels::Stereo;
        int bufferSize = 512;
        int maxSources = 256;
        bool enableHRTF = false; // Head-related transfer function
        float dopplerFactor = 1.0f;
        float speedOfSound = 343.3f; // m/s
    };

    void ApplySettings(const AudioSettings& settings);
    const AudioSettings& GetSettings() const { return m_settings; }

    // 调试信息
    struct AudioStats {
        uint32_t loadedAudioFiles = 0;
        uint32_t activeSources = 0;
        uint32_t playingSources = 0;
        uint64_t memoryUsage = 0;
    };

    AudioStats GetStats() const;

private:
    AudioManager() = default;
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // 音频加载
    std::shared_ptr<AudioData> LoadWAV(const std::string& filePath);
    std::shared_ptr<AudioData> LoadOGG(const std::string& filePath);
    std::shared_ptr<AudioData> LoadMP3(const std::string& filePath);

    // 音频解码
    bool DecodeAudio(const std::string& filePath, AudioData& outData);

    // 线程安全
    mutable std::mutex m_mutex;

    // 音频资源缓存
    std::unordered_map<std::string, std::shared_ptr<const AudioData>> m_audioCache;

    // 音频源池
    std::vector<std::shared_ptr<AudioSource>> m_sources;
    std::vector<std::shared_ptr<AudioSource>> m_availableSources;

    // 监听器
    AudioListener m_listener;

    // 设置
    AudioSettings m_settings;
    float m_masterVolume = 1.0f;
    std::string m_currentDevice;

    // 统计
    mutable AudioStats m_stats;

    // 是否已初始化
    bool m_initialized = false;

    // 内部句柄
    void* m_audioContext = nullptr;
    void* m_audioDevice = nullptr;
};

// 便利函数
inline AudioManager& GetAudioManager() {
    return AudioManager::GetInstance();
}

// 快速音频播放
inline void PlaySound(const std::string& filePath) {
    GetAudioManager().PlayAudio(filePath);
}

inline void PlaySound3D(const std::string& filePath, const DirectX::XMFLOAT3& pos) {
    GetAudioManager().PlayAudio3D(filePath, pos);
}

} // namespace Audio
} // namespace Engine