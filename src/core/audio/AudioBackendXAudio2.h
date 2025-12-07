#pragma once
#include "AudioBackend.h"
// 包含XAudio2头文件
#include <../Logger.h>
#include <mutex>
#include <wrl/client.h>
#include <xaudio2.h>
using namespace Microsoft::WRL;

#if defined(PlaySound)
#undef PlaySound
#endif
namespace Engine {
	namespace Audio {

		class AudioBackendXAudio2 : public AudioBackend, public IXAudio2VoiceCallback
		{
		public:
            const AudioBackendType backendType = AudioBackendType::XAudio2;
			AudioBackendXAudio2() : m_XAudio2(nullptr), m_MasteringVoice(nullptr),
				sourceVoice(nullptr), running(false) {}
			~AudioBackendXAudio2() { Shutdown(); }
			bool Initialize(const AudioFormat& format) override;
			void Shutdown() override;
			uint32_t PlaySound(const AudioClip& source, float volume, float pitch, bool loop) override;
            void StopSound(uint32_t instanceId) override;
            void PauseSound(uint32_t instanceId) override;
            void ResumeSound(uint32_t instanceId) override;

            void SetVolume(uint32_t instanceId, float volume) override;
            void SetPitch(uint32_t instanceId, float pitch) override;
            void SetMasterVolume(float volume) override;

            bool IsPlaying(uint32_t instanceId) override;
            void Update() override;
            // 实现 IXAudio2VoiceCallback 接口
            void __stdcall OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
            void __stdcall OnVoiceProcessingPassEnd() override;
            void __stdcall OnStreamEnd() override;
            void __stdcall OnBufferStart(void* pBufferContext) override;
            void __stdcall OnBufferEnd(void* pBufferContext) override;
            void __stdcall OnLoopEnd(void* pBufferContext) override;
            void __stdcall OnVoiceError(void* pBufferContext, HRESULT Error) override;

		private:
			ComPtr<IXAudio2> m_XAudio2;
			IXAudio2MasteringVoice* m_MasteringVoice;
			IXAudio2SourceVoice* sourceVoice;

			std::vector<float> audioBuffer;
			std::mutex bufferMutex;
			bool running;

			static const int SAMPLE_RATE = 44100;
			static const int CHANNELS = 2;
			static const int BUFFER_SIZE = 4096;

            void AudioGenerationThread() {
                LOG_INFO("XAudio2", "已启动声音生成线程");
                const int CHUNK_SIZE = 512; // 每次提交的样本数（立体声）

                while (running) {
                    // 检查是否有足够的数据
                    std::vector<float> chunk;
                    {
                        std::lock_guard<std::mutex> lock(bufferMutex);
                        if (audioBuffer.size() >= CHUNK_SIZE) {
                            chunk.assign(audioBuffer.begin(),
                                audioBuffer.begin() + CHUNK_SIZE);
                            audioBuffer.erase(audioBuffer.begin(),
                                audioBuffer.begin() + CHUNK_SIZE);
                        }
                    }

                    if (!chunk.empty()) {
                        // 提交音频数据到 XAudio2
                        XAUDIO2_BUFFER buffer = { 0 };
                        buffer.AudioBytes = static_cast<UINT32>(chunk.size()) * static_cast<UINT32>(sizeof(float));
                        buffer.pAudioData = reinterpret_cast<BYTE*>(chunk.data());
                        buffer.Flags = XAUDIO2_END_OF_STREAM;

                        sourceVoice->SubmitSourceBuffer(&buffer);
                    }
                    else {
                        // 没有数据时休眠
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }

                    // 检查处理状态
                    XAUDIO2_VOICE_STATE state;
                    sourceVoice->GetState(&state);

                    // 如果缓冲区太多，等待一下
                    if (state.BuffersQueued > 8) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }
		};

	}
}
