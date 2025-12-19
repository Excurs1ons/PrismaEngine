#include "AudioDeviceSDL3.h"
#include "../Logger.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <chrono>

namespace Engine::Audio {

// ========== SDL3工厂函数 ==========

/// @brief 创建SDL3音频设备
/// @param desc 设备描述
/// @return 设备指针
std::unique_ptr<IAudioDevice> CreateSDL3Device(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceSDL3>();
    if (device->Initialize(desc)) {
        return device;
    }
    return nullptr;
}

// ========== AudioDeviceSDL3 实现 ==========

AudioDeviceSDL3::AudioDeviceSDL3() {
    // 初始化SDL音频规格
    SDL_zero(m_audioSpec);
    m_audioSpec.freq = 44100;
    m_audioSpec.format = SDL_AUDIO_F32;
    m_audioSpec.channels = 2;
    m_audioSpec.samples = 512;

    // 初始化统计
    ResetStats();
}

AudioDeviceSDL3::~AudioDeviceSDL3() {
    Shutdown();
}

bool AudioDeviceSDL3::Initialize(const AudioDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized.load()) {
        return true;
    }

    LOG_INFO("Audio", "初始化SDL3音频设备");
    m_desc = desc;

    // 初始化SDL音频子系统
    if (!InitializeSDLAudio()) {
        LOG_ERROR("Audio", "SDL音频子系统初始化失败");
        return false;
    }

    // 配置音频规格
    if (desc.outputFormat.sampleRate != 0) {
        m_audioSpec.freq = static_cast<int>(desc.outputFormat.sampleRate);
    }
    if (desc.outputFormat.channels != 0) {
        m_audioSpec.channels = static_cast<int>(desc.outputFormat.channels);
    }
    if (desc.bufferSize != 0) {
        m_audioSpec.samples = static_cast<int>(desc.bufferSize);
    }

    // 打开音频设备
    if (!OpenAudioDevice(desc.outputFormat)) {
        LOG_ERROR("Audio", "SDL3音频设备打开失败");
        Shutdown();
        return false;
    }

    // 设置音频回调
    SetupAudioCallback();

    // 初始化混音缓冲区
    m_mixBuffer.resize(m_audioSpec.samples * m_audioSpec.channels);

    m_masterVolume = 1.0f;
    m_initialized.store(true);

    LOG_INFO("Audio", "SDL3音频设备初始化成功，频率:{}Hz, 声道:{}, 缓冲区:{}采样",
              m_obtainedSpec.freq, m_obtainedSpec.channels, m_obtainedSpec.samples);
    return true;
}

void AudioDeviceSDL3::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized.load()) {
        return;
    }

    LOG_INFO("Audio", "关闭SDL3音频设备");

    // 释放所有资源
    ReleaseAll();

    // 关闭音频设备
    if (m_deviceId != 0) {
        SDL_CloseAudioDevice(m_deviceId);
        m_deviceId = 0;
    }

    m_initialized.store(false);
    LOG_INFO("Audio", "SDL3音频设备已关闭");
}

bool AudioDeviceSDL3::IsInitialized() const {
    return m_initialized.load();
}

void AudioDeviceSDL3::Update(float deltaTime) {
    if (!m_initialized.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 更新统计信息
    m_stats.activeVoices = static_cast<uint32_t>(m_playingVoices.size());
    if (m_stats.activeVoices > m_stats.maxConcurrentVoices) {
        m_stats.maxConcurrentVoices = m_stats.activeVoices;
    }

    // 更新音频状态
    UpdateVoiceStates();

    // 计算帧率
    m_framesProcessed++;
    double currentTime = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    if (currentTime - m_lastUpdateTime > 1.0) {
        double fps = m_framesProcessed / (currentTime - m_lastUpdateTime);
        m_framesProcessed = 0;
        m_lastUpdateTime = currentTime;
    }
}

// ========== 设备信息 ==========

IAudioDevice::DeviceInfo AudioDeviceSDL3::GetDeviceInfo() const {
    DeviceInfo info;

    if (m_deviceId != 0) {
        info.name = "SDL3 Audio Device";
        info.driver = "SDL3";
        info.version = SDL_GetRevision();
        info.isDefault = true;
        info.maxVoices = 256; // SDL3理论最大值
        info.supports3D = true; // 软件模拟
        info.supportsEffects = false;
    }

    return info;
}

std::vector<IAudioDevice::DeviceInfo> AudioDeviceSDL3::GetAvailableDevices() const {
    std::vector<DeviceInfo> devices;

    // SDL3不直接支持多设备枚举
    devices.push_back(GetDeviceInfo());

    return devices;
}

// ========== 播放控制 ==========

AudioVoiceId AudioDeviceSDL3::Play(const AudioClip& clip, const PlayDesc& desc) {
    if (!m_initialized.load() || !clip.IsValid()) {
        return INVALID_VOICE_ID;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 生成Voice ID
    AudioVoiceId voiceId = GenerateVoiceId();

    // 创建音频流
    SDL_AudioStream* stream = CreateAudioStream(clip);
    if (!stream) {
        LOG_ERROR("Audio", "创建音频流失败");
        return INVALID_VOICE_ID;
    }

    // 创建播放Voice
    PlayingVoice voice;
    voice.stream = stream;
    voice.audioData = clip.data; // 复制数据
    voice.volume = desc.volume;
    voice.pitch = desc.pitch; // SDL3不支持变调
    voice.looping = desc.loop;
    voice.paused = false;
    voice.duration = clip.duration;
    voice.isActive = true;
    voice.state = VoiceState::Playing;

    // 应用3D属性
    if (desc.is3D) {
        voice.volume *= Calculate3DVolume(desc.spatial, m_listener);
    }

    // 添加到播放列表
    m_playingVoices[voiceId] = voice;

    // 更新统计
    m_stats.totalVoicesCreated++;

    // 触发事件
    TriggerEvent(AudioEventType::VoiceStarted, voiceId);

    LOG_TRACE("Audio", "开始播放音频: {} (Voice ID: {})", clip.path, voiceId);
    return voiceId;
}

void AudioDeviceSDL3::Stop(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end()) {
        return;
    }

    PlayingVoice& voice = it->second;
    voice.state = VoiceState::Stopped;
    voice.isActive = false;

    TriggerEvent(AudioEventType::VoiceStopped, voiceId);

    // 立即移除
    RemoveVoice(voiceId);
}

void AudioDeviceSDL3::Pause(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end()) {
        return;
    }

    PlayingVoice& voice = it->second;
    if (voice.state == VoiceState::Playing) {
        voice.paused = true;
        voice.state = VoiceState::Paused;
        TriggerEvent(AudioEventType::VoicePaused, voiceId);
    }
}

void AudioDeviceSDL3::Resume(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end()) {
        return;
    }

    PlayingVoice& voice = it->second;
    if (voice.state == VoiceState::Paused) {
        voice.paused = false;
        voice.state = VoiceState::Playing;
        TriggerEvent(AudioEventType::VoiceResumed, voiceId);
    }
}

void AudioDeviceSDL3::StopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [voiceId, voice] : m_playingVoices) {
        voice.state = VoiceState::Stopped;
        voice.isActive = false;
        TriggerEvent(AudioEventType::VoiceStopped, voiceId);
    }

    m_playingVoices.clear();
}

void AudioDeviceSDL3::PauseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [voiceId, voice] : m_playingVoices) {
        if (voice.state == VoiceState::Playing) {
            voice.paused = true;
            voice.state = VoiceState::Paused;
            TriggerEvent(AudioEventType::VoicePaused, voiceId);
        }
    }
}

void AudioDeviceSDL3::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [voiceId, voice] : m_playingVoices) {
        if (voice.state == VoiceState::Paused) {
            voice.paused = false;
            voice.state = VoiceState::Playing;
            TriggerEvent(AudioEventType::VoiceResumed, voiceId);
        }
    }
}

// ========== 实时控制 ==========

void AudioDeviceSDL3::SetVolume(AudioVoiceId voiceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        it->second.volume = volume;
    }
}

void AudioDeviceSDL3::SetPitch(AudioVoiceId voiceId, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // SDL3不支持实时变调，只能记录
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        it->second.pitch = pitch;
    }
}

void AudioDeviceSDL3::SetPlaybackPosition(AudioVoiceId voiceId, float time) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end() || it->second.duration <= 0.0f) {
        return;
    }

    // 计算字节偏移
    float progress = std::clamp(time / it->second.duration, 0.0f, 1.0f);
    it->second.currentPosition = static_cast<size_t>(
        progress * it->second.audioData.size()
    );

    ResetStreamPosition(it->second);
}

// ========== 3D音频（模拟实现） ==========

void AudioDeviceSDL3::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    // SDL3不原生支持3D音频，这里只是记录
    // 实际的3D效果在混音时计算
}

void AudioDeviceSDL3::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    // SDL3不原生支持3D音频
}

void AudioDeviceSDL3::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    // SDL3不原生支持3D音频
}

void AudioDeviceSDL3::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    // SDL3不原生支持3D音频，使用简单的距离衰减
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        float volume3D = Calculate3DVolume(attributes, m_listener);
        it->second.volume = it->second.volume * volume3D;
    }
}

void AudioDeviceSDL3::SetListener(const AudioListener& listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listener = listener;
}

void AudioDeviceSDL3::SetDistanceModel(DistanceModel model) {
    // SDL3不原生支持，使用软件模拟
}

void AudioDeviceSDL3::SetDopplerFactor(float factor) {
    // SDL3不支持多普勒效应
}

void AudioDeviceSDL3::SetSpeedOfSound(float speed) {
    // SDL3不支持多普勒效应
}

// ========== 全局控制 ==========

void AudioDeviceSDL3::SetMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

float AudioDeviceSDL3::GetMasterVolume() const {
    return m_masterVolume;
}

// ========== 查询 ==========

bool AudioDeviceSDL3::IsPlaying(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    return it != m_playingVoices.end() && it->second.state == VoiceState::Playing;
}

bool AudioDeviceSDL3::IsPaused(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    return it != m_playingVoices.end() && it->second.state == VoiceState::Paused;
}

bool AudioDeviceSDL3::IsStopped(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    return it == m_playingVoices.end() || it->second.state == VoiceState::Stopped;
}

float AudioDeviceSDL3::GetPlaybackPosition(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playingVoices.find(voiceId);
    if (it == m_playingVoices.end() || it->second.audioData.empty()) {
        return -1.0f;
    }

    float progress = static_cast<float>(it->second.currentPosition) / it->second.audioData.size();
    return progress * it->second.duration;
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
    return static_cast<uint32_t>(m_playingVoices.size());
}

// ========== 事件系统 ==========

void AudioDeviceSDL3::SetEventCallback(AudioEventCallback callback) {
    m_eventCallback = callback;
}

void AudioDeviceSDL3::RemoveEventCallback() {
    m_eventCallback = nullptr;
}

// ========== 统计 ==========

AudioStats AudioDeviceSDL3::GetStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_stats.activeVoices = static_cast<uint32_t>(m_playingVoices.size());
    m_stats.memoryUsage = m_playingVoices.size() * sizeof(PlayingVoice);

    return m_stats;
}

void AudioDeviceSDL3::ResetStats() {
    m_stats = AudioStats{};
}

// ========== 私有方法实现 ==========

bool AudioDeviceSDL3::InitializeSDLAudio() {
    // 初始化SDL音频子系统
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("Audio", "SDL音频初始化失败: {}", SDL_GetError());
        return false;
    }

    return true;
}

bool AudioDeviceSDL3::OpenAudioDevice(const AudioFormat& format) {
    // 打开音频设备
    m_deviceId = SDL_OpenAudioDevice(nullptr, 0, &m_audioSpec, &m_obtainedSpec, 0);
    if (m_deviceId == 0) {
        LOG_ERROR("Audio", "无法打开SDL音频设备: {}", SDL_GetError());
        return false;
    }

    LOG_INFO("Audio", "SDL音频设备已打开，实际规格: {}Hz, {}声道, {}位",
              m_obtainedSpec.freq, m_obtainedSpec.channels,
              SDL_AUDIO_BITSIZE(m_obtainedSpec.format));

    return true;
}

void AudioDeviceSDL3::SetupAudioCallback() {
    // SDL3使用新的回调方式
    SDL_ResumeAudioDevice(m_deviceId);
}

void AudioDeviceSDL3::ReleaseAll() {
    // 暂停设备
    if (m_deviceId != 0) {
        SDL_PauseAudioDevice(m_deviceId);
    }

    // 释放所有音频流
    for (auto& [voiceId, voice] : m_playingVoices) {
        DestroyAudioStream(voice.stream);
    }

    m_playingVoices.clear();
    m_mixBuffer.clear();
}

SDL_AudioStream* AudioDeviceSDL3::CreateAudioStream(const AudioClip& clip) {
    // 创建输入规格（根据clip）
    SDL_AudioSpec inputSpec;
    inputSpec.freq = static_cast<int>(clip.format.sampleRate);
    inputSpec.channels = static_cast<int>(clip.format.channels);

    // 根据位数确定格式
    switch (clip.format.bitsPerSample) {
        case 16: inputSpec.format = SDL_AUDIO_S16; break;
        case 32: inputSpec.format = SDL_AUDIO_S32; break;
        default: inputSpec.format = SDL_AUDIO_S16; break;
    }

    // 创建音频流
    SDL_AudioStream* stream = SDL_CreateAudioStream(&inputSpec, &m_obtainedSpec);
    if (!stream) {
        LOG_ERROR("Audio", "创建音频流失败: {}", SDL_GetError());
        return nullptr;
    }

    // 将数据放入流
    if (SDL_PutAudioStreamData(stream, clip.data.data(), clip.data.size()) != 0) {
        LOG_ERROR("Audio", "设置音频流数据失败: {}", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        return nullptr;
    }

    return stream;
}

void AudioDeviceSDL3::DestroyAudioStream(SDL_AudioStream* stream) {
    if (stream) {
        SDL_DestroyAudioStream(stream);
    }
}

void AudioDeviceSDL3::ResetStreamPosition(PlayingVoice& voice) {
    // 重置音频流位置
    SDL_ClearAudioStream(voice.stream);

    // 重新放入数据
    SDL_PutAudioStreamData(voice.stream,
                          voice.audioData.data() + voice.currentPosition,
                          voice.audioData.size() - voice.currentPosition);
}

AudioVoiceId AudioDeviceSDL3::GenerateVoiceId() {
    return m_nextVoiceId.fetch_add(1);
}

AudioDeviceSDL3::PlayingVoice* AudioDeviceSDL3::FindVoice(AudioVoiceId voiceId) {
    auto it = m_playingVoices.find(voiceId);
    return it != m_playingVoices.end() ? &it->second : nullptr;
}

void AudioDeviceSDL3::RemoveVoice(AudioVoiceId voiceId) {
    auto it = m_playingVoices.find(voiceId);
    if (it != m_playingVoices.end()) {
        DestroyAudioStream(it->second.stream);
        m_playingVoices.erase(it);
    }
}

void AudioDeviceSDL3::UpdateVoiceStates() {
    std::vector<AudioVoiceId> toRemove;

    for (auto& [voiceId, voice] : m_playingVoices) {
        if (!voice.isActive || voice.state == VoiceState::Stopped) {
            toRemove.push_back(voiceId);
            continue;
        }

        // 检查是否播放完成
        if (voice.state == VoiceState::Playing) {
            // 简单的时长检查（实际应该基于音频流状态）
            // TODO: 完善播放状态检查
        }
    }

    // 移除已完成的音频
    for (AudioVoiceId id : toRemove) {
        RemoveVoice(id);
    }
}

// SDL3音频回调
void AudioDeviceSDL3::AudioCallback(void* userdata, SDL_AudioStream* stream,
                                   int additional_amount, int total_amount) {
    AudioDeviceSDL3* device = static_cast<AudioDeviceSDL3*>(userdata);
    device->HandleAudioCallback(stream, additional_amount, total_amount);
}

void AudioDeviceSDL3::HandleAudioCallback(SDL_AudioStream* stream,
                                          int additional_amount, int total_amount) {
    // 清空混音缓冲区
    std::fill(m_mixBuffer.begin(), m_mixBuffer.end(), 0.0f);

    // 计算需要多少样本
    int samplesNeeded = total_amount / sizeof(float);
    if (samplesNeeded > m_mixBuffer.size()) {
        m_mixBuffer.resize(samplesNeeded);
    }

    // 混合所有音频
    for (auto& [voiceId, voice] : m_playingVoices) {
        if (voice.paused || voice.state != VoiceState::Playing) {
            continue;
        }

        // 获取音频数据
        std::vector<float> tempBuffer(samplesNeeded);
        int got = SDL_GetAudioStreamData(voice.stream,
                                         tempBuffer.data(),
                                         samplesNeeded * sizeof(float));

        if (got > 0) {
            int samples = got / sizeof(float);
            // 混音
            MixAudio(m_mixBuffer.data(), tempBuffer.data(), samples,
                     voice.volume * m_masterVolume);
        } else if (voice.looping) {
            // 循环播放
            SDL_ClearAudioStream(voice.stream);
            SDL_PutAudioStreamData(voice.stream, voice.audioData.data(), voice.audioData.size());

            // 再次尝试获取数据
            got = SDL_GetAudioStreamData(voice.stream,
                                         tempBuffer.data(),
                                         samplesNeeded * sizeof(float));
            if (got > 0) {
                int samples = got / sizeof(float);
                MixAudio(m_mixBuffer.data(), tempBuffer.data(), samples,
                         voice.volume * m_masterVolume);
            } else {
                // 播放结束
                TriggerEvent(AudioEventType::VoiceStopped, voiceId);
                voice.state = VoiceState::Stopped;
                voice.isActive = false;
            }
        } else {
            // 播放结束
            TriggerEvent(AudioEventType::VoiceStopped, voiceId);
            voice.state = VoiceState::Stopped;
            voice.isActive = false;
        }
    }

    // 将混音结果输出
    if (m_mixBuffer.size() > 0) {
        SDL_PutAudioStreamData(stream, m_mixBuffer.data(),
                              samplesNeeded * sizeof(float));
    }

    m_totalSamplesProcessed += samplesNeeded;
}

void AudioDeviceSDL3::MixAudio(float* output, const float* input, int samples, float volume) {
    // 简单的加法混音（需要防止溢出）
    for (int i = 0; i < samples; ++i) {
        output[i] += input[i] * volume;

        // 限制在[-1.0, 1.0]范围内
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }
}

float AudioDeviceSDL3::Calculate3DVolume(const Audio3DAttributes& spatial, const AudioListener& listener) {
    // 计算距离
    float dx = spatial.position[0] - listener.position[0];
    float dy = spatial.position[1] - listener.position[1];
    float dz = spatial.position[2] - listener.position[2];
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 距离衰减
    if (distance <= spatial.minDistance) {
        return 1.0f;
    } else if (distance >= spatial.maxDistance) {
        return 0.0f;
    } else {
        // 线性衰减
        float attenuation = 1.0f - (distance - spatial.minDistance) /
                                    (spatial.maxDistance - spatial.minDistance);
        return attenuation * spatial.rolloffFactor;
    }
}

float AudioDeviceSDL3::CalculateSourceAngle(const Audio3DAttributes& spatial, const AudioListener& listener) {
    // 计算声源相对于听者的方向
    float dx = spatial.position[0] - listener.position[0];
    float dy = spatial.position[1] - listener.position[1];
    float dz = spatial.position[2] - listener.position[2];

    // 归一化
    float length = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (length < 0.001f) {
        return 0.0f;
    }

    dx /= length;
    dy /= length;
    dz /= length;

    // 计算与听者前向量的夹角
    float dot = dx * listener.forward[0] +
                dy * listener.forward[1] +
                dz * listener.forward[2];

    return std::acos(std::clamp(dot, -1.0f, 1.0f));
}

void AudioDeviceSDL3::TriggerEvent(AudioEventType type, AudioVoiceId voiceId, const std::string& message) {
    if (m_eventCallback) {
        AudioEvent event;
        event.type = type;
        event.voiceId = voiceId;
        event.message = message;
        event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        m_eventCallback(event);
    }
}

} // namespace Engine::Audio