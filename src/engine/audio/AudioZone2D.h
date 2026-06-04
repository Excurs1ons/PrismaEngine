#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include "physics2d/AABB2D.h"

namespace Prisma::Audio {

struct AudioZoneDef {
    Physics2D::AABB2D bounds;
    std::string bgmPath;
    float bgmVolume = 1.0f;
    bool bgmLoop = true;
    float fadeDurationMs = 500.0f; // BGM crossfade duration
};

class AudioZoneManager {
public:
    static AudioZoneManager& Get();

    uint32_t RegisterZone(const AudioZoneDef& def);
    void UnregisterZone(uint32_t zoneId);
    void UpdateZoneBGM(uint32_t zoneId, const std::string& newBgmPath);

    // Call every frame with player position
    void SetPlayerPosition(float x, float y);

    // Zone-specific SFX playback
    void PlaySFXAtZone(uint32_t zoneId, const std::string& clipPath, float volume = 1.0f);

    // Global SFX playback (respects master SFX volume)
    void PlaySFX(const std::string& clipPath, float volume = 1.0f);

    // Global control
    void SetMasterSFXVolume(float volume);
    void SetMasterBGMVolume(float volume);
    void StopAll();

private:
    AudioZoneManager() = default;
    ~AudioZoneManager() = default;
    AudioZoneManager(const AudioZoneManager&) = delete;
    AudioZoneManager& operator=(const AudioZoneManager&) = delete;

    struct ZoneEntry {
        uint32_t id;
        AudioZoneDef def;
        bool playerInside = false;
    };

    std::unordered_map<uint32_t, ZoneEntry> m_zones;
    uint32_t m_currentZoneId = 0;
    uint32_t m_nextZoneId = 1;
    uint32_t m_activeBgmVoice = 0;
    float m_playerX = 0.0f;
    float m_playerY = 0.0f;
    float m_masterSFXVolume = 1.0f;
    float m_masterBGMVolume = 1.0f;

    void UpdateBGM();
};

} // namespace Prisma::Audio
