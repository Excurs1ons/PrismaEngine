#include "AudioDeviceSDL3.h"
#include "../logger/Logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace Prisma::Audio {

// ========== AudioDeviceSDL3 实现 ==========

AudioDeviceSDL3::AudioDeviceSDL3() {
    ResetStats();
}

AudioDeviceSDL3::~AudioDeviceSDL3() {
    Shutdown();
}

bool AudioDeviceSDL3::Initialize(const AudioDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        return true;
    }

    LOG_INFO("Audio", "初始化SDL3音频设备");
    m_desc          = desc;
    m_distanceModel = desc.distanceModel;
    m_dopplerFactor = desc.dopplerFactor;
    m_speedOfSound  = desc.speedOfSound;
    m_listener      = AudioListener{};
    ResetStats();

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        LOG_ERROR("Audio", "SDL音频子系统初始化失败: {0}", SDL_GetError());
        return false;
    }

    SDL_AudioSpec spec;
    spec.freq     = desc.outputFormat.sampleRate != 0 ? (int)desc.outputFormat.sampleRate : 44100;
    spec.channels = desc.outputFormat.channels != 0 ? (int)desc.outputFormat.channels : 2;
    spec.format   = SDL_AUDIO_F32;

    m_deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (m_deviceId == 0) {
        LOG_ERROR("Audio", "无法打开SDL音频设备: {0}", SDL_GetError());
        return false;
    }

    SDL_ResumeAudioDevice(m_deviceId);
    m_initialized = true;

    return true;
}

void AudioDeviceSDL3::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    for (auto& [id, voice] : m_playingVoices) {
        if (voice.stream) {
            SDL_DestroyAudioStream(voice.stream);
        }
    }
    m_playingVoices.clear();
    m_stats.activeVoices = 0;

    if (m_deviceId != 0) {
        SDL_CloseAudioDevice(m_deviceId);
        m_deviceId = 0;
    }

    m_initialized = false;
}

bool AudioDeviceSDL3::IsInitialized() const {
    return m_initialized;
}

void AudioDeviceSDL3::Update(Prisma::Timestep ts) {
    if (!m_initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.averageLatency = std::max(0.0f, static_cast<float>(ts) * 1000.0f);
    ++m_framesProcessed;
    UpdateVoiceStates();
    m_stats.activeVoices = static_cast<uint32_t>(m_playingVoices.size());
    m_stats.memoryUsage  = 0;
    for (const auto& [id, voice] : m_playingVoices) {
        if (voice.clip) {
            m_stats.memoryUsage += voice.clip->data.size();
        }
    }
}

IAudioDevice::DeviceInfo AudioDeviceSDL3::GetDeviceInfo() const {
    DeviceInfo info;
    info.name       = m_desc.deviceName.empty() ? "Default Playback Device" : m_desc.deviceName;
    info.driver     = "SDL3";
    info.version    = SDL_GetRevision();
    info.isDefault  = true;
    info.maxVoices  = m_desc.maxVoices;
    info.supports3D = true;
    return info;
}

std::vector<IAudioDevice::DeviceInfo> AudioDeviceSDL3::GetAvailableDevices() const {
    return {GetDeviceInfo()};
}

bool AudioDeviceSDL3::SetDevice(const std::string& deviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (deviceName.empty() || deviceName == m_desc.deviceName || deviceName == "Default Playback Device") {
        m_desc.deviceName = deviceName;
        return true;
    }

    LOG_WARNING(
        "Audio", "SDL3 backend currently supports only the default playback device, requested: {0}", deviceName);
    return false;
}

AudioVoiceId AudioDeviceSDL3::Play(const AudioClip& clip, const PlayDesc& desc) {
    if (!m_initialized || !clip.IsValid()) {
        return INVALID_VOICE_ID;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    AudioVoiceId voiceId = m_nextVoiceId.fetch_add(1);

    SDL_AudioSpec inputSpec;
    inputSpec.freq     = (int)clip.format.sampleRate;
    inputSpec.channels = (int)clip.format.channels;
    inputSpec.format   = clip.format.bitsPerSample == 32 ? SDL_AUDIO_F32 : SDL_AUDIO_S16;

    SDL_AudioStream* stream = SDL_CreateAudioStream(&inputSpec, nullptr);
    if (!stream)
        return INVALID_VOICE_ID;

    SDL_BindAudioStream(m_deviceId, stream);

    if (SDL_PutAudioStreamData(stream, clip.data.data(), (int)clip.data.size()) != 0) {
        SDL_DestroyAudioStream(stream);
        return INVALID_VOICE_ID;
    }

    PlayingVoice voice;
    voice.id       = voiceId;
    voice.clip     = &clip;
    voice.stream   = stream;
    voice.state    = VoiceState::Playing;
    voice.volume   = desc.volume;
    voice.pitch    = desc.pitch;
    voice.looping  = desc.loop;
    voice.isActive = true;
    voice.spatial  = desc.spatial;
    std::memcpy(voice.position, desc.spatial.position, sizeof(voice.position));
    std::memcpy(voice.velocity, desc.spatial.velocity, sizeof(voice.velocity));
    std::memcpy(voice.direction, desc.spatial.direction, sizeof(voice.direction));
    voice.spec = inputSpec;

    const size_t bytesPerFrame =
        static_cast<size_t>(clip.format.channels) * static_cast<size_t>(clip.format.bitsPerSample / 8);
    const size_t totalFrames = clip.GetFrameCount();
    const size_t startFrame =
        std::min(totalFrames, static_cast<size_t>(std::max(0.0f, desc.startTime) * clip.format.sampleRate));
    size_t endFrame = totalFrames;
    if (desc.endTime > 0.0f) {
        endFrame = std::min(endFrame, static_cast<size_t>(desc.endTime * clip.format.sampleRate));
    }
    if (endFrame < startFrame) {
        endFrame = startFrame;
    }
    voice.clipStartOffset = startFrame * bytesPerFrame;
    voice.clipEndOffset   = endFrame * bytesPerFrame;
    voice.duration = static_cast<float>(voice.clipEndOffset - voice.clipStartOffset) / ComputeBytesPerSecond(clip);

    SDL_ClearAudioStream(stream);
    const size_t playSize = voice.clipEndOffset - voice.clipStartOffset;
    if (playSize > 0 &&
        SDL_PutAudioStreamData(stream, clip.data.data() + voice.clipStartOffset, static_cast<int>(playSize)) != 0) {
        SDL_DestroyAudioStream(stream);
        return INVALID_VOICE_ID;
    }

    ApplyVoiceSettings(voice);

    m_playingVoices[voiceId] = voice;
    m_stats.activeVoices     = static_cast<uint32_t>(m_playingVoices.size());
    ++m_stats.totalVoicesCreated;
    m_stats.maxConcurrentVoices = std::max(m_stats.maxConcurrentVoices, m_stats.activeVoices);
    TriggerEvent(AudioEventType::VoiceStarted, voiceId);
    return voiceId;
}

AudioVoiceId AudioDeviceSDL3::PlayClip(const AudioClip& clip, const PlayDesc& desc) {
    return Play(clip, desc);
}

void AudioDeviceSDL3::Stop(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    TriggerEvent(AudioEventType::VoiceStopped, voiceId);
    RemoveVoice(voiceId);
}

void AudioDeviceSDL3::Pause(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        SDL_PauseAudioStreamDevice(it->second.stream);
        it->second.state = VoiceState::Paused;
        TriggerEvent(AudioEventType::VoicePaused, voiceId);
    }
}

void AudioDeviceSDL3::Resume(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        SDL_ResumeAudioStreamDevice(it->second.stream);
        it->second.state = VoiceState::Playing;
        TriggerEvent(AudioEventType::VoiceResumed, voiceId);
    }
}

void AudioDeviceSDL3::StopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, voice] : m_playingVoices) {
        TriggerEvent(AudioEventType::VoiceStopped, id);
        if (voice.stream) {
            SDL_DestroyAudioStream(voice.stream);
        }
    }
    m_playingVoices.clear();
    m_stats.activeVoices = 0;
}

void AudioDeviceSDL3::PauseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, voice] : m_playingVoices) {
        SDL_PauseAudioStreamDevice(voice.stream);
        voice.state = VoiceState::Paused;
        TriggerEvent(AudioEventType::VoicePaused, id);
    }
}

void AudioDeviceSDL3::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, voice] : m_playingVoices) {
        SDL_ResumeAudioStreamDevice(voice.stream);
        voice.state = VoiceState::Playing;
        TriggerEvent(AudioEventType::VoiceResumed, id);
    }
}

void AudioDeviceSDL3::SetVolume(AudioVoiceId voiceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        it->second.volume = volume;
        ApplyVoiceSettings(it->second);
    }
}

void AudioDeviceSDL3::SetPitch(AudioVoiceId voiceId, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        it->second.pitch = pitch;
        ApplyVoiceSettings(it->second);
    }
}

void AudioDeviceSDL3::SetPlaybackPosition(AudioVoiceId voiceId, float time) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end() || !it->second.clip) {
        return;
    }

    PlayingVoice& voice        = it->second;
    const float bytesPerSecond = ComputeBytesPerSecond(*voice.clip);
    const size_t offsetBytes   = static_cast<size_t>(std::clamp(time, 0.0f, voice.duration) * bytesPerSecond);
    const size_t clampedStart  = std::min(voice.clipStartOffset + offsetBytes, voice.clipEndOffset);

    SDL_ClearAudioStream(voice.stream);
    if (clampedStart < voice.clipEndOffset) {
        SDL_PutAudioStreamData(
            voice.stream, voice.clip->data.data() + clampedStart, static_cast<int>(voice.clipEndOffset - clampedStart));
    }

    voice.playbackPosition = static_cast<float>(clampedStart - voice.clipStartOffset) / bytesPerSecond;
    ApplyVoiceSettings(voice);
}

void AudioDeviceSDL3::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    if (!position) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end()) {
        return;
    }

    std::memcpy(it->second.position, position, sizeof(it->second.position));
    std::memcpy(it->second.spatial.position, position, sizeof(it->second.spatial.position));
    ApplyVoiceSettings(it->second);
}

void AudioDeviceSDL3::SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) {
    const float position[3] = {x, y, z};
    SetVoice3DPosition(voiceId, position);
}

void AudioDeviceSDL3::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    if (!velocity) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end()) {
        return;
    }

    std::memcpy(it->second.velocity, velocity, sizeof(it->second.velocity));
    std::memcpy(it->second.spatial.velocity, velocity, sizeof(it->second.spatial.velocity));
}

void AudioDeviceSDL3::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    if (!direction) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end()) {
        return;
    }

    std::memcpy(it->second.direction, direction, sizeof(it->second.direction));
    std::memcpy(it->second.spatial.direction, direction, sizeof(it->second.spatial.direction));
    ApplyVoiceSettings(it->second);
}

void AudioDeviceSDL3::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end()) {
        return;
    }

    it->second.spatial = attributes;
    std::memcpy(it->second.position, attributes.position, sizeof(it->second.position));
    std::memcpy(it->second.velocity, attributes.velocity, sizeof(it->second.velocity));
    std::memcpy(it->second.direction, attributes.direction, sizeof(it->second.direction));
    ApplyVoiceSettings(it->second);
}
void AudioDeviceSDL3::SetListener(const AudioListener& listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listener = listener;
    for (auto& [id, voice] : m_playingVoices) {
        ApplyVoiceSettings(voice);
    }
}

void AudioDeviceSDL3::SetDistanceModel(DistanceModel model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_distanceModel = model;
    for (auto& [id, voice] : m_playingVoices) {
        ApplyVoiceSettings(voice);
    }
}

void AudioDeviceSDL3::SetDopplerFactor(float factor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dopplerFactor = std::max(0.0f, factor);
}

void AudioDeviceSDL3::SetSpeedOfSound(float speed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_speedOfSound = std::max(1.0f, speed);
}

void AudioDeviceSDL3::SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_masterVolume = volume;
    for (auto& [id, voice] : m_playingVoices) {
        ApplyVoiceSettings(voice);
    }
}

float AudioDeviceSDL3::GetMasterVolume() const {
    return m_masterVolume;
}

bool AudioDeviceSDL3::IsPlaying(AudioVoiceId voiceId) {
    auto it = m_playingVoices.find(voiceId);
    return it != m_playingVoices.end() && it->second.state == VoiceState::Playing;
}

bool AudioDeviceSDL3::IsPaused(AudioVoiceId voiceId) {
    auto it = m_playingVoices.find(voiceId);
    return it != m_playingVoices.end() && it->second.state == VoiceState::Paused;
}

bool AudioDeviceSDL3::IsStopped(AudioVoiceId voiceId) {
    return m_playingVoices.find(voiceId) == m_playingVoices.end();
}

float AudioDeviceSDL3::GetPlaybackPosition(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end() || !it->second.clip) {
        return -1.0f;
    }

    const float bytesPerSecond  = ComputeBytesPerSecond(*it->second.clip);
    const float queuedSeconds   = static_cast<float>(SDL_GetAudioStreamQueued(it->second.stream)) / bytesPerSecond;
    const float position        = std::max(0.0f, it->second.duration - queuedSeconds);
    it->second.playbackPosition = std::clamp(position, 0.0f, it->second.duration);
    return it->second.playbackPosition;
}

float AudioDeviceSDL3::GetDuration(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    return it != m_playingVoices.end() ? it->second.duration : -1.0f;
}

VoiceState AudioDeviceSDL3::GetVoiceState(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    return it != m_playingVoices.end() ? it->second.state : VoiceState::Stopped;
}
uint32_t AudioDeviceSDL3::GetPlayingVoiceCount() const {
    return (uint32_t)m_playingVoices.size();
}

void AudioDeviceSDL3::SetEventCallback(AudioEventCallback callback) {
    m_eventCallback = callback;
}
void AudioDeviceSDL3::RemoveEventCallback() {
    m_eventCallback = nullptr;
}

AudioStats AudioDeviceSDL3::GetStats() const {
    return m_stats;
}
void AudioDeviceSDL3::ResetStats() {
    m_stats              = {};
    m_stats.activeVoices = static_cast<uint32_t>(m_playingVoices.size());
}

void AudioDeviceSDL3::BeginProfile() {
    m_profileStart  = std::chrono::steady_clock::now();
    m_profileActive = true;
}

std::string AudioDeviceSDL3::EndProfile() {
    if (!m_profileActive) {
        return "Audio profile inactive";
    }

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_profileStart)
            .count();
    m_profileActive = false;
    return "Audio profile duration: " + std::to_string(elapsed) + " ms";
}

std::string AudioDeviceSDL3::GenerateDebugReport() {
    return "SDL3 Audio Device";
}

void AudioDeviceSDL3::RemoveVoice(AudioVoiceId voiceId) {
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        SDL_DestroyAudioStream(it->second.stream);
        m_playingVoices.erase(it);
        m_stats.activeVoices = static_cast<uint32_t>(m_playingVoices.size());
    }
}

void AudioDeviceSDL3::UpdateVoiceStates() {
    std::vector<AudioVoiceId> toRemove;
    for (auto& [id, voice] : m_playingVoices) {
        if (voice.clip) {
            const float bytesPerSecond = ComputeBytesPerSecond(*voice.clip);
            const float queuedSeconds  = static_cast<float>(SDL_GetAudioStreamQueued(voice.stream)) / bytesPerSecond;
            voice.playbackPosition     = std::clamp(voice.duration - queuedSeconds, 0.0f, voice.duration);
        }

        if (SDL_GetAudioStreamQueued(voice.stream) == 0) {
            if (voice.looping) {
                ResetStreamPosition(voice);
                TriggerEvent(AudioEventType::VoiceLooped, id);
            } else {
                toRemove.push_back(id);
            }
        }
    }
    for (auto id : toRemove) {
        TriggerEvent(AudioEventType::VoiceStopped, id);
        RemoveVoice(id);
    }
}

void AudioDeviceSDL3::TriggerEvent(AudioEventType type, AudioVoiceId voiceId) {
    if (!m_eventCallback) {
        return;
    }

    AudioEvent event;
    event.type      = type;
    event.voiceId   = voiceId;
    event.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
    m_eventCallback(event);
}

void AudioDeviceSDL3::ResetStreamPosition(PlayingVoice& voice) {
    SDL_ClearAudioStream(voice.stream);
    if (voice.clipEndOffset > voice.clipStartOffset) {
        SDL_PutAudioStreamData(voice.stream,
                               voice.clip->data.data() + voice.clipStartOffset,
                               static_cast<int>(voice.clipEndOffset - voice.clipStartOffset));
    }
    voice.playbackPosition = 0.0f;
    ApplyVoiceSettings(voice);
}

void AudioDeviceSDL3::ApplyVoiceSettings(PlayingVoice& voice) {
    SDL_SetAudioStreamGain(voice.stream, voice.volume * m_masterVolume * ComputeVoiceAttenuation(voice));
    SDL_SetAudioStreamFrequencyRatio(voice.stream, std::max(0.01f, voice.pitch));
}

float AudioDeviceSDL3::ComputeVoiceAttenuation(const PlayingVoice& voice) const {
    const float dx          = voice.position[0] - m_listener.position[0];
    const float dy          = voice.position[1] - m_listener.position[1];
    const float dz          = voice.position[2] - m_listener.position[2];
    const float distance    = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float minDistance = std::max(0.001f, voice.spatial.minDistance);
    const float maxDistance = std::max(minDistance, voice.spatial.maxDistance);
    const float rolloff     = std::max(0.0f, voice.spatial.rolloffFactor) * std::max(0.0f, m_dopplerFactor);

    switch (m_distanceModel) {
        case DistanceModel::None:
            return 1.0f;
        case DistanceModel::Inverse:
        case DistanceModel::InverseClamped:
            return minDistance / (minDistance + rolloff * std::max(0.0f, distance - minDistance));
        case DistanceModel::Linear:
        case DistanceModel::LinearClamped:
            if (distance >= maxDistance) {
                return 0.0f;
            }
            return 1.0f - (distance - minDistance) / std::max(0.001f, maxDistance - minDistance);
        case DistanceModel::Exponential:
        case DistanceModel::ExponentialClamped:
            return std::pow(std::max(0.001f, distance / minDistance), -std::max(0.001f, rolloff));
    }

    return 1.0f;
}

float AudioDeviceSDL3::ComputeBytesPerSecond(const AudioClip& clip) {
    const uint32_t bytesPerSample = std::max<uint32_t>(1, clip.format.bitsPerSample / 8);
    return static_cast<float>(std::max<uint32_t>(1, clip.format.sampleRate) *
                              std::max<uint32_t>(1, clip.format.channels) * bytesPerSample);
}

}  // namespace Prisma::Audio
