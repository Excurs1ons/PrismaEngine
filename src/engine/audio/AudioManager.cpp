#include "AudioManager.h"
#include "Logger.h"
#include <filesystem>
#include <algorithm>

// 使用OpenAL作为音频后端
#ifdef ENABLE_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif

namespace PrismaEngine {
namespace Audio {

AudioManager& AudioManager::GetInstance() {
    static AudioManager instance;
    return instance;
}

AudioManager::~AudioManager() {
    Shutdown();
}

bool AudioManager::Initialize(int maxSources) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        return true;
    }

    LOG_INFO("AudioManager", "初始化音频系统");

#ifdef ENABLE_OPENAL
    // 打开音频设备
    m_audioDevice = alcOpenDevice(nullptr);
    if (!m_audioDevice) {
        LOG_ERROR("AudioManager", "无法打开音频设备");
        return false;
    }

    // 创建音频上下文
    m_audioContext = alcCreateContext(m_audioDevice, nullptr);
    if (!m_audioContext) {
        LOG_ERROR("AudioManager", "无法创建音频上下文");
        alcCloseDevice(m_audioDevice);
        return false;
    }

    // 设置当前上下文
    if (!alcMakeContextCurrent(m_audioContext)) {
        LOG_ERROR("AudioManager", "无法设置音频上下文");
        alcDestroyContext(m_audioContext);
        alcCloseDevice(m_audioDevice);
        return false;
    }

    // 生成音频源
    m_sources.reserve(maxSources);
    m_availableSources.reserve(maxSources);

    ALuint* sourceIds = new ALuint[maxSources];
    alGenSources(maxSources, sourceIds);

    if (alGetError() != AL_NO_ERROR) {
        LOG_ERROR("AudioManager", "无法生成音频源");
        delete[] sourceIds;
        return false;
    }

    // 创建AudioSource包装
    for (int i = 0; i < maxSources; ++i) {
        auto source = std::make_shared<AudioSource>();
        source->m_sourceHandle = reinterpret_cast<void*>(sourceIds[i]);
        m_sources.push_back(source);
        m_availableSources.push_back(source);
    }

    delete[] sourceIds;

    // 设置监听器默认位置
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    ALfloat orientation[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
    alListenerfv(AL_ORIENTATION, orientation);

    LOG_INFO("AudioManager", "音频系统初始化成功，最大音频源: {0}", maxSources);
#else
    LOG_WARNING("AudioManager", "OpenAL未启用，音频系统将使用空实现");
#endif

    m_initialized = true;
    return true;
}

void AudioManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    LOG_INFO("AudioManager", "关闭音频系统");

    // 停止所有音频源
    for (auto& source : m_sources) {
        if (source) {
            source->Stop();
        }
    }

#ifdef ENABLE_OPENAL
    if (m_audioContext) {
        // 删除所有音频源
        ALuint* sourceIds = new ALuint[m_sources.size()];
        for (size_t i = 0; i < m_sources.size(); ++i) {
            sourceIds[i] = reinterpret_cast<ALuint>(m_sources[i]->m_sourceHandle);
        }
        alDeleteSources(static_cast<ALsizei>(m_sources.size()), sourceIds);
        delete[] sourceIds;

        // 清理上下文
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_audioContext);
        m_audioContext = nullptr;

        // 关闭设备
        if (m_audioDevice) {
            alcCloseDevice(m_audioDevice);
            m_audioDevice = nullptr;
        }
    }
#endif

    // 清理资源
    m_sources.clear();
    m_availableSources.clear();
    m_audioCache.clear();

    m_initialized = false;
    LOG_INFO("AudioManager", "音频系统已关闭");
}

std::shared_ptr<const AudioData> AudioManager::LoadAudio(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查缓存
    auto it = m_audioCache.find(filePath);
    if (it != m_audioCache.end()) {
        return it->second;
    }

    // 加载音频文件
    std::shared_ptr<AudioData> audioData = nullptr;
    std::string ext = std::filesystem::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".wav") {
        audioData = LoadWAV(filePath);
    } else if (ext == ".ogg") {
        audioData = LoadOGG(filePath);
    } else if (ext == ".mp3") {
        audioData = LoadMP3(filePath);
    }

    if (audioData) {
        m_audioCache[filePath] = audioData;
        LOG_INFO("AudioManager", "成功加载音频: {0}", filePath);
    } else {
        LOG_ERROR("AudioManager", "加载音频失败: {0}", filePath);
    }

    return audioData;
}

void AudioManager::UnloadAudio(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_audioCache.find(filePath);
    if (it != m_audioCache.end()) {
        m_audioCache.erase(it);
        LOG_DEBUG("AudioManager", "卸载音频: {0}", filePath);
    }
}

void AudioManager::UnloadAllAudio() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_audioCache.clear();
    LOG_INFO("AudioManager", "已卸载所有音频资源");
}

std::shared_ptr<AudioSource> AudioManager::CreateSource() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        LOG_ERROR("AudioManager", "音频系统未初始化");
        return nullptr;
    }

    if (m_availableSources.empty()) {
        LOG_WARNING("AudioManager", "无可用音频源");
        return nullptr;
    }

    auto source = m_availableSources.back();
    m_availableSources.pop_back();

    return source;
}

void AudioManager::DestroySource(std::shared_ptr<AudioSource> source) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!source) {
        return;
    }

    // 停止音频
    source->Stop();

    // 归还到可用源池
    m_availableSources.push_back(source);
}

std::shared_ptr<AudioSource> AudioManager::PlayAudio(const std::string& filePath,
                                                   float volume,
                                                   bool loop) {
    auto audioData = LoadAudio(filePath);
    if (!audioData) {
        return nullptr;
    }

    auto source = CreateSource();
    if (!source) {
        return nullptr;
    }

    source->SetAudioData(audioData);
    source->SetVolume(volume);
    source->SetLoop(loop);
    source->Play();

    return source;
}

std::shared_ptr<AudioSource> AudioManager::PlayAudio3D(const std::string& filePath,
                                                      const DirectX::XMFLOAT3& position,
                                                      float volume,
                                                      bool loop) {
    auto source = PlayAudio(filePath, volume, loop);
    if (source) {
        source->Set3DPosition(position);
    }

    return source;
}

void AudioManager::SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);

#ifdef ENABLE_OPENAL
    alListenerf(AL_GAIN, m_masterVolume);
#endif
}

std::vector<AudioManager::DeviceInfo> AudioManager::GetAvailableDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<DeviceInfo> devices;

#ifdef ENABLE_OPENAL
    const ALCchar* deviceList = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
    if (deviceList) {
        const ALCchar* device = deviceList;
        while (*device && strlen(device) > 0) {
            DeviceInfo info;
            info.name = device;
            info.isDefault = false;
            devices.push_back(info);

            device += strlen(device) + 1;
        }
    }
#endif

    return devices;
}

bool AudioManager::SetDevice(const std::string& deviceName) {
    // TODO: 实现设备切换
    LOG_WARNING("AudioManager", "设备切换功能尚未实现: {0}", deviceName);
    return false;
}

void AudioManager::Update() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

#ifdef ENABLE_OPENAL
    // 更新监听器位置
    alListener3f(AL_POSITION, m_listener.GetPosition().x,
                              m_listener.GetPosition().y,
                              m_listener.GetPosition().z);
    alListener3f(AL_VELOCITY, m_listener.GetVelocity().x,
                              m_listener.GetVelocity().y,
                              m_listener.GetVelocity().z);

    ALfloat orientation[] = {
        m_listener.GetForward().x, m_listener.GetForward().y, m_listener.GetForward().z,
        m_listener.GetUp().x, m_listener.GetUp().y, m_listener.GetUp().z
    };
    alListenerfv(AL_ORIENTATION, orientation);

    // 更新统计
    m_stats.activeSources = static_cast<uint32_t>(m_sources.size());
    m_stats.playingSources = 0;

    for (const auto& source : m_sources) {
        if (source && source->GetState() == AudioState::Playing) {
            m_stats.playingSources++;
        }
    }
#endif

    m_stats.loadedAudioFiles = static_cast<uint32_t>(m_audioCache.size());
}

void AudioManager::ApplySettings(const AudioSettings& settings) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_settings = settings;

    // TODO: 应用音频设置
    LOG_INFO("AudioManager", "应用音频设置");
}

AudioManager::AudioStats AudioManager::GetStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    AudioStats stats = m_stats;
    stats.memoryUsage = 0;

    for (const auto& pair : m_audioCache) {
        if (pair.second) {
            stats.memoryUsage += pair.second->size;
        }
    }

    return stats;
}

std::shared_ptr<AudioData> AudioManager::LoadWAV(const std::string& filePath) {
    // TODO: 实现WAV加载
    LOG_WARNING("AudioManager", "WAV加载功能尚未实现: {0}", filePath);
    return nullptr;
}

std::shared_ptr<AudioData> AudioManager::LoadOGG(const std::string& filePath) {
    // TODO: 实现OGG加载（需要stb_vorbis或libvorbis）
    LOG_WARNING("AudioManager", "OGG加载功能尚未实现: {0}", filePath);
    return nullptr;
}

std::shared_ptr<AudioData> AudioManager::LoadMP3(const std::string& filePath) {
    // TODO: 实现MP3加载（需要mpg123或类似库）
    LOG_WARNING("AudioManager", "MP3加载功能尚未实现: {0}", filePath);
    return nullptr;
}

bool AudioManager::DecodeAudio(const std::string& filePath, AudioData& outData) {
    // TODO: 通用音频解码
    return false;
}

// AudioSource 实现
AudioSource::AudioSource() {
#ifdef ENABLE_OPENAL
    // 创建缓冲区
    ALuint bufferId;
    alGenBuffers(1, &bufferId);
    m_bufferHandle = reinterpret_cast<void*>(bufferId);
#endif
}

AudioSource::~AudioSource() {
    Stop();

#ifdef ENABLE_OPENAL
    if (m_bufferHandle) {
        ALuint bufferId = reinterpret_cast<ALuint>(m_bufferHandle);
        alDeleteBuffers(1, &bufferId);
    }
#endif
}

void AudioSource::Play() {
    if (!m_audioData || m_state == AudioState::Playing) {
        return;
    }

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourcePlay(sourceId);
    m_state = AudioState::Playing;
#endif
}

void AudioSource::Pause() {
    if (m_state != AudioState::Playing) {
        return;
    }

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourcePause(sourceId);
    m_state = AudioState::Paused;
#endif
}

void AudioSource::Stop() {
    if (m_state == AudioState::Stopped) {
        return;
    }

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourceStop(sourceId);
    m_state = AudioState::Stopped;
    m_currentSample = 0;
    m_playbackTime = 0.0f;
#endif
}

void AudioSource::Resume() {
    if (m_state != AudioState::Paused) {
        return;
    }

    Play();
}

void AudioSource::SetVolume(float volume) {
    m_volume = std::clamp(volume, 0.0f, 1.0f);

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourcef(sourceId, AL_GAIN, m_volume);
#endif
}

void AudioSource::SetPitch(float pitch) {
    m_pitch = std::clamp(pitch, 0.5f, 2.0f);

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourcef(sourceId, AL_PITCH, m_pitch);
#endif
}

void AudioSource::SetPan(float pan) {
    m_pan = std::clamp(pan, -1.0f, 1.0f);

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSource3f(sourceId, AL_POSITION, m_pan, 0.0f, 0.0f);
#endif
}

void AudioSource::SetLoop(bool loop) {
    m_loop = loop;

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourcei(sourceId, AL_LOOPING, m_loop ? AL_TRUE : AL_FALSE);
#endif
}

void AudioSource::Set3DPosition(const DirectX::XMFLOAT3& position) {
    m_position = position;

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSource3f(sourceId, AL_POSITION, position.x, position.y, position.z);
#endif
}

void AudioSource::Set3DVelocity(const DirectX::XMFLOAT3& velocity) {
    m_velocity = velocity;

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSource3f(sourceId, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
#endif
}

void AudioSource::Set3DDistance(float minDistance, float maxDistance) {
    m_minDistance = minDistance;
    m_maxDistance = maxDistance;

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourcef(sourceId, AL_REFERENCE_DISTANCE, minDistance);
    alSourcef(sourceId, AL_MAX_DISTANCE, maxDistance);
#endif
}

void AudioSource::Set3DCone(float innerAngle, float outerAngle, float outerVolume) {
    m_innerConeAngle = innerAngle;
    m_outerConeAngle = outerAngle;
    m_outerConeVolume = outerVolume;

#ifdef ENABLE_OPENAL
    ALuint sourceId = reinterpret_cast<ALuint>(m_sourceHandle);
    alSourcef(sourceId, AL_CONE_INNER_ANGLE, DirectX::XMConvertToDegrees(innerAngle));
    alSourcef(sourceId, AL_CONE_OUTER_ANGLE, DirectX::XMConvertToDegrees(outerAngle));
    alSourcef(sourceId, AL_CONE_OUTER_GAIN, outerVolume);
#endif
}

float AudioSource::GetPlaybackTime() const {
    // TODO: 实现获取播放进度
    return m_playbackTime;
}

void AudioSource::SetPlaybackTime(float time) {
    m_playbackTime = std::clamp(time, 0.0f, m_audioData ? m_audioData->duration : 0.0f);
    // TODO: 实现设置播放进度
}

void AudioSource::SetAudioData(std::shared_ptr<const AudioData> data) {
    m_audioData = data;

    if (m_audioData) {
        // TODO: 将音频数据上传到OpenAL缓冲区
    }
}

// AudioListener 实现
AudioListener::AudioListener() {
    // 默认值已在成员初始化中设置
}

void AudioListener::SetPosition(const DirectX::XMFLOAT3& position) {
    m_position = position;
}

void AudioListener::SetVelocity(const DirectX::XMFLOAT3& velocity) {
    m_velocity = velocity;
}

void AudioListener::SetOrientation(const DirectX::XMFLOAT3& forward, const DirectX::XMFLOAT3& up) {
    m_forward = forward;
    m_up = up;
}

} // namespace Audio
} // namespace Engine