#include "AudioDeviceOpenAL.h"
#include "../Logger.h"
#include <algorithm>
#include <cstring>
#include <chrono>

namespace Engine::Audio {

// ========== OpenAL工厂函数 ==========

/// @brief 创建OpenAL音频设备
/// @param desc 设备描述
/// @return 设备指针
std::unique_ptr<IAudioDevice> CreateOpenALDevice(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceOpenAL>();
    if (device->Initialize(desc)) {
        return device;
    }
    return nullptr;
}

// ========== AudioDeviceOpenAL 实现 ==========

AudioDeviceOpenAL::AudioDeviceOpenAL() {
    // 初始化语音池
    for (size_t i = 0; i < MAX_VOICES; ++i) {
        m_availableVoices.push_back(&m_voicePool[i]);
    }

    // 初始化统计
    ResetStats();
}

AudioDeviceOpenAL::~AudioDeviceOpenAL() {
    Shutdown();
}

bool AudioDeviceOpenAL::Initialize(const AudioDesc& desc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized.load()) {
        return true;
    }

    LOG_INFO("Audio", "初始化OpenAL音频设备");
    m_desc = desc;

    // 初始化设备
    if (!InitializeDevice(desc.deviceName)) {
        LOG_ERROR("Audio", "OpenAL设备初始化失败");
        return false;
    }

    // 初始化上下文
    if (!InitializeContext()) {
        LOG_ERROR("Audio", "OpenAL上下文初始化失败");
        Shutdown();
        return false;
    }

    // 初始化音频源池
    if (!InitializeVoicePool(desc.maxVoices)) {
        LOG_ERROR("Audio", "OpenAL音频源池初始化失败");
        Shutdown();
        return false;
    }

    // 初始化音效系统（如果需要）
    if (desc.enableEffects) {
        InitializeEFX();
    }

    // 设置默认监听器
    AudioListener listener;
    SetListener(listener);

    // 设置3D音频参数
    SetDistanceModel(desc.distanceModel);
    SetDopplerFactor(desc.dopplerFactor);
    SetSpeedOfSound(desc.speedOfSound);

    m_masterVolume = 1.0f;
    m_initialized.store(true);

    LOG_INFO("Audio", "OpenAL音频设备初始化成功，最大音频源数: {}", desc.maxVoices);
    return true;
}

void AudioDeviceOpenAL::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized.load()) {
        return;
    }

    LOG_INFO("Audio", "关闭OpenAL音频设备");

    // 停止所有音频源
    StopAll();

    // 释放资源
    ReleaseAll();

    // 关闭上下文和设备
    if (m_context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        m_context = nullptr;
    }

    if (m_device) {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }

    m_initialized.store(false);
    LOG_INFO("Audio", "OpenAL音频设备已关闭");
}

bool AudioDeviceOpenAL::IsInitialized() const {
    return m_initialized.load();
}

void AudioDeviceOpenAL::Update(float deltaTime) {
    if (!m_initialized.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 更新统计信息
    m_stats.activeVoices = static_cast<uint32_t>(m_activeVoices.size());
    if (m_stats.activeVoices > m_stats.maxConcurrentVoices) {
        m_stats.maxConcurrentVoices = m_stats.activeVoices;
    }

    // 处理已结束的音频源
    ProcessFinishedVoices();

    // 更新音频源状态
    for (auto& [voiceId, voice] : m_activeVoices) {
        UpdateVoiceState(voice);
    }
}

// ========== 设备信息 ==========

IAudioDevice::DeviceInfo AudioDeviceOpenAL::GetDeviceInfo() const {
    DeviceInfo info;

    if (m_device) {
        info.name = alcGetString(m_device, ALC_DEVICE_SPECIFIER);
        info.driver = "OpenAL";
        info.version = "1.1";
        info.isDefault = true;
        info.maxVoices = MAX_VOICES;
        info.supports3D = true;
        info.supportsEffects = m_hasEFX;
    }

    return info;
}

std::vector<IAudioDevice::DeviceInfo> AudioDeviceOpenAL::GetAvailableDevices() const {
    std::vector<DeviceInfo> devices;

    if (!alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT")) {
        // 不支持设备枚举，返回默认设备
        devices.push_back(GetDeviceInfo());
        return devices;
    }

    const ALCchar* deviceList = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
    if (!deviceList) {
        return devices;
    }

    const ALCchar* device = deviceList;
    while (*device && strlen(device) > 0) {
        DeviceInfo info;
        info.name = device;
        info.driver = "OpenAL";
        info.version = "1.1";
        info.isDefault = (info.name == GetDeviceInfo().name);
        info.maxVoices = MAX_VOICES;
        info.supports3D = true;
        info.supportsEffects = m_hasEFX;

        devices.push_back(info);
        device += strlen(device) + 1;
    }

    return devices;
}

// ========== 播放控制 ==========

AudioVoiceId AudioDeviceOpenAL::Play(const AudioClip& clip, const PlayDesc& desc) {
    if (!m_initialized.load() || !clip.IsValid()) {
        return INVALID_VOICE_ID;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 分配音频源
    Voice* voice = AllocateVoice();
    if (!voice) {
        LOG_WARNING("Audio", "无可用音频源");
        return INVALID_VOICE_ID;
    }

    // 生成或获取缓冲区
    ALuint bufferId = GetOrCreateBuffer(clip);
    if (bufferId == 0) {
        ReleaseVoice(voice);
        return INVALID_VOICE_ID;
    }

    // 设置音频源
    alSourcei(voice->sourceId, AL_BUFFER, static_cast<ALint>(bufferId));
    alSourcef(voice->sourceId, AL_GAIN, desc.volume * m_masterVolume);
    alSourcef(voice->sourceId, AL_PITCH, desc.pitch);
    alSourcei(voice->sourceId, AL_LOOPING, desc.loop ? AL_TRUE : AL_FALSE);

    // 保存音频数据
    voice->clip = clip;
    voice->desc = desc;
    voice->isLooping = desc.loop;
    voice->basePitch = desc.pitch;
    voice->baseVolume = desc.volume;
    voice->state = VoiceState::Playing;
    voice->isActive = true;

    // 应用3D属性
    if (desc.is3D) {
        voice->desc.spatial = desc.spatial;
        Apply3DAttributes(voice);
    }

    // 播放音频
    alSourcePlay(voice->sourceId);

    if (CheckOpenALError("Play")) {
        ReleaseVoice(voice);
        return INVALID_VOICE_ID;
    }

    // 生成Voice ID并加入活跃列表
    AudioVoiceId voiceId = m_nextVoiceId.fetch_add(1);
    m_activeVoices[voiceId] = voice;

    // 更新统计
    m_stats.totalVoicesCreated++;

    // 触发事件
    TriggerEvent(AudioEventType::VoiceStarted, voiceId);

    LOG_TRACE("Audio", "开始播放音频: {} (Voice ID: {})", clip.path, voiceId);
    return voiceId;
}

void AudioDeviceOpenAL::Stop(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->state == VoiceState::Stopped) {
        return;
    }

    alSourceStop(voice->sourceId);
    voice->state = VoiceState::Stopped;

    TriggerEvent(AudioEventType::VoiceStopped, voiceId);

    // 稍后释放
    UpdateVoiceState(voice);
}

void AudioDeviceOpenAL::Pause(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->state != VoiceState::Playing) {
        return;
    }

    alSourcePause(voice->sourceId);
    voice->state = VoiceState::Paused;

    TriggerEvent(AudioEventType::VoicePaused, voiceId);
}

void AudioDeviceOpenAL::Resume(AudioVoiceId voiceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->state != VoiceState::Paused) {
        return;
    }

    alSourcePlay(voice->sourceId);
    voice->state = VoiceState::Playing;

    TriggerEvent(AudioEventType::VoiceResumed, voiceId);
}

void AudioDeviceOpenAL::StopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& voice : m_voicePool) {
        if (voice.isActive) {
            alSourceStop(voice.sourceId);
            voice.state = VoiceState::Stopped;
        }
    }
}

void AudioDeviceOpenAL::PauseAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 收集所有活跃音频源的 sourceId
    std::vector<ALuint> sources;
    sources.reserve(m_activeVoices.size());
    for (const auto& [voiceId, voice] : m_activeVoices) {
        if (voice && voice->sourceId != 0) {
            sources.push_back(voice->sourceId);
        }
    }

    alSourcePausev(static_cast<ALsizei>(sources.size()),
                   sources.data());

    for (auto& voice : m_voicePool) {
        if (voice.isActive && voice.state == VoiceState::Playing) {
            voice.state = VoiceState::Paused;
        }
    }
}

void AudioDeviceOpenAL::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<ALuint> sources;
    for (auto& voice : m_voicePool) {
        if (voice.isActive && voice.state == VoiceState::Paused) {
            sources.push_back(voice.sourceId);
            voice.state = VoiceState::Playing;
        }
    }

    if (!sources.empty()) {
        alSourcePlayv(static_cast<ALsizei>(sources.size()), sources.data());
    }
}

// ========== 实时控制 ==========

void AudioDeviceOpenAL::SetVolume(AudioVoiceId voiceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->baseVolume = volume;
    alSourcef(voice->sourceId, AL_GAIN, volume * m_masterVolume);
}

void AudioDeviceOpenAL::SetPitch(AudioVoiceId voiceId, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->basePitch = pitch;
    alSourcef(voice->sourceId, AL_PITCH, pitch);
}

void AudioDeviceOpenAL::SetPlaybackPosition(AudioVoiceId voiceId, float time) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->clip.data.empty()) {
        return;
    }

    // 计算字节偏移
    float duration = voice->clip.duration;
    if (duration <= 0.0f) {
        return;
    }

    float progress = time / duration;
    if (progress < 0.0f || progress > 1.0f) {
        return;
    }

    ALsizei byteOffset = static_cast<ALsizei>(
        progress * voice->clip.data.size()
    );

    alSourcei(voice->sourceId, AL_BYTE_OFFSET, byteOffset);
    voice->playbackPosition = time;
}

// ========== 3D音频 ==========

void AudioDeviceOpenAL::SetVoice3DPosition(AudioVoiceId voiceId, const float position[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->desc.spatial.position[0] = position[0];
    voice->desc.spatial.position[1] = position[1];
    voice->desc.spatial.position[2] = position[2];

    alSource3f(voice->sourceId, AL_POSITION, position[0], position[1], position[2]);
}

void AudioDeviceOpenAL::SetVoice3DVelocity(AudioVoiceId voiceId, const float velocity[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->desc.spatial.velocity[0] = velocity[0];
    voice->desc.spatial.velocity[1] = velocity[1];
    voice->desc.spatial.velocity[2] = velocity[2];

    alSource3f(voice->sourceId, AL_VELOCITY, velocity[0], velocity[1], velocity[2]);
}

void AudioDeviceOpenAL::SetVoice3DDirection(AudioVoiceId voiceId, const float direction[3]) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->desc.spatial.direction[0] = direction[0];
    voice->desc.spatial.direction[1] = direction[1];
    voice->desc.spatial.direction[2] = direction[2];

    alSource3f(voice->sourceId, AL_DIRECTION, direction[0], direction[1], direction[2]);
}

void AudioDeviceOpenAL::SetVoice3DAttributes(AudioVoiceId voiceId, const Audio3DAttributes& attributes) {
    std::lock_guard<std::mutex> lock(m_mutex);

    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return;
    }

    voice->desc.spatial = attributes;
    Apply3DAttributes(voice);
}

void AudioDeviceOpenAL::SetListener(const AudioListener& listener) {
    alListener3f(AL_POSITION, listener.position[0], listener.position[1], listener.position[2]);
    alListener3f(AL_VELOCITY, listener.velocity[0], listener.velocity[1], listener.velocity[2]);

    float orientation[6] = {
        listener.forward[0], listener.forward[1], listener.forward[2],
        listener.up[0], listener.up[1], listener.up[2]
    };
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioDeviceOpenAL::SetDistanceModel(DistanceModel model) {
    ALenum alModel = GetOpenALDistanceModel(model);
    alDistanceModel(alModel);
}

void AudioDeviceOpenAL::SetDopplerFactor(float factor) {
    alDopplerFactor(factor);
}

void AudioDeviceOpenAL::SetSpeedOfSound(float speed) {
    alSpeedOfSound(speed);
}

// ========== 全局控制 ==========

void AudioDeviceOpenAL::SetMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);

    // 更新所有音频源的音量
    for (auto& voice : m_voicePool) {
        if (voice.isActive) {
            alSourcef(voice.sourceId, AL_GAIN, voice.baseVolume * m_masterVolume);
        }
    }
}

float AudioDeviceOpenAL::GetMasterVolume() const {
    return m_masterVolume;
}

// ========== 查询 ==========

bool AudioDeviceOpenAL::IsPlaying(AudioVoiceId voiceId) {
    Voice* voice = FindVoice(voiceId);
    return voice && voice->state == VoiceState::Playing;
}

bool AudioDeviceOpenAL::IsPaused(AudioVoiceId voiceId) {
    Voice* voice = FindVoice(voiceId);
    return voice && voice->state == VoiceState::Paused;
}

bool AudioDeviceOpenAL::IsStopped(AudioVoiceId voiceId) {
    Voice* voice = FindVoice(voiceId);
    return voice && voice->state == VoiceState::Stopped;
}

float AudioDeviceOpenAL::GetPlaybackPosition(AudioVoiceId voiceId) {
    Voice* voice = FindVoice(voiceId);
    if (!voice || voice->clip.data.empty()) {
        return -1.0f;
    }

    ALint byteOffset;
    alGetSourcei(voice->sourceId, AL_BYTE_OFFSET, &byteOffset);

    float progress = static_cast<float>(byteOffset) / voice->clip.data.size();
    return progress * voice->clip.duration;
}

float AudioDeviceOpenAL::GetDuration(AudioVoiceId voiceId) {
    Voice* voice = FindVoice(voiceId);
    if (!voice) {
        return -1.0f;
    }

    return voice->clip.duration;
}

VoiceState AudioDeviceOpenAL::GetVoiceState(AudioVoiceId voiceId) {
    Voice* voice = FindVoice(voiceId);
    return voice ? voice->state : VoiceState::Stopped;
}

uint32_t AudioDeviceOpenAL::GetPlayingVoiceCount() const {
    return static_cast<uint32_t>(m_activeVoices.size());
}

// ========== 音效 ==========

bool AudioDeviceOpenAL::ApplyEffect(AudioVoiceId voiceId, EffectType effectType, const void* params) {
    if (!m_hasEFX) {
        return false;
    }

    // TODO: 实现EFX音效
    return false;
}

void AudioDeviceOpenAL::RemoveEffects(AudioVoiceId voiceId) {
    // TODO: 移除音效
}

// ========== 事件系统 ==========

void AudioDeviceOpenAL::SetEventCallback(AudioEventCallback callback) {
    m_eventCallback = callback;
}

void AudioDeviceOpenAL::RemoveEventCallback() {
    m_eventCallback = nullptr;
}

// ========== 统计 ==========

AudioStats AudioDeviceOpenAL::GetStats() const {
    m_stats.activeVoices = static_cast<uint32_t>(m_activeVoices.size());
    return m_stats;
}

void AudioDeviceOpenAL::ResetStats() {
    m_stats = AudioStats{};
}

// ========== 调试 ==========

void AudioDeviceOpenAL::BeginProfile() {
    m_profilingEnabled = true;
    m_profileStartTime = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

std::string AudioDeviceOpenAL::EndProfile() {
    if (!m_profilingEnabled) {
        return "";
    }

    double endTime = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    m_profilingEnabled = false;

    return "OpenAL Profile: " + std::to_string(endTime - m_profileStartTime) + " seconds";
}

// ========== 私有方法实现 ==========

bool AudioDeviceOpenAL::InitializeDevice(const std::string& deviceName) {
    // 打开设备
    if (deviceName.empty()) {
        m_device = alcOpenDevice(nullptr);
    } else {
        m_device = alcOpenDevice(deviceName.c_str());
    }

    if (!m_device) {
        LOG_ERROR("Audio", "无法打开OpenAL设备: {}", deviceName);
        return false;
    }

    return true;
}

bool AudioDeviceOpenAL::InitializeContext() {
    // 创建属性列表
    std::vector<int> attributes;

    // 设置采样率
    if (m_desc.outputFormat.sampleRate != 0) {
        attributes.push_back(ALC_FREQUENCY);
        attributes.push_back(static_cast<int>(m_desc.outputFormat.sampleRate));
    }

    // 设置声道数
    if (m_desc.outputFormat.channels != 0) {
        attributes.push_back(ALC_MONO_SOURCES);
        attributes.push_back(m_desc.outputFormat.channels == 1 ? m_desc.maxVoices : 0);

        attributes.push_back(ALC_STEREO_SOURCES);
        attributes.push_back(m_desc.outputFormat.channels == 2 ? m_desc.maxVoices : 0);
    }

    // 结束标记
    attributes.push_back(0);

    // 创建上下文
    m_context = alcCreateContext(m_device, attributes.data());
    if (!m_context) {
        LOG_ERROR("Audio", "无法创建OpenAL上下文");
        return false;
    }

    // 设置当前上下文
    if (!alcMakeContextCurrent(m_context)) {
        LOG_ERROR("Audio", "无法设置OpenAL上下文");
        return false;
    }

    return true;
}

bool AudioDeviceOpenAL::InitializeVoicePool(uint32_t maxVoices) {
    uint32_t voiceCount = std::min(maxVoices, static_cast<uint32_t>(MAX_VOICES));

    std::vector<ALuint> sourceIds(voiceCount);
    alGenSources(static_cast<ALsizei>(voiceCount), sourceIds.data());

    if (CheckOpenALError("InitializeVoicePool")) {
        return false;
    }

    // 分配给语音池
    for (size_t i = 0; i < voiceCount; ++i) {
        m_voicePool[i].sourceId = sourceIds[i];
    }

    return true;
}

bool AudioDeviceOpenAL::InitializeEFX() {
    // 检查EFX扩展
    if (alcIsExtensionPresent(nullptr, "ALC_EXT_EFX")) {
        m_hasEFX = true;

        // 加载EFX函数指针
        alGenEffects = reinterpret_cast<LPALGENEFFECTS>(
            alGetProcAddress("alGenEffects"));
        alDeleteEffects = reinterpret_cast<LPALDELETEEFFECTS>(
            alGetProcAddress("alDeleteEffects"));
        alEffecti = reinterpret_cast<LPALEFFECTI>(
            alGetProcAddress("alEffecti"));
        alEffectf = reinterpret_cast<LPALEFFECTF>(
            alGetProcAddress("alEffectf"));

        LOG_INFO("Audio", "OpenAL EFX音效系统已启用");
    } else {
        m_hasEFX = false;
        LOG_INFO("Audio", "OpenAL EFX音效系统不可用");
    }

    return m_hasEFX;
}

void AudioDeviceOpenAL::ReleaseAll() {
    // 停止所有音频源
    for (auto& voice : m_voicePool) {
        if (voice.isActive) {
            alSourceStop(voice.sourceId);
        }
    }

    // 释放源
    std::vector<ALuint> sourceIds;
    for (auto& voice : m_voicePool) {
        if (voice.sourceId != 0) {
            sourceIds.push_back(voice.sourceId);
        }
    }

    if (!sourceIds.empty()) {
        alDeleteSources(static_cast<ALsizei>(sourceIds.size()), sourceIds.data());
    }

    // 释放缓冲区
    for (auto& [clip, bufferId] : m_bufferCache) {
        alDeleteBuffers(1, &bufferId);
    }

    // 清理状态
    m_voicePool = {};
    m_availableVoices.clear();
    m_activeVoices.clear();
    m_bufferCache.clear();
}

AudioDeviceOpenAL::Voice* AudioDeviceOpenAL::AllocateVoice() {
    if (m_availableVoices.empty()) {
        return nullptr;
    }

    Voice* voice = m_availableVoices.back();
    m_availableVoices.pop_back();
    return voice;
}

void AudioDeviceOpenAL::ReleaseVoice(Voice* voice) {
    if (!voice || !voice->isActive) {
        return;
    }

    // 停止并重置音频源
    alSourceStop(voice->sourceId);
    alSourcei(voice->sourceId, AL_BUFFER, 0);

    // 重置状态
    voice->isActive = false;
    voice->state = VoiceState::Stopped;
    voice->clip = AudioClip{};

    // 归还到可用列表
    m_availableVoices.push_back(voice);
}

AudioDeviceOpenAL::Voice* AudioDeviceOpenAL::FindVoice(AudioVoiceId voiceId) {
    auto it = m_activeVoices.find(voiceId);
    return (it != m_activeVoices.end()) ? it->second : nullptr;
}

ALenum AudioDeviceOpenAL::GetOpenALFormat(const AudioFormat& format) const {
    switch (format.channels) {
        case 1: // Mono
            switch (format.bitsPerSample) {
                case 8: return AL_FORMAT_MONO8;
                case 16: return AL_FORMAT_MONO16;
                case 32: return AL_FORMAT_MONO_FLOAT32;
            }
            break;

        case 2: // Stereo
            switch (format.bitsPerSample) {
                case 8: return AL_FORMAT_STEREO8;
                case 16: return AL_FORMAT_STEREO16;
                case 32: return AL_FORMAT_STEREO_FLOAT32;
            }
            break;

        case 4: // Quad
            if (format.bitsPerSample == 16) {
                return AL_FORMAT_QUAD16;
            }
            break;

        case 6: // 5.1
            switch (format.bitsPerSample) {
                case 16: return AL_FORMAT_51CHN16;
                case 32: return AL_FORMAT_51CHN32;
            }
            break;

        case 7: // 6.1
            if (format.bitsPerSample == 16) {
                return AL_FORMAT_61CHN16;
            }
            break;

        case 8: // 7.1
            switch (format.bitsPerSample) {
                case 16: return AL_FORMAT_71CHN16;
                case 32: return AL_FORMAT_71CHN32;
            }
            break;
    }

    LOG_ERROR("Audio", "不支持的音频格式: channels={}, bits={}",
              format.channels, format.bitsPerSample);
    return AL_FORMAT_STEREO16; // 默认返回
}

ALuint AudioDeviceOpenAL::GetOrCreateBuffer(const AudioClip& clip) {
    // 查找缓存
    auto it = m_bufferCache.find(&clip);
    if (it != m_bufferCache.end()) {
        return it->second;
    }

    // 创建新缓冲区
    ALuint bufferId;
    alGenBuffers(1, &bufferId);

    if (CheckOpenALError("CreateBuffer")) {
        return 0;
    }

    // 填充数据
    ALenum format = GetOpenALFormat(clip.format);
    alBufferData(bufferId, format, clip.data.data(),
                 static_cast<ALsizei>(clip.data.size()),
                 static_cast<ALsizei>(clip.format.sampleRate));

    if (CheckOpenALError("BufferData")) {
        alDeleteBuffers(1, &bufferId);
        return 0;
    }

    // 缓存
    m_bufferCache[&clip] = bufferId;

    // 更新内存统计
    m_stats.memoryUsage += clip.data.size();

    return bufferId;
}

void AudioDeviceOpenAL::UpdateVoiceState(Voice* voice) {
    if (!voice || !voice->isActive) {
        return;
    }

    ALint state;
    alGetSourcei(voice->sourceId, AL_SOURCE_STATE, &state);

    VoiceState oldState = voice->state;
    VoiceState newState = VoiceState::Stopped;

    switch (state) {
        case AL_PLAYING:
            newState = VoiceState::Playing;
            break;
        case AL_PAUSED:
            newState = VoiceState::Paused;
            break;
        case AL_STOPPED:
        case AL_INITIAL:
        default:
            newState = VoiceState::Stopped;
            break;
    }

    // 状态变化处理
    if (newState != oldState) {
        voice->state = newState;

        // 查找Voice ID
        AudioVoiceId voiceId = INVALID_VOICE_ID;
        for (auto& [id, v] : m_activeVoices) {
            if (v == voice) {
                voiceId = id;
                break;
            }
        }

        if (voiceId != INVALID_VOICE_ID) {
            if (oldState == VoiceState::Playing && newState == VoiceState::Stopped) {
                TriggerEvent(AudioEventType::VoiceStopped, voiceId);
                ReleaseVoice(voice);

                // 从活跃列表移除
                m_activeVoices.erase(voiceId);
            }
        }
    }
}

void AudioDeviceOpenAL::ProcessFinishedVoices() {
    std::vector<AudioVoiceId> toRemove;

    for (auto& [voiceId, voice] : m_activeVoices) {
        ALint state;
        alGetSourcei(voice->sourceId, AL_SOURCE_STATE, &state);

        if (state == AL_STOPPED && voice->state == VoiceState::Playing) {
            if (voice->isLooping) {
                // 循环播放
                alSourcePlay(voice->sourceId);
                TriggerEvent(AudioEventType::VoiceLooped, voiceId);
            } else {
                // 播放结束
                toRemove.push_back(voiceId);
            }
        }
    }

    // 移除已结束的音频源
    for (AudioVoiceId id : toRemove) {
        auto it = m_activeVoices.find(id);
        if (it != m_activeVoices.end()) {
            TriggerEvent(AudioEventType::VoiceStopped, id);
            ReleaseVoice(it->second);
            m_activeVoices.erase(it);
        }
    }
}

void AudioDeviceOpenAL::Apply3DAttributes(Voice* voice) {
    if (!voice || !voice->desc.is3D) {
        return;
    }

    const auto& spatial = voice->desc.spatial;

    // 位置
    alSource3f(voice->sourceId, AL_POSITION,
               spatial.position[0], spatial.position[1], spatial.position[2]);

    // 速度
    alSource3f(voice->sourceId, AL_VELOCITY,
               spatial.velocity[0], spatial.velocity[1], spatial.velocity[2]);

    // 方向
    if (spatial.coneInnerAngle < 360.0f) {
        alSource3f(voice->sourceId, AL_DIRECTION,
                   spatial.direction[0], spatial.direction[1], spatial.direction[2]);
    }

    // 距离衰减
    alSourcef(voice->sourceId, AL_REFERENCE_DISTANCE, spatial.minDistance);
    alSourcef(voice->sourceId, AL_MAX_DISTANCE, spatial.maxDistance);
    alSourcef(voice->sourceId, AL_ROLLOFF_FACTOR, spatial.rolloffFactor);

    // 锥形参数
    if (spatial.coneInnerAngle < 360.0f || spatial.coneOuterAngle < 360.0f) {
        alSourcef(voice->sourceId, AL_CONE_INNER_ANGLE, spatial.coneInnerAngle);
        alSourcef(voice->sourceId, AL_CONE_OUTER_ANGLE, spatial.coneOuterAngle);
        alSourcef(voice->sourceId, AL_CONE_OUTER_GAIN, spatial.coneOuterGain);
    }
}

ALenum AudioDeviceOpenAL::GetOpenALDistanceModel(DistanceModel model) const {
    switch (model) {
        case DistanceModel::None: return AL_NONE;
        case DistanceModel::Inverse: return AL_INVERSE_DISTANCE;
        case DistanceModel::InverseClamped: return AL_INVERSE_DISTANCE_CLAMPED;
        case DistanceModel::Linear: return AL_LINEAR_DISTANCE;
        case DistanceModel::LinearClamped: return AL_LINEAR_DISTANCE_CLAMPED;
        case DistanceModel::Exponential: return AL_EXPONENT_DISTANCE;
        case DistanceModel::ExponentialClamped: return AL_EXPONENT_DISTANCE_CLAMPED;
        default: return AL_INVERSE_DISTANCE_CLAMPED;
    }
}

bool AudioDeviceOpenAL::CheckOpenALError(const std::string& operation) {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOG_ERROR("Audio", "OpenAL错误 [{}]: {}", operation, GetOpenALErrorString(error));
        return true;
    }

    // 检查ALC错误
    error = alcGetError(m_device);
    if (error != ALC_NO_ERROR) {
        LOG_ERROR("Audio", "OpenAL ALC错误 [{}]: {}", operation, GetOpenALErrorString(error));
        return true;
    }

    return false;
}

std::string AudioDeviceOpenAL::GetOpenALErrorString(ALenum error) {
    switch (error) {
        case AL_NO_ERROR: return "无错误";
        case AL_INVALID_NAME: return "无效名称";
        case AL_INVALID_ENUM: return "无效枚举";
        case AL_INVALID_VALUE: return "无效值";
        case AL_INVALID_OPERATION: return "无效操作";
        case AL_OUT_OF_MEMORY: return "内存不足";
        default: return "未知错误";
    }
}

void AudioDeviceOpenAL::TriggerEvent(AudioEventType type, AudioVoiceId voiceId,
                                    const std::string& message) {
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