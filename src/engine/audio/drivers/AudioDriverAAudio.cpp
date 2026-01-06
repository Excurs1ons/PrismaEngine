#include "AudioDriverAAudio.h"
#include <cmath>
#include <algorithm>

namespace PrismaEngine::Audio {

// ========== AudioDriverAAudio ==========

AudioDriverAAudio::AudioDriverAAudio() {
    m_sources.resize(MAX_SOURCES, nullptr);
}

AudioDriverAAudio::~AudioDriverAAudio() {
    Shutdown();
}

AudioFormat AudioDriverAAudio::Initialize(const AudioFormat& format) {
    if (m_initialized.load()) {
        return m_format;
    }

    // 设置音频格式
    m_format = AudioFormat(
        format.sampleRate > 0 ? format.sampleRate : SAMPLE_RATE,
        format.channels > 0 ? format.channels : CHANNELS,
        16  // AAudio 固定 16 位输出
    );

    if (!BuildAudioStream()) {
        return AudioFormat();
    }

    // 分配混音缓冲区
    m_mixBuffer.resize(FRAME_COUNT * m_format.channels);
    m_outputBuffer.resize(FRAME_COUNT * m_format.channels);

    m_initialized.store(true);
    m_running.store(true);

    return m_format;
}

void AudioDriverAAudio::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    m_running.store(false);

    std::lock_guard<std::mutex> lock(m_mutex);

    // 停止并关闭流
    if (m_stream) {
        AAudioStream_requestStop(m_stream);
        AAudioStream_close(m_stream);
        m_stream = nullptr;
    }

    // 释放所有音频源
    for (auto* source : m_sources) {
        delete source;
    }
    m_sources.clear();
    std::fill_n(m_sourceUsed, MAX_SOURCES, false);

    m_initialized.store(false);
}

bool AudioDriverAAudio::IsInitialized() const {
    return m_initialized.load();
}

bool AudioDriverAAudio::BuildAudioStream() {
    AAudioStreamBuilder* builder = nullptr;

    // 创建流构建器
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        return false;
    }

    // 配置流
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSampleRate(builder, m_format.sampleRate);
    AAudioStreamBuilder_setChannelCount(builder, m_format.channels);
    AAudioStreamBuilder_setFramesPerDataCallback(builder, FRAME_COUNT);
    AAudioStreamBuilder_setDataCallback(builder, DataCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, ErrorCallback, this);

    // 性能模式：低延迟
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);

    // 共享模式（允许混音）
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);

    // 打开流
    result = AAudioStreamBuilder_openStream(builder, &m_stream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        return false;
    }

    // 获取实际格式
    m_format.sampleRate = AAudioStream_getSampleRate(m_stream);
    m_format.channels = AAudioStream_getChannelCount(m_stream);

    // 启动流
    result = AAudioStream_requestStart(m_stream);
    return result == AAUDIO_OK;
}

void AudioDriverAAudio::ReleaseResources() {
    if (m_stream) {
        AAudioStream_requestStop(m_stream);
        AAudioStream_close(m_stream);
        m_stream = nullptr;
    }

    for (auto* source : m_sources) {
        delete source;
    }
    m_sources.clear();
}

// ========== AAudio 回调 ==========

aaudio_data_callback_result_t AudioDriverAAudio::DataCallback(
    AAudioStream* stream,
    void* userData,
    void* audioData,
    int32_t numFrames) {

    auto* driver = static_cast<AudioDriverAAudio*>(userData);
    return driver->ProcessAudio(audioData, numFrames);
}

void AudioDriverAAudio::ErrorCallback(
    AAudioStream* stream,
    void* userData,
    aaudio_result_t error) {

    // 处理错误（如断开耳机）
    if (error == AAUDIO_ERROR_DISCONNECTED) {
        auto* driver = static_cast<AudioDriverAAudio*>(userData);
        // 尝试重新打开流
    }
}

aaudio_data_callback_result_t AudioDriverAAudio::ProcessAudio(
    void* audioData,
    int32_t numFrames) {

    if (!m_running.load()) {
        return AAUDIO_CALLBACK_RESULT_STOP;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 清空混音缓冲区
    std::fill(m_mixBuffer.begin(), m_mixBuffer.end(), 0.0f);

    // 混音所有活跃的音频源
    for (SourceId i = 0; i < MAX_SOURCES; ++i) {
        if (m_sourceUsed[i] && m_sources[i]) {
            AAudioSource& source = *m_sources[i];
            if (source.playing && !source.paused) {
                MixSource(m_mixBuffer.data(), numFrames, source);
            }
        }
    }

    // 应用主音量并转换为 16 位 PCM
    int16_t* output = static_cast<int16_t*>(audioData);
    for (size_t i = 0; i < m_mixBuffer.size(); ++i) {
        float sample = m_mixBuffer[i] * m_masterVolume;
        // 软限幅
        sample = std::clamp(sample, -1.0f, 1.0f);
        // 转换到 16 位
        output[i] = static_cast<int16_t>(sample * 32767.0f);
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AudioDriverAAudio::MixSource(float* output, int32_t numFrames, AAudioSource& source) {
    const size_t framesToMix = std::min(
        static_cast<size_t>(numFrames),
        (source.dataSize - source.readPos) / m_format.GetFrameSize()
    );

    if (framesToMix == 0) {
        if (source.looping && source.dataSize > 0) {
            source.readPos = 0;
        } else {
            source.playing = false;
            source.state = SourceState::Stopped;
            if (m_bufferEndCallback) {
                m_bufferEndCallback(reinterpret_cast<SourceId>(&source), m_callbackUserData);
            }
        }
        return;
    }

    // 读取 16 位 PCM 数据并混音
    const int16_t* input = reinterpret_cast<const int16_t*>(source.audioData.data() + source.readPos);
    const size_t samplesToMix = framesToMix * m_format.channels;

    for (size_t i = 0; i < samplesToMix; ++i) {
        float sample = static_cast<float>(input[i]) / 32768.0f;
        output[i] += sample * source.volume * m_masterVolume;
    }

    source.readPos += framesToMix * m_format.GetFrameSize();
}

// ========== 音频源管理 ==========

IAudioDriver::SourceId AudioDriverAAudio::CreateSource() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized.load()) {
        return InvalidSource;
    }

    // 查找空闲槽位
    for (SourceId i = 0; i < MAX_SOURCES; ++i) {
        if (!m_sourceUsed[i]) {
            m_sources[i] = new AAudioSource();
            m_sourceUsed[i] = true;
            m_activeSourceCount.fetch_add(1);
            return i + 1;  // ID 从 1 开始
        }
    }

    return InvalidSource;  // 没有空闲槽位
}

void AudioDriverAAudio::DestroySource(SourceId sourceId) {
    if (sourceId == InvalidSource || sourceId > MAX_SOURCES) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    size_t index = sourceId - 1;
    if (!m_sourceUsed[index]) {
        return;
    }

    delete m_sources[index];
    m_sources[index] = nullptr;
    m_sourceUsed[index] = false;
    m_activeSourceCount.fetch_sub(1);
}

bool AudioDriverAAudio::IsSourceValid(SourceId sourceId) const {
    if (sourceId == InvalidSource || sourceId > MAX_SOURCES) {
        return false;
    }
    return m_sourceUsed[sourceId - 1];
}

AudioDriverAAudio::AAudioSource* AudioDriverAAudio::GetSource(SourceId sourceId) {
    if (sourceId == InvalidSource || sourceId > MAX_SOURCES) {
        return nullptr;
    }
    return m_sources[sourceId - 1];
}

const AudioDriverAAudio::AAudioSource* AudioDriverAAudio::GetSource(SourceId sourceId) const {
    if (sourceId == InvalidSource || sourceId > MAX_SOURCES) {
        return nullptr;
    }
    return m_sources[sourceId - 1];
}

// ========== 播放控制 ==========

bool AudioDriverAAudio::QueueBuffer(SourceId sourceId, const AudioBuffer& buffer) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AAudioSource* source = GetSource(sourceId);
    if (!source) {
        return false;
    }

    // 复制音频数据
    source->audioData.assign(buffer.data, buffer.data + buffer.size);
    source->dataSize = buffer.size;
    source->readPos = 0;
    source->writePos = 0;

    return true;
}

bool AudioDriverAAudio::Play(SourceId sourceId, bool loop) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AAudioSource* source = GetSource(sourceId);
    if (!source || source->audioData.empty()) {
        return false;
    }

    source->playing = true;
    source->paused = false;
    source->looping = loop;
    source->state = SourceState::Playing;

    return true;
}

void AudioDriverAAudio::Stop(SourceId sourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AAudioSource* source = GetSource(sourceId);
    if (!source) {
        return;
    }

    source->playing = false;
    source->paused = false;
    source->readPos = 0;
    source->state = SourceState::Stopped;
}

void AudioDriverAAudio::Pause(SourceId sourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AAudioSource* source = GetSource(sourceId);
    if (!source) {
        return;
    }

    source->paused = true;
    source->state = SourceState::Paused;
}

void AudioDriverAAudio::Resume(SourceId sourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AAudioSource* source = GetSource(sourceId);
    if (!source) {
        return;
    }

    source->paused = false;
    if (source->playing) {
        source->state = SourceState::Playing;
    }
}

IAudioDriver::SourceState AudioDriverAAudio::GetState(SourceId sourceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    const AAudioSource* source = GetSource(sourceId);
    if (!source) {
        return SourceState::Stopped;
    }

    return source->state;
}

// ========== 实时控制 ==========

void AudioDriverAAudio::SetVolume(SourceId sourceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AAudioSource* source = GetSource(sourceId);
    if (!source) {
        return;
    }

    source->volume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioDriverAAudio::SetPosition(SourceId sourceId, float seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AAudioSource* source = GetSource(sourceId);
    if (!source || source->dataSize == 0) {
        return;
    }

    size_t bytePos = static_cast<size_t>(seconds * m_format.GetBytesPerSecond());
    bytePos = std::min(bytePos, source->dataSize);
    source->readPos = bytePos;
}

float AudioDriverAAudio::GetPosition(SourceId sourceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    const AAudioSource* source = GetSource(sourceId);
    if (!source || source->dataSize == 0) {
        return 0.0f;
    }

    return static_cast<float>(source->readPos) / m_format.GetBytesPerSecond();
}

// ========== 全局控制 ==========

void AudioDriverAAudio::SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

// ========== 回调 ==========

void AudioDriverAAudio::SetBufferEndCallback(BufferEndCallback callback, void* userData) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bufferEndCallback = callback;
    m_callbackUserData = userData;
}

} // namespace PrismaEngine::Audio
