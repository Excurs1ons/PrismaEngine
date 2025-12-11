#pragma once
#include "ISubSystem.h"
#include <string>
#include <vector>

namespace Engine {
	namespace Audio {

		// 音频后端类型
		enum class AudioBackendType {
			None,
			SDL3,
			XAudio2,
		};

		// WAV文件头结构
		struct WAVHeader {
			char riff[4];           // "RIFF"
			unsigned int fileSize;  // 文件大小
			char wave[4];           // "WAVE"
			char fmt[4];            // "fmt "
			unsigned int fmtSize;   // fmt块大小
			unsigned short format;  // 音频格式
			unsigned short channels;// 声道数
			unsigned int sampleRate;// 采样率
			unsigned int byteRate;  // 字节率
			unsigned short blockAlign;// 块对齐
			unsigned short bitsPerSample;// 位深度
			char data[4];           // "data"
			unsigned int dataSize;  // 数据块大小
		};

		// 音频格式
		struct AudioFormat {
			int sampleRate = 44100;
			int channels = 2;
			int bitsPerSample = 16;
		};

		// 音频源数据
		struct AudioClip {
			std::vector<uint8_t> data;
			AudioFormat format;
			float duration = 0.0f;
			std::string path;
		};

		// 音频实例(正在播放的声音)
		struct AudioInstance {
			uint32_t id;
			std::string sourceId;
			float volume = 1.0f;
			float pitch = 1.0f;
			bool looping = false;
			bool paused = false;
		};


		// 音频后端接口
		class AudioBackend {
		public:
			// const 成员变量没有在构造函数中初始化，会导致默认构造函数被隐式删除。
			//const AudioBackend backendType;
            AudioBackend() : m_BackendType(AudioBackendType::None) {}
			virtual ~AudioBackend() = default;
			virtual bool Initialize(const AudioFormat& format) = 0;
			virtual void Shutdown() = 0;
			virtual uint32_t PlaySoundOnce(const AudioClip& source, float volume, float pitch, bool loop) = 0;
			virtual void StopSound(uint32_t instanceId) = 0;
			virtual void PauseSound(uint32_t instanceId) = 0;
			virtual void ResumeSound(uint32_t instanceId) = 0;

			virtual void SetVolume(uint32_t instanceId, float volume) = 0;
			virtual void SetPitch(uint32_t instanceId, float pitch) = 0;
			virtual void SetMasterVolume(float volume) = 0;

			virtual bool IsPlaying(uint32_t instanceId) = 0;
			virtual void Update() = 0;
		protected:
            AudioBackendType m_BackendType = AudioBackendType::None;
		};
	}
}
