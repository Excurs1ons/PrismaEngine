#include "AudioDeviceNull.h"
#include "../Logger.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace PrismaEngine::Audio {

AudioDeviceNull::AudioDeviceNull() {
    LOG_INFO("Audio", "创建空音频设备");
}

AudioDeviceNull::~AudioDeviceNull() {
    if (m_initialized) {
        Shutdown();
    }
    LOG_INFO("Audio", "销毁空音频设备");
}

bool AudioDeviceNull::Initialize(const AudioDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        LOG_WARNING("Audio", "空音频设备已经初始化");
        return true;
    }

    // 保存配置
    m_masterVolume = 1.0f;
    m_distanceModel = desc.distanceModel;
    m_listener = AudioListener{};

    // 重置统计
    m_stats = AudioStats{};
    m_stats.maxVoices = desc.maxVoices;
    m_stats.activeVoices = 0;

    m_initialized = true;
    LOG_INFO("Audio", "空音频设备初始化成功");
    return true;
}

void AudioDeviceNull::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    // 停止所有Voice
    StopAll();
    m_voices.clear();

    m_initialized = false;
    LOG_INFO("Audio", "空音频设备已关闭");
}

bool AudioDeviceNull::IsInitialized() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_initialized;
}

AudioVoiceId AudioDeviceNull::PlayClip(const AudioClip& clip, const PlayDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return INVALID_VOICE_ID;
    }

    AudioVoiceId voiceId = GenerateVoiceId();
    VoiceState voice;
    voice.id = voiceId;
    voice.playing = true;
    voice.paused = false;
    voice.looping = desc.loop;
    voice.volume = desc.volume;
    voice.pitch = desc.pitch;
    voice.position = 0.0f;
    voice.duration = clip.duration;
    voice.velocity[0] = 0.0f;
    voice.velocity[1] = 0.0f;
    voice.velocity[2] = 0.0f;
    voice.direction[0] = 0.0f;
    voice.direction[1] = 0.0f;
    voice.direction[2] = 1.0f;
    voice.desc = desc;

    m_voices[voiceId] = voice;

    // 更新统计
    m_stats.activeVoices = static_cast<uint32_t>(m_voices.size());
    m_stats.totalVoicesCreated++;
    m_stats.maxConcurrentVoices = std::max(m_stats.maxConcurrentVoices, m_stats.activeVoices);

    // 触发事件
    if (m_eventCallback) {
        AudioEvent event;
        event.type = AudioEventType::VoiceStarted;
        event.voiceId = voiceId;
        event.timestamp = 0; // 实际应该使用真实时间戳
        m_eventCallback(event);
    }

    LOG_DEBUG("Audio", "空音频设备播放声音，Voice ID: {}", voiceId);
    return voiceId;
}

AudioVoiceId AudioDeviceNull::Play(const AudioClip& clip, const PlayDesc& desc) {
    return PlayClip(clip, desc);
}

void AudioDeviceNull::Stop(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        if (m_eventCallback && it->second.playing) {
            AudioEvent event;
            event.type = AudioEventType::VoiceStopped;
            event.voiceId = voiceId;
            event.timestamp = 0;
            m_eventCallback(event);
        }

        m_voices.erase(it);
        m_stats.activeVoices = static_cast<uint32_t>(m_voices.size());
        LOG_DEBUG("Audio", "空音频设备停止声音，Voice ID: {}", voiceId);
    }
}

void AudioDeviceNull::Pause(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice && voice->playing && !voice->paused) {
        voice->paused = true;

        if (m_eventCallback) {
            AudioEvent event;
            event.type = AudioEventType::VoicePaused;
            event.voiceId = voiceId;
            event.timestamp = 0;
            m_eventCallback(event);
        }

        LOG_DEBUG("Audio", "空音频设备暂停声音，Voice ID: {}", voiceId);
    }
}

void AudioDeviceNull::Resume(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice && voice->playing && voice->paused) {
        voice->paused = false;

        if (m_eventCallback) {
            AudioEvent event;
            event.type = AudioEventType::VoiceResumed;
            event.voiceId = voiceId;
            event.timestamp = 0;
            m_eventCallback(event);
        }

        LOG_DEBUG("Audio", "空音频设备恢复声音，Voice ID: {}", voiceId);
    }
}

void AudioDeviceNull::PauseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_voices) {
        auto& voice = pair.second;
        if (voice.playing && !voice.paused) {
            voice.paused = true;

            if (m_eventCallback) {
                AudioEvent event;
                event.type = AudioEventType::VoicePaused;
                event.voiceId = voice.id;
                event.timestamp = 0;
                m_eventCallback(event);
            }
        }
    }

    LOG_DEBUG("Audio", "空音频设备暂停所有声音");
}

void AudioDeviceNull::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_voices) {
        auto& voice = pair.second;
        if (voice.playing && voice.paused) {
            voice.paused = false;

            if (m_eventCallback) {
                AudioEvent event;
                event.type = AudioEventType::VoiceResumed;
                event.voiceId = voice.id;
                event.timestamp = 0;
                m_eventCallback(event);
            }
        }
    }

    LOG_DEBUG("Audio", "空音频设备恢复所有声音");
}

void AudioDeviceNull::StopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& pair : m_voices) {
        if (m_eventCallback && pair.second.playing) {
            AudioEvent event;
            event.type = AudioEventType::VoiceStopped;
            event.voiceId = pair.first;
            event.timestamp = 0;
            m_eventCallback(event);
        }
    }

    m_voices.clear();
    m_stats.activeVoices = 0;
    LOG_DEBUG("Audio", "空音频设备停止所有声音");
}

void AudioDeviceNull::SetVolume(AudioVoiceId voiceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->volume = volume;
    }
}

void AudioDeviceNull::SetPitch(AudioVoiceId voiceId, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->pitch = pitch;
    }
}

void AudioDeviceNull::SetPlaybackPosition(AudioVoiceId voiceId, float time) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->position = std::clamp(time, 0.0f, voice->duration);
    }
}

void AudioDeviceNull::SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->desc.spatial.position[0] = x;
        voice->desc.spatial.position[1] = y;
        voice->desc.spatial.position[2] = z;
    }
}

void AudioDeviceNull::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->desc.spatial.position[0] = position[0];
        voice->desc.spatial.position[1] = position[1];
        voice->desc.spatial.position[2] = position[2];
    }
}

void AudioDeviceNull::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->velocity[0] = velocity[0];
        voice->velocity[1] = velocity[1];
        voice->velocity[2] = velocity[2];
    }
}

void AudioDeviceNull::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->direction[0] = direction[0];
        voice->direction[1] = direction[1];
        voice->direction[2] = direction[2];
    }
}

void AudioDeviceNull::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto voice = FindVoice(voiceId);
    if (voice) {
        voice->desc.spatial = attributes;
    }
}

void AudioDeviceNull::SetListener(const AudioListener& listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listener = listener;
}

void AudioDeviceNull::SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

float AudioDeviceNull::GetMasterVolume() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_masterVolume;
}

void AudioDeviceNull::SetDistanceModel(DistanceModel model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_distanceModel = model;
}

void AudioDeviceNull::SetDopplerFactor(float factor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dopplerFactor = factor;
}

void AudioDeviceNull::SetSpeedOfSound(float speed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_speedOfSound = speed;
}

bool AudioDeviceNull::IsPlaying(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto voice = FindVoice(voiceId);
    return voice && voice->playing && !voice->paused;
}

bool AudioDeviceNull::IsPaused(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto voice = FindVoice(voiceId);
    return voice && voice->paused;
}

bool AudioDeviceNull::IsStopped(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return FindVoice(voiceId) == nullptr;
}

float AudioDeviceNull::GetPlaybackPosition(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto voice = FindVoice(voiceId);
    return voice ? voice->position : 0.0f;
}

float AudioDeviceNull::GetDuration(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto voice = FindVoice(voiceId);
    return voice ? voice->duration : 0.0f;
}

VoiceState AudioDeviceNull::GetVoiceState(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    VoiceState state = VoiceState::Stopped;
    auto voice = FindVoice(voiceId);
    if (voice) {
        if (voice->playing && !voice->paused) {
            state = VoiceState::Playing;
        } else if (voice->paused) {
            state = VoiceState::Paused;
        }
    }
    return state;
}

uint32_t AudioDeviceNull::GetPlayingVoiceCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.activeVoices;
}

DeviceInfo AudioDeviceNull::GetDeviceInfo() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    DeviceInfo info;
    info.name = "Null Audio Device";
    info.version = "1.0";
    info.extensions = "None";
    info.maxVoices = 1024;
    info.sampleRate = 44100;
    info.channels = 2;
    info.supports3D = false;
    info.supportsEffects = false;

    return info;
}

std::vector<DeviceInfo> AudioDeviceNull::GetAvailableDevices() const {
    return { GetDeviceInfo() };
}

bool AudioDeviceNull::SetDevice(const std::string& deviceName) {
    // Null设备不支持切换
    return deviceName == "Null Audio Device";
}

AudioDeviceType AudioDeviceNull::GetDeviceType() const {
    return AudioDeviceType::Null;
}

void AudioDeviceNull::SetEventCallback(AudioEventCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventCallback = callback;
}

void AudioDeviceNull::RemoveEventCallback() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventCallback = nullptr;
}

AudioStats AudioDeviceNull::GetStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void AudioDeviceNull::ResetStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.activeVoices = static_cast<uint32_t>(m_voices.size());
    m_stats.totalVoicesCreated = 0;
    m_stats.maxConcurrentVoices = m_stats.activeVoices;
    m_stats.memoryUsage = 0;
    m_stats.cpuUsage = 0.0f;
    m_stats.averageLatency = 0.0f;
    m_stats.dropouts = 0;
    m_stats.underruns = 0;
}

void AudioDeviceNull::BeginProfile() {
    // 空实现
}

std::string AudioDeviceNull::EndProfile() {
    return "空音频设备性能分析";
}

std::string AudioDeviceNull::GenerateDebugReport() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string report = "=== 空音频设备调试报告 ===\n";
    report += "初始化状态: " + std::string(m_initialized ? "已初始化" : "未初始化") + "\n";
    report += "主音量: " + std::to_string(m_masterVolume) + "\n";
    report += "活跃Voice数: " + std::to_string(m_voices.size()) + "\n";
    report += "距离模型: " + std::to_string(static_cast<int>(m_distanceModel)) + "\n";
    report += "========================\n";

    return report;
}

void AudioDeviceNull::Update(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 模拟播放进度
    for (auto it = m_voices.begin(); it != m_voices.end();) {
        auto& voice = it->second;

        if (voice.playing && !voice.paused) {
            // 更新播放位置
            voice.position += deltaTime * voice.pitch;

            // 检查是否播放完毕
            if (voice.position >= voice.duration) {
                if (voice.looping) {
                    voice.position = std::fmod(voice.position, voice.duration);

                    if (m_eventCallback) {
                        AudioEvent event;
                        event.type = AudioEventType::VoiceLooped;
                        event.voiceId = voice.id;
                        event.timestamp = 0;
                        m_eventCallback(event);
                    }
                } else {
                    // 播放完毕，移除
                    if (m_eventCallback) {
                        AudioEvent event;
                        event.type = AudioEventType::VoiceStopped;
                        event.voiceId = voice.id;
                        event.timestamp = 0;
                        m_eventCallback(event);
                    }

                    it = m_voices.erase(it);
                    m_stats.activeVoices = static_cast<uint32_t>(m_voices.size());
                    continue;
                }
            }
        }
        ++it;
    }
}

AudioVoiceId AudioDeviceNull::GenerateVoiceId() {
    AudioVoiceId id = m_nextVoiceId++;
    if (id == INVALID_VOICE_ID) {
        id = m_nextVoiceId++;
    }
    return id;
}

AudioDeviceNull::VoiceState* AudioDeviceNull::FindVoice(AudioVoiceId voiceId) {
    auto it = m_voices.find(voiceId);
    return (it != m_voices.end()) ? &it->second : nullptr;
}

} // namespace Engine::Audio