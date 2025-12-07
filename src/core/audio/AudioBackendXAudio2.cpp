#include "AudioBackendXAudio2.h"
#include "Helper.h"
#include "Logger.h"
#include <iostream>
#ifdef PlaySound
#undef PlaySound
#endif
namespace Engine {
    namespace Audio {

        bool AudioBackendXAudio2::Initialize(const AudioFormat& format)
        {

            // 初始化XAudio2
            HRESULT hr = XAudio2Create(m_XAudio2.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
            if (FAILED(hr)) {
                LOG_FATAL("Audio", "XAudio2无法初始化: {0}", HrToString(hr));
                return false;
            }
            LOG_INFO("Audio", "XAudio2初始化成功");
            // 创建主声音
            hr = m_XAudio2->CreateMasteringVoice(&m_MasteringVoice);
            if (FAILED(hr)) {
                LOG_FATAL("Audio", "XAudio2无法创建主声音: {0}", HrToString(hr));
                return false;
            }
			LOG_INFO("Audio", "XAudio2主声音创建成功");
            return true;

            // 设置波形格式
            WAVEFORMATEX waveFormat;
            waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            waveFormat.nChannels = CHANNELS;
            waveFormat.nSamplesPerSec = SAMPLE_RATE;
            waveFormat.nAvgBytesPerSec = SAMPLE_RATE * CHANNELS * sizeof(float);
            waveFormat.nBlockAlign = CHANNELS * sizeof(float);
            waveFormat.wBitsPerSample = 32;
            waveFormat.cbSize = 0;

            // 创建源声音
            hr = m_XAudio2->CreateSourceVoice(&sourceVoice, &waveFormat, 0,
                XAUDIO2_DEFAULT_FREQ_RATIO,
                this, nullptr, nullptr);
            if (FAILED(hr)) {
                LOG_FATAL("Audio", "XAudio2无法创建源声音: {0}", HrToString(hr));
                return false;
			}
            LOG_INFO("Audio", "XAudio2创建源声音成功");
            // 启动源声音
            hr = sourceVoice->Start(0);
            if (FAILED(hr)) {
                LOG_FATAL("Audio", "XAudio2无法启动源声音");
                return false;
            }
            LOG_INFO("Audio", "XAudio2启动源声音成功");
            // 启动音频生成线程
            std::thread audioThread(&AudioBackendXAudio2::AudioGenerationThread, this);
            audioThread.detach();

            return true;

        }

        void AudioBackendXAudio2::Shutdown()
        {
            if (m_MasteringVoice) {
                m_MasteringVoice->DestroyVoice();
                m_MasteringVoice = nullptr;
            }
            m_XAudio2.Reset();
        }

        uint32_t AudioBackendXAudio2::PlaySound(const AudioClip& source, float volume, float pitch, bool loop)
        {

            // 打开WAV文件
            std::ifstream file(source.path, std::ios::binary);
            if (!file.is_open()) {
                LOG_ERROR("Audio", "无法打开音频文件: {0}", source.path);
                return false;
            }

            // 读取WAV文件头
            WAVHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

            // 检查文件格式
            if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
                std::cerr << "Invalid WAV file format: " << source.path << std::endl;
                return false;
            }

            // 读取音频数据
            std::vector<char> audioData(header.dataSize);
            file.read(audioData.data(), header.dataSize);
            file.close();

            // 创建音频源声音
            WAVEFORMATEX waveFormat = { 0 };
            waveFormat.wFormatTag = header.format;
            waveFormat.nChannels = header.channels;
            waveFormat.nSamplesPerSec = header.sampleRate;
            waveFormat.nAvgBytesPerSec = header.byteRate;
            waveFormat.nBlockAlign = header.blockAlign;
            waveFormat.wBitsPerSample = header.bitsPerSample;
            waveFormat.cbSize = 0;

            // 创建音频缓冲区
            XAUDIO2_BUFFER buffer = { 0 };
            buffer.AudioBytes = header.dataSize;
            buffer.pAudioData = reinterpret_cast<const BYTE*>(audioData.data());
            buffer.Flags = XAUDIO2_END_OF_STREAM;

            // 创建源声音
            IXAudio2SourceVoice* sourceVoice = nullptr;
            HRESULT hr = m_XAudio2->CreateSourceVoice(&sourceVoice, &waveFormat);
            if (FAILED(hr)) {
                LOG_ERROR("XAudio2", "创建音频源失败");
                return false;
            }

            // 提交音频缓冲区
            hr = sourceVoice->SubmitSourceBuffer(&buffer);
            if (FAILED(hr)) {
                LOG_ERROR("XAudio2", "提交音频缓冲区失败");
                sourceVoice->DestroyVoice();
                return false;
            }

            // 开始播放
            hr = sourceVoice->Start(0);
            if (FAILED(hr)) {
                std::cerr << "开始播放失败" << std::endl;
                sourceVoice->DestroyVoice();
                return false;
            }

            // 等待播放完成（实际项目中可能需要异步处理）
            XAUDIO2_VOICE_STATE state;
            do {
                sourceVoice->GetState(&state);
                Sleep(10);
            } while (state.BuffersQueued > 0);

            // 清理资源
            sourceVoice->DestroyVoice();

            return 0;
        }

        void AudioBackendXAudio2::StopSound(uint32_t instanceId)
        {
        }

        void AudioBackendXAudio2::PauseSound(uint32_t instanceId)
        {
        }

        void AudioBackendXAudio2::ResumeSound(uint32_t instanceId)
        {
        }

        void AudioBackendXAudio2::SetVolume(uint32_t instanceId, float volume)
        {
        }

        void AudioBackendXAudio2::SetPitch(uint32_t instanceId, float pitch)
        {
        }

        void AudioBackendXAudio2::SetMasterVolume(float volume)
        {
        }

        bool AudioBackendXAudio2::IsPlaying(uint32_t instanceId)
        {
            return false;
        }

        void AudioBackendXAudio2::Update()
        {
        }

        void __stdcall AudioBackendXAudio2::OnVoiceProcessingPassStart(UINT32 BytesRequired)
        {
        }
        void __stdcall AudioBackendXAudio2::OnVoiceProcessingPassEnd()
        {
        }
        void __stdcall AudioBackendXAudio2::OnStreamEnd()
        {
        }
        void __stdcall AudioBackendXAudio2::OnBufferStart(void* pBufferContext)
        {
        }
        void __stdcall AudioBackendXAudio2::OnBufferEnd(void* pBufferContext)
        {
        }
        void __stdcall AudioBackendXAudio2::OnLoopEnd(void* pBufferContext)
        {
        }
        void __stdcall AudioBackendXAudio2::OnVoiceError(void* pBufferContext, HRESULT Error)
        {
        }
    }
}
