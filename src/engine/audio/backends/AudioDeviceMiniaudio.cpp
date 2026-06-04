#define MINIAUDIO_IMPLEMENTATION
#include "AudioDeviceMiniaudio.h"
#include "audio/dsp/AudioBuffer.h"
#include <algorithm>
#include <cmath>
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
    info.supports3D = true;
    info.supportsEffects = true;
    return info;
}

AudioDeviceType AudioDeviceMiniaudio::GetDeviceType() const {
    return AudioDeviceType::Miniaudio;
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
    uint32_t sampleRate = self->m_device.sampleRate;
    if (sampleRate == 0) sampleRate = 48000;

    // 清空输出
    std::memset(out, 0, sizeof(float) * frameCount * channels);

    // 处理 DSP 图（如果存在）直接输出到 out
    if (self->m_graph) {
        DSP::AudioBuffer graphBuf(channels, frameCount);
        self->m_graph->Process(graphBuf);
        for (uint32_t f = 0; f < frameCount; ++f)
            for (uint32_t c = 0; c < channels; ++c)
                out[f * channels + c] += graphBuf.GetChannel(c)[f];
    }

    // 逐 Voice 读取 PCM → 转换 → 应用效果 → 混合
    {
        ma_mutex_lock(&self->m_mutex);

        std::vector<AudioVoiceId> finishedVoices;

        // 每个 Voice 使用独立临时缓冲, 方便应用效果后再混合
        std::vector<float> voiceBuf; // 按需扩容

        for (auto& [id, voice] : self->m_voices) {
            if (voice.state != VoiceState::Playing || !voice.clip) continue;

            const auto& clip = *voice.clip;
            if (clip.data.empty() || clip.format.sampleRate == 0) continue;

            const size_t bytesPerFrame = clip.format.GetFrameSize();
            const uint32_t clipChannels = clip.format.channels;

            // 确保 voiceBuf 足够大 (channels * frames)
            size_t needed = (size_t)channels * frameCount;
            if (voiceBuf.size() < needed) voiceBuf.resize(needed, 0.0f);
            std::fill(voiceBuf.begin(), voiceBuf.begin() + needed, 0.0f);

            // 读取 PCM 数据并转换为 float (planar 格式)
            // voiceBuf[c * frameCount + f] 存储声道 c 的第 f 帧
            bool voiceFinished = false;
            for (uint32_t f = 0; f < frameCount; ++f) {
                // 检查是否到达结尾
                if (voice.readCursor + bytesPerFrame > clip.data.size()) {
                    if (voice.desc.loop) {
                        voice.readCursor = 0;
                    } else {
                        voiceFinished = true;
                        break;
                    }
                }

                const uint8_t* src = clip.data.data() + voice.readCursor;

                if (clip.format.bitsPerSample == 32) {
                    const float* srcF = reinterpret_cast<const float*>(src);
                    for (uint32_t c = 0; c < clipChannels; ++c)
                        voiceBuf[c * frameCount + f] = srcF[c];
                } else if (clip.format.bitsPerSample == 16) {
                    const int16_t* srcI = reinterpret_cast<const int16_t*>(src);
                    for (uint32_t c = 0; c < clipChannels; ++c)
                        voiceBuf[c * frameCount + f] = (float)srcI[c] / 32768.0f;
                } else if (clip.format.bitsPerSample == 8) {
                    for (uint32_t c = 0; c < clipChannels; ++c)
                        voiceBuf[c * frameCount + f] = (float)((int)src[c] - 128) / 128.0f;
                }

                voice.readCursor += bytesPerFrame;
            }

            // 应用音效 (逐声道处理)
            if (!voiceFinished && voice.effectType != EffectType::None && voice.effectParamsSize > 0) {
                for (uint32_t c = 0; c < clipChannels; ++c) {
                    float* chBuf = voiceBuf.data() + c * frameCount;
                    DSP::ProcessEffect(chBuf, frameCount - 0,
                                       clipChannels, sampleRate,
                                       voice.effectType,
                                       voice.effectParams,
                                       voice.effectState);
                }
            }

            // 应用音量和 3D 衰减, 并混合到输出
            float vol = voice.desc.volume;
            for (uint32_t f = 0; f < frameCount; ++f) {
                for (uint32_t c = 0; c < channels; ++c) {
                    uint32_t srcCh = std::min(c, clipChannels - 1);
                    out[f * channels + c] += voiceBuf[srcCh * frameCount + f] * vol;
                }
            }

            if (voiceFinished) {
                finishedVoices.push_back(id);
            }
        }

        for (auto vid : finishedVoices) {
            auto it = self->m_voices.find(vid);
            if (it != self->m_voices.end()) {
                it->second.state = VoiceState::Stopped;
                self->m_voices.erase(it);
            }
        }

        ma_mutex_unlock(&self->m_mutex);
    }

    // 主音量 + 削波保护
    for (uint32_t f = 0; f < frameCount * channels; ++f) {
        out[f] = std::clamp(out[f] * self->m_masterVolume, -1.0f, 1.0f);
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

    // 复制 3D 空间属性
    if (desc.is3D) {
        voice.position[0] = desc.spatial.position[0];
        voice.position[1] = desc.spatial.position[1];
        voice.position[2] = desc.spatial.position[2];
        voice.velocity[0] = desc.spatial.velocity[0];
        voice.velocity[1] = desc.spatial.velocity[1];
        voice.velocity[2] = desc.spatial.velocity[2];
        voice.direction[0] = desc.spatial.direction[0];
        voice.direction[1] = desc.spatial.direction[1];
        voice.direction[2] = desc.spatial.direction[2];
        voice.rolloffFactor = desc.spatial.rolloffFactor;
        voice.coneInnerAngle = desc.spatial.coneInnerAngle;
        voice.coneOuterAngle = desc.spatial.coneOuterAngle;
        voice.coneOuterGain = desc.spatial.coneOuterGain;
    }

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
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.position[0] = x;
        it->second.position[1] = y;
        it->second.position[2] = z;
        it->second.desc.spatial.position[0] = x;
        it->second.desc.spatial.position[1] = y;
        it->second.desc.spatial.position[2] = z;
    }
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    if (!position) return;
    SetVoice3DPosition(voiceId, position[0], position[1], position[2]);
}

void AudioDeviceMiniaudio::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    if (!velocity) return;
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.velocity[0] = velocity[0];
        it->second.velocity[1] = velocity[1];
        it->second.velocity[2] = velocity[2];
        it->second.desc.spatial.velocity[0] = velocity[0];
        it->second.desc.spatial.velocity[1] = velocity[1];
        it->second.desc.spatial.velocity[2] = velocity[2];
    }
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    if (!direction) return;
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.direction[0] = direction[0];
        it->second.direction[1] = direction[1];
        it->second.direction[2] = direction[2];
        it->second.desc.spatial.direction[0] = direction[0];
        it->second.desc.spatial.direction[1] = direction[1];
        it->second.desc.spatial.direction[2] = direction[2];
    }
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.desc.spatial = attributes;
        it->second.position[0] = attributes.position[0];
        it->second.position[1] = attributes.position[1];
        it->second.position[2] = attributes.position[2];
        it->second.velocity[0] = attributes.velocity[0];
        it->second.velocity[1] = attributes.velocity[1];
        it->second.velocity[2] = attributes.velocity[2];
        it->second.direction[0] = attributes.direction[0];
        it->second.direction[1] = attributes.direction[1];
        it->second.direction[2] = attributes.direction[2];
        it->second.rolloffFactor = attributes.rolloffFactor;
        it->second.coneInnerAngle = attributes.coneInnerAngle;
        it->second.coneOuterAngle = attributes.coneOuterAngle;
        it->second.coneOuterGain = attributes.coneOuterGain;
    }
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetListener(const AudioListener& listener) {
    ma_mutex_lock(&m_mutex);
    m_listener = listener;
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetMasterVolume(float volume) {
    m_masterVolume = volume;
}

void AudioDeviceMiniaudio::SetDistanceModel(DistanceModel model) {
    ma_mutex_lock(&m_mutex);
    m_distanceModel = model;
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetDopplerFactor(float factor) {
    ma_mutex_lock(&m_mutex);
    m_dopplerFactor = std::max(0.0f, factor);
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetSpeedOfSound(float speed) {
    ma_mutex_lock(&m_mutex);
    m_speedOfSound = std::max(1.0f, speed);
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetConeAngles(AudioVoiceId voiceId, float innerAngle, float outerAngle) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.coneInnerAngle = innerAngle;
        it->second.coneOuterAngle = outerAngle;
        it->second.desc.spatial.coneInnerAngle = innerAngle;
        it->second.desc.spatial.coneOuterAngle = outerAngle;
    }
    ma_mutex_unlock(&m_mutex);
}

void AudioDeviceMiniaudio::SetRolloffFactor(AudioVoiceId voiceId, float factor) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.rolloffFactor = std::max(0.0f, factor);
        it->second.desc.spatial.rolloffFactor = std::max(0.0f, factor);
    }
    ma_mutex_unlock(&m_mutex);
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
    std::string report = "AudioDeviceMiniaudio Debug Report\n"
                         "  Backend: miniaudio\n"
                         "  Voices: " + std::to_string(m_voices.size()) + "\n"
                         "  Master Volume: " + std::to_string(m_masterVolume) + "\n"
                         "  Distance Model: " + std::to_string(static_cast<int>(m_distanceModel)) + "\n"
                         "  Doppler Factor: " + std::to_string(m_dopplerFactor) + "\n"
                         "  Speed of Sound: " + std::to_string(m_speedOfSound) + "\n"
                         "  Listener: (" + std::to_string(m_listener.position[0]) + ", "
                                        + std::to_string(m_listener.position[1]) + ", "
                                        + std::to_string(m_listener.position[2]) + ")\n";

    ma_mutex_lock(&m_mutex);
    for (const auto& [id, voice] : m_voices) {
        report += "  Voice " + std::to_string(id) + ": "
                  "pos(" + std::to_string(voice.position[0]) + ", "
                         + std::to_string(voice.position[1]) + ", "
                         + std::to_string(voice.position[2]) + ") "
                  "cone(" + std::to_string(voice.coneInnerAngle) + "/"
                          + std::to_string(voice.coneOuterAngle) + ") "
                  "rolloff=" + std::to_string(voice.rolloffFactor) + "\n";
    }
    ma_mutex_unlock(&m_mutex);

    return report;
}

bool AudioDeviceMiniaudio::ApplyEffect(AudioVoiceId voiceId, EffectType type, const void* params) {
    if (type == EffectType::None) {
        RemoveEffects(voiceId);
        return true;
    }

    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it == m_voices.end()) {
        ma_mutex_unlock(&m_mutex);
        return false;
    }

    auto& voice = it->second;
    if (type != voice.effectType) {
        DSP::ResetEffectState(voice.effectState);
    }
    voice.effectType = type;

    // 复制参数到 Voice 的内部存储
    if (params) {
        // EffectParams union is ~104 bytes, fits in 128 byte buffer
        std::memcpy(voice.effectParams, params,
                    std::min(sizeof(voice.effectParams), (size_t)128));
        voice.effectParamsSize = std::min(sizeof(voice.effectParams), (size_t)128);
    } else {
        voice.effectParamsSize = 0;
    }

    ma_mutex_unlock(&m_mutex);
    return true;
}

void AudioDeviceMiniaudio::RemoveEffects(AudioVoiceId voiceId) {
    ma_mutex_lock(&m_mutex);
    auto it = m_voices.find(voiceId);
    if (it != m_voices.end()) {
        it->second.effectType = EffectType::None;
        it->second.effectParamsSize = 0;
        DSP::ResetEffectState(it->second.effectState);
    }
    ma_mutex_unlock(&m_mutex);
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
