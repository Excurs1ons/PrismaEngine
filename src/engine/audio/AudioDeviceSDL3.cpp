#include "AudioDeviceSDL3.h"
#include "../Logger.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <chrono>

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
    m_desc = desc;

    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("Audio", "SDL音频初始化失败: {0}", SDL_GetError());
        return false;
    }

    SDL_AudioSpec spec;
    spec.freq = desc.outputFormat.sampleRate != 0 ? (int)desc.outputFormat.sampleRate : 44100;
    spec.channels = desc.outputFormat.channels != 0 ? (int)desc.outputFormat.channels : 2;
    spec.format = SDL_AUDIO_F32;

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

    StopAll();

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
    UpdateVoiceStates();
}

IAudioDevice::DeviceInfo AudioDeviceSDL3::GetDeviceInfo() const {
    DeviceInfo info;
    info.name = "SDL3 Audio Device";
    info.driver = "SDL3";
    info.isDefault = true;
    return info;
}

std::vector<IAudioDevice::DeviceInfo> AudioDeviceSDL3::GetAvailableDevices() const {
    return { GetDeviceInfo() };
}

bool AudioDeviceSDL3::SetDevice(const std::string& deviceName) {
    return false;
}

AudioVoiceId AudioDeviceSDL3::Play(const AudioClip& clip, const PlayDesc& desc) {
    if (!m_initialized || !clip.IsValid()) {
        return INVALID_VOICE_ID;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    AudioVoiceId voiceId = m_nextVoiceId.fetch_add(1);

    SDL_AudioSpec inputSpec;
    inputSpec.freq = (int)clip.format.sampleRate;
    inputSpec.channels = (int)clip.format.channels;
    inputSpec.format = clip.format.bitsPerSample == 32 ? SDL_AUDIO_F32 : SDL_AUDIO_S16;

    SDL_AudioStream* stream = SDL_CreateAudioStream(&inputSpec, nullptr);
    if (!stream) return INVALID_VOICE_ID;

    SDL_BindAudioStream(m_deviceId, stream);

    if (SDL_PutAudioStreamData(stream, clip.data.data(), (int)clip.data.size()) != 0) {
        SDL_DestroyAudioStream(stream);
        return INVALID_VOICE_ID;
    }

    PlayingVoice voice;
    voice.id = voiceId;
    voice.clip = &clip;
    voice.stream = stream;
    voice.state = VoiceState::Playing;
    voice.volume = desc.volume;
    voice.pitch = desc.pitch;
    voice.looping = desc.loop;
    voice.isActive = true;

    m_playingVoices[voiceId] = voice;
    return voiceId;
}

AudioVoiceId AudioDeviceSDL3::PlayClip(const AudioClip& clip, const PlayDesc& desc) {
    return Play(clip, desc);
}

void AudioDeviceSDL3::Stop(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    RemoveVoice(voiceId);
}

void AudioDeviceSDL3::Pause(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        SDL_PauseAudioStreamDevice(it->second.stream);
        it->second.state = VoiceState::Paused;
    }
}

void AudioDeviceSDL3::Resume(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        SDL_ResumeAudioStreamDevice(it->second.stream);
        it->second.state = VoiceState::Playing;
    }
}

void AudioDeviceSDL3::StopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, voice] : m_playingVoices) {
        SDL_DestroyAudioStream(voice.stream);
    }
    m_playingVoices.clear();
}

void AudioDeviceSDL3::PauseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, voice] : m_playingVoices) {
        SDL_PauseAudioStreamDevice(voice.stream);
        voice.state = VoiceState::Paused;
    }
}

void AudioDeviceSDL3::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, voice] : m_playingVoices) {
        SDL_ResumeAudioStreamDevice(voice.stream);
        voice.state = VoiceState::Playing;
    }
}

void AudioDeviceSDL3::SetVolume(AudioVoiceId voiceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        it->second.volume = volume;
        SDL_SetAudioStreamGain(it->second.stream, volume * m_masterVolume);
    }
}

void AudioDeviceSDL3::SetPitch(AudioVoiceId voiceId, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        it->second.pitch = pitch;
        SDL_SetAudioStreamFrequencyRatio(it->second.stream, pitch);
    }
}

void AudioDeviceSDL3::SetPlaybackPosition(AudioVoiceId voiceId, float time) {}

void AudioDeviceSDL3::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {}
void AudioDeviceSDL3::SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) {}
void AudioDeviceSDL3::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {}
void AudioDeviceSDL3::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {}
void AudioDeviceSDL3::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {}
void AudioDeviceSDL3::SetListener(const AudioListener& listener) {
    m_listener = listener;
}

void AudioDeviceSDL3::SetDistanceModel(DistanceModel model) {}
void AudioDeviceSDL3::SetDopplerFactor(float factor) {}
void AudioDeviceSDL3::SetSpeedOfSound(float speed) {}

void AudioDeviceSDL3::SetMasterVolume(float volume) {
    m_masterVolume = volume;
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

float AudioDeviceSDL3::GetPlaybackPosition(AudioVoiceId voiceId) { return 0.0f; }
float AudioDeviceSDL3::GetDuration(AudioVoiceId voiceId) { return 0.0f; }
VoiceState AudioDeviceSDL3::GetVoiceState(AudioVoiceId voiceId) { return VoiceState::Stopped; }
uint32_t AudioDeviceSDL3::GetPlayingVoiceCount() const { return (uint32_t)m_playingVoices.size(); }

void AudioDeviceSDL3::SetEventCallback(AudioEventCallback callback) { m_eventCallback = callback; }
void AudioDeviceSDL3::RemoveEventCallback() { m_eventCallback = nullptr; }

AudioStats AudioDeviceSDL3::GetStats() const { return m_stats; }
void AudioDeviceSDL3::ResetStats() { m_stats = {}; }

void AudioDeviceSDL3::BeginProfile() {}
std::string AudioDeviceSDL3::EndProfile() { return ""; }

std::string AudioDeviceSDL3::GenerateDebugReport() {
    return "SDL3 Audio Device";
}

void AudioDeviceSDL3::RemoveVoice(AudioVoiceId voiceId) {
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        SDL_DestroyAudioStream(it->second.stream);
        m_playingVoices.erase(it);
    }
}

void AudioDeviceSDL3::UpdateVoiceStates() {
    std::vector<AudioVoiceId> toRemove;
    for (auto& [id, voice] : m_playingVoices) {
        if (SDL_GetAudioStreamQueued(voice.stream) == 0) {
            if (voice.looping) {
                ResetStreamPosition(voice);
            } else {
                toRemove.push_back(id);
            }
        }
    }
    for (auto id : toRemove) RemoveVoice(id);
}

void AudioDeviceSDL3::TriggerEvent(AudioEventType type, AudioVoiceId voiceId) {}

void AudioDeviceSDL3::ResetStreamPosition(PlayingVoice& voice) {
    SDL_ClearAudioStream(voice.stream);
    SDL_PutAudioStreamData(voice.stream, voice.clip->data.data(), (int)voice.clip->data.size());
}

} // namespace Prisma::Audio
