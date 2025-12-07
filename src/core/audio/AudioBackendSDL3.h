#pragma once
#include <SDL3/SDL.h>
#include <map>
#include <vector>

#include "AudioBackend.h"

namespace Engine {
    namespace Audio {

        class AudioBackendSDL3 : public AudioBackend {
        public:
            AudioBackendSDL3();
            ~AudioBackendSDL3() override;

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

        private:
            struct PlayingSound {
                SDL_AudioStream* stream;
                std::vector<uint8_t> audioData;
                float volume;
                float pitch;
                bool looping;
                bool paused;
                size_t currentPosition;
            };

            SDL_AudioDeviceID m_deviceId;
            SDL_AudioSpec m_audioSpec;
            std::map<uint32_t, PlayingSound> m_playingSounds;
            float m_masterVolume = 1.0f;
        };
    }
}
