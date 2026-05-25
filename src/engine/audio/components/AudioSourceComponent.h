#pragma once

#include <Engine/audio/AudioTypes.h>
#include <string>
#include <memory>
#include <cstdint>

namespace Prisma::Audio::Components {

struct AudioSourceComponent {
    std::string clipPath;
    std::shared_ptr<AudioClip> clip;
    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool looping = false;
    bool playing = false;
    bool paused = false;
    float playTime = 0.0f;
    float duration = 0.0f;
    bool is3D = false;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    float rolloffFactor = 1.0f;
    uint32_t audioVoiceId = 0;
};

} // namespace Prisma::Audio::Components
