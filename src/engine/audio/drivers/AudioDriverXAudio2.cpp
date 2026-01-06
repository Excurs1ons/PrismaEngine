#include "AudioDriverXAudio2.h"
#include <cassert>

namespace PrismaEngine::Audio {

// ========== XAudio2 回调类 ==========

class XAudio2Callback : public IXAudio2VoiceCallback {
public:
    XAudio2Callback(AudioDriverXAudio2* driver, SourceId sourceId)
        : m_driver(driver), m_sourceId(sourceId) {}

    STDMETHOD_(void, OnVoiceProcessingPassStart)(THIS_ UINT32 bytesRequired) override {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() override {}
    STDMETHOD_(void, OnStreamEnd)() override {}
    STDMETHOD_(void, OnBufferStart)(THIS_ void* pBufferContext) override {}
    STDMETHOD_(void, OnBufferEnd)(THIS_ void* pBufferContext) override {
        if (m_driver) {
            m_driver->OnBufferEnd(m_sourceId);
        }
    }
    STDMETHOD_(void, OnLoopEnd)(THIS_ void* pBufferContext) override {}
    STDMETHOD_(void, OnVoiceError)(THIS_ void* pBufferContext, HRESULT error) override {}

private:
    AudioDriverXAudio2* m_driver;
    SourceId m_sourceId;
};

// 每个源的回调实例
static thread_local std::unordered_map<SourceId, XAudio2Callback*> s_callbacks;

// ========== AudioDriverXAudio2 ==========

AudioDriverXAudio2::AudioDriverXAudio2() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

AudioDriverXAudio2::~AudioDriverXAudio2() {
    Shutdown();
    CoUninitialize();
}

AudioFormat AudioDriverXAudio2::Initialize(const AudioFormat& format) {
    if (m_initialized.load()) {
        return m_format;
    }

    if (!CreateXAudio2()) {
        return AudioFormat();  // 返回默认格式表示失败
    }

    if (!CreateMasteringVoice()) {
        ReleaseResources();
        return AudioFormat();
    }

    // 保存格式（XAudio2 会处理格式转换）
    m_format = AudioFormat(
        format.sampleRate > 0 ? format.sampleRate : SAMPLE_RATE,
        format.channels > 0 ? format.channels : CHANNELS,
        format.bitsPerSample > 0 ? format.bitsPerSample : BITS_PER_SAMPLE
    );

    m_initialized.store(true);
    return m_format;
}

void AudioDriverXAudio2::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 销毁所有音频源
    for (auto& [id, source] : m_sources) {
        if (source.voice) {
            source.voice->DestroyVoice();
            source.voice = nullptr;
        }
    }
    m_sources.clear();

    ReleaseResources();

    m_initialized.store(false);
}

bool AudioDriverXAudio2::IsInitialized() const {
    return m_initialized.load();
}

bool AudioDriverXAudio2::CreateXAudio2() {
    #ifdef _WIN32_WINNT_WIN10
        // Windows 10+ 使用 XAudio2.9
        HRESULT hr = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    #else
        // 旧版本使用 XAudio2.7
        HRESULT hr = CoCreateInstance(
            __uuidof(XAudio2),
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IXAudio2),
            reinterpret_cast<void**>(&m_xaudio2)
        );
        if (SUCCEEDED(hr)) {
            m_xaudio2->StartEngine();
        }
    #endif

    return SUCCEEDED(hr);
}

bool AudioDriverXAudio2::CreateMasteringVoice() {
    HRESULT hr = m_xaudio2->CreateMasteringVoice(
        &m_masteringVoice,
        m_format.channels,
        m_format.sampleRate
    );
    return SUCCEEDED(hr);
}

void AudioDriverXAudio2::ReleaseResources() {
    if (m_masteringVoice) {
        m_masteringVoice->DestroyVoice();
        m_masteringVoice = nullptr;
    }

    if (m_xaudio2) {
        #ifndef _WIN32_WINNT_WIN10
            m_xaudio2->StopEngine();
        #endif
        m_xaudio2->Release();
        m_xaudio2 = nullptr;
    }

    // 清理回调
    for (auto& [id, callback] : s_callbacks) {
        delete callback;
    }
    s_callbacks.clear();
}

// ========== 音频源管理 ==========

IAudioDriver::SourceId AudioDriverXAudio2::CreateSource() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized.load()) {
        return InvalidSource;
    }

    SourceId sourceId = GenerateSourceId();

    XAudioSource source;
    ZeroMemory(&source, sizeof(source));

    // 创建源语音
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = m_format.channels;
    wfx.nSamplesPerSec = m_format.sampleRate;
    wfx.wBitsPerSample = m_format.bitsPerSample;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    HRESULT hr = m_xaudio2->CreateSourceVoice(
        &source.voice,
        &wfx,
        0,
        XAUDIO2_DEFAULT_FREQ_RATIO,
        new XAudio2Callback(this, sourceId)
    );

    if (FAILED(hr)) {
        return InvalidSource;
    }

    m_sources[sourceId] = std::move(source);
    m_activeSourceCount.fetch_add(1);

    return sourceId;
}

void AudioDriverXAudio2::DestroySource(SourceId sourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sources.find(sourceId);
    if (it == m_sources.end()) {
        return;
    }

    if (it->second.voice) {
        it->second.voice->DestroyVoice();
    }

    m_sources.erase(it);
    m_activeSourceCount.fetch_sub(1);
}

bool AudioDriverXAudio2::IsSourceValid(SourceId sourceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sources.find(sourceId) != m_sources.end();
}

AudioDriverXAudio2::XAudioSource* AudioDriverXAudio2::GetSource(SourceId sourceId) {
    auto it = m_sources.find(sourceId);
    return (it != m_sources.end()) ? &it->second : nullptr;
}

const AudioDriverXAudio2::XAudioSource* AudioDriverXAudio2::GetSource(SourceId sourceId) const {
    auto it = m_sources.find(sourceId);
    return (it != m_sources.end()) ? &it->second : nullptr;
}

IAudioDriver::SourceId AudioDriverXAudio2::GenerateSourceId() {
    SourceId id;
    do {
        id = m_nextSourceId.fetch_add(1);
        if (id == InvalidSource) {
            id = m_nextSourceId.fetch_add(1);
        }
    } while (m_sources.find(id) != m_sources.end());
    return id;
}

// ========== 播放控制 ==========

bool AudioDriverXAudio2::QueueBuffer(SourceId sourceId, const AudioBuffer& buffer) {
    std::lock_guard<std::mutex> lock(m_mutex);

    XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice) {
        return false;
    }

    if (source->queuedBuffers >= MAX_BUFFERS) {
        return false;  // 缓冲区满
    }

    // 保存音频数据
    source->audioData.assign(buffer.data, buffer.data + buffer.size);

    // 填充 XAudio2 缓冲区
    XAUDIO2_BUFFER& xbuf = source->buffers[source->bufferIndex];
    ZeroMemory(&xbuf, sizeof(xbuf));
    xbuf.AudioBytes = static_cast<UINT32>(buffer.size);
    xbuf.pAudioData = source->audioData.data();
    xbuf.pContext = reinterpret_cast<void*>(sourceId);

    // 计算时长
    source->duration = static_cast<float>(buffer.frames) / m_format.sampleRate;

    HRESULT hr = source->voice->SubmitSourceBuffer(&xbuf);
    if (SUCCEEDED(hr)) {
        source->queuedBuffers++;
        source->bufferIndex = (source->bufferIndex + 1) % MAX_BUFFERS;
        return true;
    }

    return false;
}

bool AudioDriverXAudio2::Play(SourceId sourceId, bool loop) {
    std::lock_guard<std::mutex> lock(m_mutex);

    XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice) {
        return false;
    }

    source->looping = loop;

    HRESULT hr = source->voice->Start();
    return SUCCEEDED(hr);
}

void AudioDriverXAudio2::Stop(SourceId sourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice) {
        return;
    }

    source->voice->Stop();
    source->voice->FlushSourceBuffers();
    source->queuedBuffers = 0;
    source->bufferIndex = 0;
}

void AudioDriverXAudio2::Pause(SourceId sourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice) {
        return;
    }

    source->voice->Stop();
}

void AudioDriverXAudio2::Resume(SourceId sourceId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice) {
        return;
    }

    source->voice->Start();
}

IAudioDriver::SourceState AudioDriverXAudio2::GetState(SourceId sourceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    const XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice) {
        return SourceState::Stopped;
    }

    XAUDIO2_VOICE_STATE state;
    source->voice->GetState(&state);

    if (state.buffersQueued > 0) {
        XAUDIO2_VOICE_STATE vs;
        source->voice->GetState(&vs);
        return (vs.state == XAUDIO2_VOICE_STATE::Stopped) ? SourceState::Stopped : SourceState::Playing;
    }

    return SourceState::Stopped;
}

// ========== 实时控制 ==========

void AudioDriverXAudio2::SetVolume(SourceId sourceId, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice) {
        return;
    }

    source->voice->SetVolume(volume);
}

void AudioDriverXAudio2::SetPosition(SourceId sourceId, float seconds) {
    std::lock_guard<std::mutex> lock(m_mutex);

    XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice || source->duration <= 0.0f) {
        return;
    }

    // XAudio2 不支持直接设置播放位置，需要重新提交缓冲区
    // 这是一个简化实现，实际应该保存原始数据并重新计算偏移
    float ratio = seconds / source->duration;
    source->voice->SetFrequencyRatio(ratio);
}

float AudioDriverXAudio2::GetPosition(SourceId sourceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    const XAudioSource* source = GetSource(sourceId);
    if (!source || !source->voice || source->duration <= 0.0f) {
        return 0.0f;
    }

    XAUDIO2_VOICE_STATE state;
    source->voice->GetState(&state);

    float position = static_cast<float>(state.samplesPlayed) / m_format.sampleRate;
    return position;
}

// ========== 全局控制 ==========

void AudioDriverXAudio2::SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_masterVolume = volume;

    if (m_masteringVoice) {
        m_masteringVoice->SetVolume(volume);
    }
}

// ========== 回调 ==========

void AudioDriverXAudio2::SetBufferEndCallback(BufferEndCallback callback, void* userData) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bufferEndCallback = callback;
    m_callbackUserData = userData;
}

void AudioDriverXAudio2::OnBufferEnd(SourceId sourceId) {
    XAudioSource* source = GetSource(sourceId);
    if (source) {
        source->queuedBuffers--;

        // 循环播放
        if (source->looping && source->queuedBuffers == 0) {
            // 重新提交缓冲区（简化实现）
        }
    }

    if (m_bufferEndCallback) {
        m_bufferEndCallback(sourceId, m_callbackUserData);
    }
}

} // namespace PrismaEngine::Audio
