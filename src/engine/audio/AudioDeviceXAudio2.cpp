#include "AudioDeviceXAudio2.h"
#include "../Logger.h"

namespace PrismaEngine::Audio {

AudioDeviceXAudio2::AudioDeviceXAudio2() = default;

AudioDeviceXAudio2::~AudioDeviceXAudio2() {
    Shutdown();
}

bool AudioDeviceXAudio2::Initialize(const AudioDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        return true;
    }

    m_desc = desc;
    m_initialized = true;

    LOG_INFO("Audio", "XAudio2 device initialized (minimal implementation)");
    return true;
}

void AudioDeviceXAudio2::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    StopAll();
    ReleaseAll();

    m_initialized = false;
}

bool AudioDeviceXAudio2::IsInitialized() const {
    return m_initialized;
}

void AudioDeviceXAudio2::Update(float deltaTime) {
    (void)deltaTime;
}

DeviceInfo AudioDeviceXAudio2::GetDeviceInfo() const {
    DeviceInfo info;
    info.name = "XAudio2";
    info.description = "Windows XAudio2 Audio Device";
    return info;
}

std::vector<DeviceInfo> AudioDeviceXAudio2::GetAvailableDevices() const {
    return {GetDeviceInfo()};
}

bool AudioDeviceXAudio2::SetDevice(const std::string& deviceName) {
    return deviceName == "XAudio2";
}

AudioDeviceType AudioDeviceXAudio2::GetDeviceType() const {
    return AudioDeviceType::XAudio2;
}

AudioVoiceId AudioDeviceXAudio2::PlayClip(const AudioClip& clip, const PlayDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return INVALID_VOICE_ID;
    }

    Voice* voice = AllocateVoice();
    if (!voice) {
        return INVALID_VOICE_ID;
    }

    AudioVoiceId voiceId = m_nextVoiceId++;
    voice->isActive = true;
    voice->isLooping = desc.loop;
    voice->volume = desc.volume;
    voice->pitch = desc.pitch;
    voice->duration = clip.duration;
    voice->audioData = clip.data;

    m_activeVoices[voiceId] = voice;
    m_stats.activeVoiceCount = static_cast<int>(m_activeVoices.size());

    TriggerEvent(AudioEventType::VoiceStarted, voiceId);

    return voiceId;
}

void AudioDeviceXAudio2::Stop(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_activeVoices.find(voiceId);
    if (it != m_activeVoices.end()) {
        Voice* voice = it->second;
        voice->isActive = false;
        ReleaseVoice(voice);
        m_activeVoices.erase(it);
        m_stats.activeVoiceCount = static_cast<int>(m_activeVoices.size());
    }
}

void AudioDeviceXAudio2::Pause(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_activeVoices.find(voiceId);
    if (it != m_activeVoices.end()) {
        // TODO: 实现暂停
        (void)it->second;
    }
}

void AudioDeviceXAudio2::Resume(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_activeVoices.find(voiceId);
    if (it != m_activeVoices.end()) {
        // TODO: 实现恢复
        (void)it->second;
    }
}

void AudioDeviceXAudio2::SetVolume(AudioVoiceId voiceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_activeVoices.find(voiceId);
    if (it != m_activeVoices.end()) {
        it->second->volume = volume;
    }
}

void AudioDeviceXAudio2::SetPitch(AudioVoiceId voiceId, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_activeVoices.find(voiceId);
    if (it != m_activeVoices.end()) {
        it->second->pitch = pitch;
    }
}

void AudioDeviceXAudio2::SetPlaybackPosition(AudioVoiceId voiceId, float time) {
    (void)voiceId;
    (void)time;
    // TODO: 实现设置播放位置
}

void AudioDeviceXAudio2::SetVoice3DPosition(AudioVoiceId voiceId, float x, float y, float z) {
    (void)voiceId;
    (void)x;
    (void)y;
    (void)z;
    // TODO: 实现 3D 位置
}

void AudioDeviceXAudio2::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    (void)voiceId;
    (void)attributes;
    // TODO: 实现 3D 属性
}

void AudioDeviceXAudio2::SetListener(const AudioListener& listener) {
    m_listener = listener;
}

void AudioDeviceXAudio2::SetDistanceModel(DistanceModel model) {
    (void)model;
    // TODO: 实现距离模型
}

void AudioDeviceXAudio2::SetMasterVolume(float volume) {
    m_masterVolume = volume;
}

float AudioDeviceXAudio2::GetMasterVolume() const {
    return m_masterVolume;
}

bool AudioDeviceXAudio2::IsPlaying(AudioVoiceId voiceId) {
    auto it = m_activeVoices.find(voiceId);
    return it != m_activeVoices.end() && it->second->isActive;
}

bool AudioDeviceXAudio2::IsPaused(AudioVoiceId voiceId) {
    (void)voiceId;
    return false;
}

bool AudioDeviceXAudio2::IsStopped(AudioVoiceId voiceId) {
    auto it = m_activeVoices.find(voiceId);
    return it == m_activeVoices.end() || !it->second->isActive;
}

float AudioDeviceXAudio2::GetPlaybackPosition(AudioVoiceId voiceId) {
    (void)voiceId;
    return 0.0f;
}

float AudioDeviceXAudio2::GetDuration(AudioVoiceId voiceId) {
    auto it = m_activeVoices.find(voiceId);
    if (it != m_activeVoices.end()) {
        return it->second->duration;
    }
    return 0.0f;
}

uint32_t AudioDeviceXAudio2::GetPlayingVoiceCount() const {
    return static_cast<uint32_t>(m_activeVoices.size());
}

void AudioDeviceXAudio2::SetEventCallback(AudioEventCallback callback) {
    m_eventCallback = callback;
}

void AudioDeviceXAudio2::RemoveEventCallback() {
    m_eventCallback = nullptr;
}

AudioStats AudioDeviceXAudio2::GetStats() const {
    return m_stats;
}

void AudioDeviceXAudio2::ResetStats() {
    m_stats = AudioStats{};
}

void AudioDeviceXAudio2::BeginProfile() {
    // TODO: 实现性能分析
}

std::string AudioDeviceXAudio2::EndProfile() {
    return "";
}

std::string AudioDeviceXAudio2::GenerateDebugReport() {
    return "XAudio2 Debug Report (minimal implementation)";
}

// IXAudio2VoiceCallback 实现
void __stdcall AudioDeviceXAudio2::OnVoiceProcessingPassStart(UINT32 BytesRequired) {
    (void)BytesRequired;
}

void __stdcall AudioDeviceXAudio2::OnVoiceProcessingPassEnd() {
}

void __stdcall AudioDeviceXAudio2::OnStreamEnd() {
}

void __stdcall AudioDeviceXAudio2::OnBufferStart(void* pBufferContext) {
    (void)pBufferContext;
}

void __stdcall AudioDeviceXAudio2::OnBufferEnd(void* pBufferContext) {
    (void)pBufferContext;
}

void __stdcall AudioDeviceXAudio2::OnLoopEnd(void* pBufferContext) {
    (void)pBufferContext;
}

void __stdcall AudioDeviceXAudio2::OnVoiceError(void* pBufferContext, HRESULT Error) {
    (void)pBufferContext;
    (void)Error;
}

// 私有方法实现

bool AudioDeviceXAudio2::InitializeXAudio2() {
    return true;
}

bool AudioDeviceXAudio2::CreateMasteringVoice() {
    return true;
}

bool AudioDeviceXAudio2::Initialize3DAudio() {
    return true;
}

void AudioDeviceXAudio2::ReleaseAll() {
    m_activeVoices.clear();
    m_availableVoices.clear();
}

AudioDeviceXAudio2::Voice* AudioDeviceXAudio2::AllocateVoice() {
    if (!m_availableVoices.empty()) {
        Voice* voice = m_availableVoices.back();
        m_availableVoices.pop_back();
        return voice;
    }

    // 从池中分配
    for (auto& voice : m_voicePool) {
        if (!voice.isActive) {
            return &voice;
        }
    }

    return nullptr;
}

void AudioDeviceXAudio2::ReleaseVoice(Voice* voice) {
    if (voice) {
        voice->isActive = false;
        m_availableVoices.push_back(voice);
    }
}

AudioDeviceXAudio2::Voice* AudioDeviceXAudio2::FindVoice(AudioVoiceId voiceId) {
    auto it = m_activeVoices.find(voiceId);
    if (it != m_activeVoices.end()) {
        return it->second;
    }
    return nullptr;
}

bool AudioDeviceXAudio2::CreateWaveFormat(const AudioFormat& format, WAVEFORMATEX& waveFormat) {
    (void)format;
    (void)waveFormat;
    return true;
}

bool AudioDeviceXAudio2::SubmitBuffer(Voice* voice, bool forceStart) {
    (void)voice;
    (void)forceStart;
    return true;
}

void AudioDeviceXAudio2::Update3DAudio() {
}

void AudioDeviceXAudio2::Apply3DToVoice(Voice* voice) {
    (void)voice;
}

bool AudioDeviceXAudio2::CheckHResult(HRESULT hr, const std::string& operation) {
    (void)hr;
    (void)operation;
    return true;
}

void AudioDeviceXAudio2::TriggerEvent(AudioEventType type, AudioVoiceId voiceId, const std::string& message) {
    if (m_eventCallback) {
        AudioEvent event;
        event.type = type;
        event.voiceId = voiceId;
        event.message = message;
        m_eventCallback(event);
    }
}

AudioVoiceId AudioDeviceXAudio2::Play(const AudioClip& clip, const PlayDesc& desc) {
    return PlayClip(clip, desc);
}

void AudioDeviceXAudio2::StopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [voiceId, voice] : m_activeVoices) {
        (void)voiceId;
        voice->isActive = false;
    }

    m_activeVoices.clear();
    m_stats.activeVoiceCount = 0;
}

void AudioDeviceXAudio2::PauseAll() {
}

void AudioDeviceXAudio2::ResumeAll() {
}

void AudioDeviceXAudio2::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    (void)voiceId;
    (void)position;
}

void AudioDeviceXAudio2::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    (void)voiceId;
    (void)velocity;
}

void AudioDeviceXAudio2::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    (void)voiceId;
    (void)direction;
}

void AudioDeviceXAudio2::SetDopplerFactor(float factor) {
    (void)factor;
}

void AudioDeviceXAudio2::SetSpeedOfSound(float speed) {
    m_speedOfSound = speed;
}

VoiceState AudioDeviceXAudio2::GetVoiceState(AudioVoiceId voiceId) {
    auto it = m_activeVoices.find(voiceId);
    if (it == m_activeVoices.end()) {
        return VoiceState::Stopped;
    }

    if (it->second->isActive) {
        return VoiceState::Playing;
    }

    return VoiceState::Stopped;
}

} // namespace PrismaEngine::Audio
