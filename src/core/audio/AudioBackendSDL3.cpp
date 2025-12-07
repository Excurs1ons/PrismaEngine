#include "AudioBackendSDL3.h"
#include "../Logger.h"
namespace Engine {
    namespace Audio {
        AudioBackendSDL3::AudioBackendSDL3():m_deviceId(0),m_audioSpec({})
        {
        }

        AudioBackendSDL3::~AudioBackendSDL3()
        {
        }

        bool AudioBackendSDL3::Initialize(const AudioFormat& format)
        {
            SDL_AudioStream* audioStream;
            SDL_AudioSpec deviceSpec;
            // 初始化SDL音频子系统
            if (!SDL_Init(SDL_INIT_AUDIO)) {
                LOG_FATAL("System", "SDL_AUDIO无法初始化: {0}", SDL_GetError());
                return false;
            }
            else {
                LOG_INFO("System", "音频子系统初始化成功");
            }

            // 配置音频规格
            SDL_AudioSpec spec;
            SDL_zero(spec);
            spec.freq = 44100;
            spec.format = SDL_AUDIO_F32;
            spec.channels = 2;

            m_deviceId = SDL_OpenAudioDevice(0, &spec);
            if (!m_deviceId) {
                LOG_ERROR("Audio", "无法打开音频设备: {0}", SDL_GetError());
                SDL_Quit();
                return false;
            }
            // 创建音频流
            audioStream = SDL_CreateAudioStream(&spec, &deviceSpec);
            if (!audioStream) {
                LOG_ERROR("Audio", "无法创建音频流: ", SDL_GetError());
                return false;
            }


            return false;
        }

        void AudioBackendSDL3::Shutdown()
        {

            // 停止播放并清理
            SDL_CloseAudioDevice(m_deviceId);
        }

        uint32_t AudioBackendSDL3::PlaySound(const AudioClip& source, float volume, float pitch, bool loop)
        {
            return 0;
        }

        void AudioBackendSDL3::StopSound(uint32_t instanceId)
        {
        }

        void AudioBackendSDL3::PauseSound(uint32_t instanceId)
        {
        }

        void AudioBackendSDL3::ResumeSound(uint32_t instanceId)
        {
        }

        void AudioBackendSDL3::SetVolume(uint32_t instanceId, float volume)
        {
        }

        void AudioBackendSDL3::SetPitch(uint32_t instanceId, float pitch)
        {
        }

        void AudioBackendSDL3::SetMasterVolume(float volume)
        {
        }

        bool AudioBackendSDL3::IsPlaying(uint32_t instanceId)
        {
            return false;
        }

        void AudioBackendSDL3::Update()
        {
        }
    }
}
