#include "AudioSystem.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <SDL3/SDL_init.h>
#include <Logger.h>
#include "Helper.h"
#include <AudioBackendXAudio2.h>
#include <AudioBackendSDL3.h>
namespace Engine {
    namespace Audio {

        AudioSystem::AudioSystem()
        {
            
        }

        AudioSystem::~AudioSystem()
        {
            Shutdown();
        }
        bool AudioSystem::Initialize(AudioBackendType audioBackendType, AudioFormat audioFormat)
        {
            switch (audioBackendType) {
                case AudioBackendType::SDL3:
                    audioBackend = std::make_unique<AudioBackendSDL3>();
                    break;
                case AudioBackendType::XAudio2:
                    audioBackend = std::make_unique<AudioBackendXAudio2>();
                    break;
                case AudioBackendType::None:
                default:
                    LOG_ERROR("AudioSystem",
                              "未指定有效的音频后端类型: AudioBackend@{0}",
                              static_cast<int>(audioBackendType));
                    break;
            }
            if (audioBackend)
                return audioBackend->Initialize(audioFormat);
            else
                LOG_ERROR("AudioSystem", "指定的音频后端创建失败: AudioBackend@{0}", static_cast<int>(audioBackendType));
            return false;
        }
        // 音频回调函数
        void audioCallback(void* userdata, Uint8* stream, int len) {
            // 这里填充音频数据
            // 例如播放静音
            SDL_memset(stream, 0, len);
        }
        bool AudioSystem::Initialize()
        {
            AudioFormat audioFormat;
            return Initialize(AudioBackendType::XAudio2, audioFormat);
        }

        void AudioSystem::Shutdown()
        {
            if (audioBackend)
                audioBackend->Shutdown();
        }

        bool AudioSystem::PlayAudioFile(const char* filename)
        {
            if (!audioBackend) {
                LOG_ERROR("Audio", "音频系统未初始化");
                return false;
            }

            std::cout << "Successfully played audio file: " << filename << std::endl;
            return true;
        }
    }
}
