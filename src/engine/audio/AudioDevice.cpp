#include "AudioDevice.h"

// 平台驱动
#if defined(_WIN32) && defined(PRISMA_ENABLE_AUDIO_XAUDIO2)
    #include "drivers/AudioDriverXAudio2.h"
#elif defined(__ANDROID__) && defined(PRISMA_ENABLE_AUDIO_AAUDIO)
    #include "drivers/AudioDriverAAudio.h"
#endif

#include <algorithm>
#include <cmath>

namespace PrismaEngine::Audio {

// ========== AudioDevice ==========

AudioDevice::AudioDevice() = default;

AudioDevice::~AudioDevice() {
    Shutdown();
}

bool AudioDevice::Initialize(const AudioDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized.load()) {
        return true;
    }

    m_desc = desc;

    // 创建驱动
    m_driver = CreateDriver(desc.deviceType);
    if (!m_driver) {
        return false;
    }

    // 初始化驱动
    AudioFormat format(desc.outputFormat.sampleRate, desc.outputFormat.channels,
                      desc.outputFormat.bitsPerSample);
    if (format.sampleRate == 0) format.sampleRate = 48000;
    if (format.channels == 0) format.channels = 2;
    if (format.bitsPerSample == 0) format.bitsPerSample = 16;

    format = m_driver->Initialize(format);
    if (!m_driver->IsInitialized()) {
        m_driver.reset();
        return false;
    }

    // 设置缓冲区结束回调
    m_driver->SetBufferEndCallback(OnBufferEnd, this);

    m_initialized.store(true);
    return true;
}

void AudioDevice::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 停止所有 Voice
    for (auto& [id, voice] : m_voices) {
        if (voice.driverSourceId != IAudioDriver::InvalidSource) {
            m_driver->Stop(voice.driverSourceId);
            m_driver->DestroySource(voice.driverSourceId);
        }
    }
    m_voices.clear();
    m_sourceToVoice.clear();

    // 关闭驱动
    if (m_driver) {
        m_driver->Shutdown();
        m_driver.reset();
    }

    m_initialized.store(false);
}

void AudioDevice::Update(float deltaTime) {
    if (!m_initialized.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 更新多普勒效应
    for (auto& [id, voice] : m_voices) {
        if (voice.state == VoiceState::Playing && voice.is3D) {
            // 这里可以添加多普勒效应计算
            // 简化实现：仅更新位置
        }

        // 更新播放位置
        if (voice.state == VoiceState::Playing && !voice.desc.loop) {
            voice.playbackPosition = m_driver->GetPosition(voice.driverSourceId);
        }
    }

    m_stats.activeVoices = static_cast<uint32_t>(m_voices.size());
}

DeviceInfo AudioDevice::GetDeviceInfo() const {
    DeviceInfo info{};
    if (!m_driver) {
        return info;
    }

    auto format = m_driver->GetFormat();
    info.name = m_driver->GetName();
    info.sampleRate = format.sampleRate;
    info.channels = format.channels;
    info.maxVoices = m_driver->GetMaxBuffers();
    info.supports3D = true;  // AudioDevice 总是支持软件 3D

    return info;
}

// ========== 驱动创建 ==========

std::unique_ptr<IAudioDriver> AudioDevice::CreateDriver(AudioDeviceType deviceType) {
    // Native 模式：使用平台原生驱动
    #if defined(_WIN32) && defined(PRISMA_ENABLE_AUDIO_XAUDIO2)
        if (deviceType == AudioDeviceType::Auto || deviceType == AudioDeviceType::XAudio2) {
            return CreateXAudio2Driver();
        }
    #endif

    #if defined(__ANDROID__) && defined(PRISMA_ENABLE_AUDIO_AAUDIO)
        if (deviceType == AudioDeviceType::Auto || deviceType == AudioDeviceType::SDL3) {
            return CreateAAudioDriver();
        }
    #endif

    // 跨平台模式：使用 SDL3（暂未实现）
    // if (deviceType == AudioDeviceType::SDL3) {
    //     return CreateSDL3Driver();
    // }

    return nullptr;
}

// ========== 播放控制 ==========

AudioVoiceId AudioDevice::PlayClip(const AudioClip& clip, const PlayDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_driver || !clip.IsValid()) {
        return INVALID_VOICE_ID;
    }

    // 创建驱动源
    IAudioDriver::SourceId sourceId = m_driver->CreateSource();
    if (sourceId == IAudioDriver::InvalidSource) {
        return INVALID_VOICE_ID;
    }

    // 创建 Voice
    AudioVoiceId voiceId = GenerateVoiceId();
    Voice voice;
    voice.driverSourceId = sourceId;
    voice.clip = clip;
    voice.desc = desc;
    voice.state = VoiceState::Stopped;
    voice.is3D = desc.is3D;
    voice.spatial3D = desc.spatial;

    // 准备音频缓冲区
    AudioBuffer buffer;
    buffer.data = clip.data.data();
    buffer.size = clip.data.size();
    buffer.frames = clip.GetFrameCount();
    buffer.format = AudioFormat(clip.format.sampleRate, clip.format.channels, clip.format.bitsPerSample);

    if (!m_driver->QueueBuffer(sourceId, buffer)) {
        m_driver->DestroySource(sourceId);
        return INVALID_VOICE_ID;
    }

    // 计算 3D 音频音量
    float volume = desc.volume;
    if (desc.is3D) {
        volume *= Calculate3DVolume(desc.spatial);
    }

    m_driver->SetVolume(sourceId, volume);

    // 开始播放
    if (m_driver->Play(sourceId, desc.loop)) {
        voice.state = VoiceState::Playing;
        m_voices[voiceId] = std::move(voice);
        m_sourceToVoice[sourceId] = voiceId;
        m_playingCount.fetch_add(1);

        TriggerEvent(AudioEventType::VoiceStarted, voiceId);
        return voiceId;
    }

    // 播放失败，清理
    m_driver->DestroySource(sourceId);
    return INVALID_VOICE_ID;
}

void AudioDevice::Stop(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->state == VoiceState::Stopped) {
        return;
    }

    m_driver->Stop(voice->driverSourceId);
    voice->state = VoiceState::Stopped;
    m_playingCount.fetch_sub(1);

    TriggerEvent(AudioEventType::VoiceStopped, voiceId);
}

void AudioDevice::Pause(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->state != VoiceState::Playing) {
        return;
    }

    m_driver->Pause(voice->driverSourceId);
    voice->state = VoiceState::Paused;

    TriggerEvent(AudioEventType::VoicePaused, voiceId);
}

void AudioDevice::Resume(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->state != VoiceState::Paused) {
        return;
    }

    m_driver->Resume(voice->driverSourceId);
    voice->state = VoiceState::Playing;

    TriggerEvent(AudioEventType::VoiceResumed, voiceId);
}

void AudioDevice::StopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, voice] : m_voices) {
        if (voice.state == VoiceState::Playing || voice.state == VoiceState::Paused) {
            m_driver->Stop(voice.driverSourceId);
            voice.state = VoiceState::Stopped;
            TriggerEvent(AudioEventType::VoiceStopped, id);
        }
    }
    m_playingCount.store(0);
}

void AudioDevice::PauseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, voice] : m_voices) {
        if (voice.state == VoiceState::Playing) {
            m_driver->Pause(voice.driverSourceId);
            voice.state = VoiceState::Paused;
            TriggerEvent(AudioEventType::VoicePaused, id);
        }
    }
    m_playingCount.store(0);
}

void AudioDevice::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [id, voice] : m_voices) {
        if (voice.state == VoiceState::Paused) {
            m_driver->Resume(voice.driverSourceId);
            voice.state = VoiceState::Playing;
            TriggerEvent(AudioEventType::VoiceResumed, id);
        }
    }
}

// ========== 实时控制 ==========

void AudioDevice::SetVolume(AudioVoiceId voiceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->desc.volume = std::clamp(volume, 0.0f, 1.0f);

    // 应用 3D 衰减
    float finalVolume = voice->desc.volume;
    if (voice->is3D) {
        finalVolume *= Calculate3DVolume(voice->spatial3D);
    }

    m_driver->SetVolume(voice->driverSourceId, finalVolume);
}

void AudioDevice::SetPitch(AudioVoiceId voiceId, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->desc.pitch = std::clamp(pitch, 0.5f, 2.0f);
    // Note: 音调实现需要驱动支持或软件重采样
}

void AudioDevice::SetPlaybackPosition(AudioVoiceId voiceId, float time) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    m_driver->SetPosition(voice->driverSourceId, time);
    voice->playbackPosition = time;
}

float AudioDevice::GetPlaybackPosition(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return 0.0f;
    }

    return m_driver->GetPosition(voice->driverSourceId);
}

float AudioDevice::GetDuration(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return 0.0f;
    }

    return voice->clip.duration;
}

// ========== 3D 音频 ==========

void AudioDevice::SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) {
    float pos[3] = {x, y, z};
    SetVoice3DPosition(voiceId, pos);
}

void AudioDevice::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    // 更新位置
    voice->spatial3D.position[0] = position[0];
    voice->spatial3D.position[1] = position[1];
    voice->spatial3D.position[2] = position[2];

    // 重新计算音量
    float volume = voice->desc.volume * Calculate3DVolume(voice->spatial3D);
    m_driver->SetVolume(voice->driverSourceId, volume);
}

void AudioDevice::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->spatial3D.velocity[0] = velocity[0];
    voice->spatial3D.velocity[1] = velocity[1];
    voice->spatial3D.velocity[2] = velocity[2];
}

void AudioDevice::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->spatial3D.direction[0] = direction[0];
    voice->spatial3D.direction[1] = direction[1];
    voice->spatial3D.direction[2] = direction[2];
}

void AudioDevice::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->spatial3D = attributes;

    // 重新计算音量
    float volume = voice->desc.volume * Calculate3DVolume(attributes);
    m_driver->SetVolume(voice->driverSourceId, volume);
}

void AudioDevice::SetListener(const AudioListener& listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listener = listener;

    // 更新所有 3D 音源的音量
    for (auto& [id, voice] : m_voices) {
        if (voice.is3D && voice.state == VoiceState::Playing) {
            float volume = voice.desc.volume * Calculate3DVolume(voice.spatial3D);
            m_driver->SetVolume(voice.driverSourceId, volume);
        }
    }
}

void AudioDevice::SetDistanceModel(DistanceModel model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_distanceModel = model;
}

void AudioDevice::SetDopplerFactor(float factor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dopplerFactor = factor;
}

void AudioDevice::SetSpeedOfSound(float speed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_speedOfSound = speed;
}

// ========== 3D 音频计算 ==========

float AudioDevice::Calculate3DVolume(const Audio3DAttributes& spatial) {
    // 计算距离
    float dx = spatial.position[0] - m_listener.position[0];
    float dy = spatial.position[1] - m_listener.position[1];
    float dz = spatial.position[2] - m_listener.position[2];
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 距离衰减
    float attenuation = 1.0f;

    switch (m_distanceModel) {
        case DistanceModel::None:
            attenuation = 1.0f;
            break;

        case DistanceModel::Inverse:
        case DistanceModel::InverseClamped:
            attenuation = spatial.minDistance / (spatial.minDistance + spatial.maxDistance * (distance - spatial.minDistance));
            break;

        case DistanceModel::Linear:
        case DistanceModel::LinearClamped:
            attenuation = 1.0f - (distance - spatial.minDistance) / (spatial.maxDistance - spatial.minDistance);
            break;

        case DistanceModel::Exponential:
        case DistanceModel::ExponentialClamped:
            attenuation = std::pow(distance / spatial.minDistance, -spatial.rolloffFactor);
            break;
    }

    // 限制范围
    if (m_distanceModel == DistanceModel::InverseClamped ||
        m_distanceModel == DistanceModel::LinearClamped ||
        m_distanceModel == DistanceModel::ExponentialClamped) {
        attenuation = std::clamp(attenuation, 0.0f, 1.0f);
    }

    // 距离限制
    if (distance < spatial.minDistance) {
        attenuation = 1.0f;
    } else if (distance > spatial.maxDistance) {
        attenuation = 0.0f;
    }

    return attenuation;
}

void AudioDevice::CalculatePan(const float position[3], float& left, float& right) {
    // 简化的立体声平移计算
    float dx = position[0] - m_listener.position[0];
    float dz = position[2] - m_listener.position[2];

    // 计算角度
    float angle = std::atan2(dx, dz);

    // 基于角度的平移 (-1 到 +1)
    float pan = std::clamp(angle / (3.14159f / 4), -1.0f, 1.0f);

    left = 1.0f - pan;
    right = 1.0f + pan;

    // 归一化
    float maxVol = std::max(left, right);
    if (maxVol > 0.0f) {
        left /= maxVol;
        right /= maxVol;
    }
}

// ========== 全局控制 ==========

void AudioDevice::SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    m_driver->SetMasterVolume(m_masterVolume);
}

float AudioDevice::GetMasterVolume() const {
    return m_masterVolume;
}

// ========== 查询 ==========

bool AudioDevice::IsPlaying(AudioVoiceId voiceId) {
    return GetVoiceState(voiceId) == VoiceState::Playing;
}

bool AudioDevice::IsPaused(AudioVoiceId voiceId) {
    return GetVoiceState(voiceId) == VoiceState::Paused;
}

bool AudioDevice::IsStopped(AudioVoiceId voiceId) {
    return GetVoiceState(voiceId) == VoiceState::Stopped;
}

VoiceState AudioDevice::GetVoiceState(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return VoiceState::Stopped;
    }

    // 更新状态
    auto driverState = m_driver->GetState(voice->driverSourceId);
    if (driverState == IAudioDriver::SourceState::Stopped) {
        voice->state = VoiceState::Stopped;
    }

    return voice->state;
}

uint32_t AudioDevice::GetPlayingVoiceCount() const {
    return m_playingCount.load();
}

// ========== 事件系统 ==========

void AudioDevice::SetEventCallback(AudioEventCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventCallback = std::move(callback);
}

void AudioDevice::RemoveEventCallback() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventCallback = nullptr;
}

void AudioDevice::TriggerEvent(AudioEventType type, AudioVoiceId voiceId, const std::string& message) {
    if (m_eventCallback) {
        AudioEvent event;
        event.type = type;
        event.voiceId = voiceId;
        event.message = message;
        event.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_eventCallback(event);
    }
}

// ========== 统计 ==========

AudioStats AudioDevice::GetStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.activeVoices = static_cast<uint32_t>(m_voices.size());
    return m_stats;
}

void AudioDevice::ResetStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats = AudioStats{};
}

// ========== 调试 ==========

std::string AudioDevice::GenerateDebugReport() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string report = "=== AudioDevice Debug Report ===\n";
    report += "Driver: ";
    report += m_driver ? m_driver->GetName() : "None";
    report += "\n";
    report += "Initialized: ";
    report += m_initialized.load() ? "Yes" : "No";
    report += "\n";
    report += "Active Voices: " + std::to_string(m_voices.size()) + "\n";
    report += "Master Volume: " + std::to_string(m_masterVolume) + "\n";
    report += "==============================\n";

    return report;
}

// ========== Voice 管理 ==========

AudioVoiceId AudioDevice::GenerateVoiceId() {
    AudioVoiceId id;
    do {
        id = m_nextVoiceId.fetch_add(1);
        if (id == INVALID_VOICE_ID) {
            id = m_nextVoiceId.fetch_add(1);
        }
    } while (m_voices.find(id) != m_voices.end());
    return id;
}

AudioDevice::Voice* AudioDevice::FindVoice(AudioVoiceId voiceId) {
    auto it = m_voices.find(voiceId);
    return (it != m_voices.end()) ? &it->second : nullptr;
}

const AudioDevice::Voice* AudioDevice::FindVoice(AudioVoiceId voiceId) const {
    auto it = m_voices.find(voiceId);
    return (it != m_voices.end()) ? &it->second : nullptr;
}

void AudioDevice::OnBufferEnd(IAudioDriver::SourceId sourceId, void* userData) {
    auto* device = static_cast<AudioDevice*>(userData);
    if (!device) {
        return;
    }

    std::lock_guard<std::mutex> lock(device->m_mutex);

    // 查找对应的 Voice
    auto it = device->m_sourceToVoice.find(sourceId);
    if (it == device->m_sourceToVoice.end()) {
        return;
    }

    AudioVoiceId voiceId = it->second;
    Voice* voice = device->FindVoice(voiceId);
    if (!voice) {
        return;
    }

    // 处理循环
    if (voice->desc.loop && voice->state == VoiceState::Playing) {
        device->TriggerEvent(AudioEventType::VoiceLooped, voiceId);
    } else {
        voice->state = VoiceState::Stopped;
        device->m_playingCount.fetch_sub(1);
        device->TriggerEvent(AudioEventType::VoiceStopped, voiceId);
    }
}

} // namespace PrismaEngine::Audio
