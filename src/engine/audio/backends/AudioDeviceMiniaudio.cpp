#include "AudioDeviceMiniaudio.h"
#include <Engine/audio/dsp/AudioBuffer.h>
#include <cstring>

namespace Prisma::Audio {

bool AudioDeviceMiniaudio::Initialize(const AudioDesc& desc) {
    if (m_initialized) return true;

    m_desc = desc;
    ma_mutex_init(&m_mutex);

    m_config = ma_device_config_init(ma_device_type_playback);
    m_config.playback.format = ma_format_f32;
    m_config.playback.channels = desc.outputFormat.channels;
    m_config.sampleRate = desc.outputFormat.sampleRate;
    m_config.dataCallback = OnSendAudio;
    m_config.pUserData = this;
    m_config.periodSizeInFrames = desc.bufferSize;

    if (ma_device_init(nullptr, &m_config, &m_device) != MA_SUCCESS) {
        return false;
    }

    m_mixBuffer = std::make_unique<DSP::AudioBuffer>(
        desc.outputFormat.channels, desc.bufferSize);

    if (ma_device_start(&m_device) != MA_SUCCESS) {
        ma_device_uninit(&m_device);
        return false;
    }

    m_initialized = true;
    return true;
}

void AudioDeviceMiniaudio::Shutdown() {
    if (!m_initialized) return;

    ma_device_stop(&m_device);
    ma_device_uninit(&m_device);
    m_voices.clear();
    m_mixBuffer.reset();
    ma_mutex_uninit(&m_mutex);
    m_initialized = false;
}

IAudioDevice::DeviceInfo AudioDeviceMiniaudio::GetDeviceInfo() const {
    DeviceInfo info;
    info.name = "miniaudio";
    info.driver = "miniaudio";
    info.version = "0.11.21";
    info.isDefault = true;
    info.maxVoices = 256;
    info.supports3D = false;
    info.supportsEffects = false;
    return info;
}

AudioDeviceType AudioDeviceMiniaudio::GetDeviceType() const {
    return AudioDeviceType::Null;
}

std::vector<IAudioDevice::DeviceInfo> AudioDeviceMiniaudio::GetAvailableDevices() const {
    std::vector<IAudioDevice::DeviceInfo> devices;
    DeviceInfo info;
    info.name = "Default";
    info.driver = "miniaudio";
    info.isDefault = true;
    devices.push_back(info);
    return devices;
}

void AudioDeviceMiniaudio::Update(Prisma::Timestep ts) {
    // miniaudio handles its own threading; nothing needed here
    (void)ts;
}

void AudioDeviceMiniaudio::OnSendAudio(ma_device* pDevice, void* pOutput,
                                       const void* /*pInput*/, ma_uint32 frameCount) {
    auto* self = static_cast<AudioDeviceMiniaudio*>(pDevice->pUserData);
    if (!self || !self->m_mixBuffer) return;

    float* out = static_cast<float*>(pOutput);
    uint32_t channels = self->m_desc.outputFormat.channels;

    DSP::AudioBuffer mixBuf(channels, frameCount);

    if (self->m_graph) {
        self->m_graph->Process(mixBuf);
    }

    for (uint32_t f = 0; f < frameCount; ++f) {
        for (uint32_t c = 0; c < channels; ++c) {
            out[f * channels + c] = mixBuf.GetChannel(c)[f] * self->m_masterVolume;
        }
    }
}

AudioVoiceId AudioDeviceMiniaudio::Play(const AudioClip& clip, const PlayDesc& desc) {
    return PlayClip(clip, desc);
}

AudioVoiceId AudioDeviceMiniaudio::PlayClip(const AudioClip& clip, const PlayDesc& desc) {
    if (!clip.IsValid()) return INVALID_VOICE_ID;

    AudioVoiceId id = m_nextVoiceId++;
    Voice voice;
    voice.clip = std::make_shared<AudioClip>(clip);
    voice.desc = desc;
    voice.state = VoiceState::Playing;
    voice.readCursor = static_cast<size_t>(desc.startTime * clip.format.sampleRate) * clip.format.GetFrameSize();

    {
        ma_mutex_lock(&m_mutex);
        m_voices[id] = std::move(voice);
        ma_mutex_unlock(&m_mutex);
    }

    FireEvent(AudioEventType::VoiceStarted, id);
    return id;
}

void AudioDeviceMiniaudio::Stop(AudioVoiceId voiceId) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.state = VoiceState::Stopped;
        m_voices.erase(it);
    }
    ma_mutex_unlock(&m_mutex);
    FireEvent(AudioEventType::VoiceStopped, voiceId);
}

void AudioDeviceMiniaudio::Pause(AudioVoiceId voiceId) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.state = VoiceState::Paused;
    }
    ma_mutex_unlock(&m_mutex);
    FireEvent(AudioEventType::VoicePaused, voiceId);
}

void AudioDeviceMiniaudio::PauseAll() {
    ma_mutex_lock(&m_mutex);
    for (auto& [id, voice] : m_voices) {
        if (voice.state == VoiceState::Playing) {
            voice.state = VoiceState::Paused;
        }
    }
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::Resume(AudioVoiceId voiceId) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.state = VoiceState::Playing;
    }
    ma_mutex_unlock(&m_mutex);
    FireEvent(AudioEventType::VoiceResumed, voiceId);
}

void AudioDeviceMiniaudio::ResumeAll() {
    ma_mutex_lock(&m_mutex);
    for (auto& [id, voice] : m_voices) {
        if (voice.state == VoiceState::Paused) {
            voice.state = VoiceState::Playing;
        }
    }
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::StopAll() {
    ma_mutex_lock(&m_mutex);
    m_voices.clear();
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetVolume(AudioVoiceId voiceId, float volume) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) it->second.desc.volume = volume;
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetPitch(AudioVoiceId voiceId, float pitch) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) it->second.desc.pitch = pitch;
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetPlaybackPosition(AudioVoiceId voiceId, float time) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end() && it->second.clip) {
        it->second.readCursor =
            static_cast<size_t>(time * it->second.clip->format.sampleRate) *
            it->second.clip->format.GetFrameSize();
    }
    ma_mutex_unlock(&m_mutex);
}

float AudioDeviceMiniaudio::GetPlaybackPosition(AudioVoiceId voiceId) {
    ma_mutex_lock(&m_mutex);
    float pos = -1.0f;
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end() && it->second.clip &&
        it->second.clip->format.sampleRate > 0) {
        size_t frameSize = it->second.clip->format.GetFrameSize();
        if (frameSize > 0) {
            pos = static_cast<float>(it->second.readCursor / frameSize) /
                  static_cast<float>(it->second.clip->format.sampleRate);
        }
    }
    ma_mutex_unlock(&m_mutex);
    return pos;
}

float AudioDeviceMiniaudio::GetDuration(AudioVoiceId voiceId) {
    ma_mutex_lock(&m_mutex);
    float duration = -1.0f;
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end() && it->second.clip) {
        duration = it->second.clip->duration;
    }
    ma_mutex_unlock(&m_mutex);
    return duration;
}

bool AudioDeviceMiniaudio::IsPlaying(AudioVoiceId voiceId) {
    return GetVoiceState(voiceId) == VoiceState::Playing;
}

bool AudioDeviceMiniaudio::IsPaused(AudioVoiceId voiceId) {
    return GetVoiceState(voiceId) == VoiceState::Paused;
}

bool AudioDeviceMiniaudio::IsStopped(AudioVoiceId voiceId) {
    return GetVoiceState(voiceId) == VoiceState::Stopped;
}

VoiceState AudioDeviceMiniaudio::GetVoiceState(AudioVoiceId voiceId) {
    ma_mutex_lock(&m_mutex);
    VoiceState state = VoiceState::Stopped;
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) state = it->second.state;
    ma_mutex_unlock(&m_mutex);
    return state;
}

uint32_t AudioDeviceMiniaudio::GetPlayingVoiceCount() const {
    ma_mutex_lock(&m_mutex);
    uint32_t count = 0;
    for (const auto& [id, voice] : m_voices) {
        if (voice.state == VoiceState::Playing) ++count;
    }
    ma_mutex_unlock(&m_mutex);
    return count;
}

void AudioDeviceMiniaudio::SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) {
    (void)voiceId; (void)x; (void)y; (void)z;
}

void AudioDeviceMiniaudio::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    (void)voiceId; (void)position;
}

void AudioDeviceMiniaudio::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    (void)voiceId; (void)velocity;
}

void AudioDeviceMiniaudio::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    (void)voiceId; (void)direction;
}

void AudioDeviceMiniaudio::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    (void)voiceId; (void)attributes;
}

void AudioDeviceMiniaudio::SetListener(const AudioListener& listener) {
    (void)listener;
}

void AudioDeviceMiniaudio::SetMasterVolume(float volume) {
    m_masterVolume = volume;
}

void AudioDeviceMiniaudio::SetDistanceModel(DistanceModel model) {
    (void)model;
}

void AudioDeviceMiniaudio::SetDopplerFactor(float factor) {
    (void)factor;
}

void AudioDeviceMiniaudio::SetSpeedOfSound(float speed) {
    (void)speed;
}

void AudioDeviceMiniaudio::SetEventCallback(AudioEventCallback callback) {
    m_eventCallback = callback;
}

void AudioDeviceMiniaudio::RemoveEventCallback() {
    m_eventCallback = nullptr;
}

AudioStats AudioDeviceMiniaudio::GetStats() const {
    AudioStats stats;
    stats.activeVoices = static_cast<uint32_t>(m_voices.size());
    return stats;
}

void AudioDeviceMiniaudio::ResetStats() {
}

std::string AudioDeviceMiniaudio::GenerateDebugReport() {
    return "AudioDeviceMiniaudio Debug Report\n"
           "  Backend: miniaudio\n"
           "  Voices: " + std::to_string(m_voices.size()) + "\n"
           "  Master Volume: " + std::to_string(m_masterVolume) + "\n";
}

void AudioDeviceMiniaudio::FireEvent(AudioEventType type, AudioVoiceId voiceId, const std::string& msg) {
    if (m_eventCallback) {
        AudioEvent ev;
        ev.type = type;
        ev.voiceId = voiceId;
        ev.message = msg;
        ev.timestamp = 0;
        m_eventCallback(ev);
    }
}

} // namespace Prisma::Audio
