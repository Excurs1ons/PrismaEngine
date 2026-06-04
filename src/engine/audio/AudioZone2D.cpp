#include "audio/AudioZone2D.h"
#include "audio/AudioAPI.h"
#include "audio/IAudioDevice.h"
#include "audio/AudioTypes.h"
#include "app/Engine.h"
#include "Logger.h"

namespace Prisma::Audio {

// ============================================================
// Singleton
// ============================================================

AudioZoneManager& AudioZoneManager::Get() {
    static AudioZoneManager instance;
    return instance;
}

// ============================================================
// Zone Registration
// ============================================================

uint32_t AudioZoneManager::RegisterZone(const AudioZoneDef& def) {
    uint32_t id = m_nextZoneId++;
    ZoneEntry entry;
    entry.id = id;
    entry.def = def;
    entry.playerInside = false;
    m_zones[id] = std::move(entry);
    LOG_INFO("AudioZone", "Registered zone {}: bounds({:.1f},{:.1f},{:.1f},{:.1f}) bgm={}",
             id, def.bounds.minX, def.bounds.minY, def.bounds.maxX, def.bounds.maxY,
             def.bgmPath.empty() ? "(none)" : def.bgmPath.c_str());
    return id;
}

void AudioZoneManager::UnregisterZone(uint32_t zoneId) {
    auto it = m_zones.find(zoneId);
    if (it == m_zones.end()) {
        LOG_WARN("AudioZone", "UnregisterZone: zone {} not found", zoneId);
        return;
    }

    // If we're currently in this zone, stop BGM
    if (m_currentZoneId == zoneId) {
        auto* dev = Engine::Get().GetAudioDevice();
        if (dev && m_activeBgmVoice != 0) {
            dev->Stop(m_activeBgmVoice);
            m_activeBgmVoice = 0;
        }
        m_currentZoneId = 0;
    }

    m_zones.erase(it);
    LOG_INFO("AudioZone", "Unregistered zone {}", zoneId);
}

void AudioZoneManager::UpdateZoneBGM(uint32_t zoneId, const std::string& newBgmPath) {
    auto it = m_zones.find(zoneId);
    if (it == m_zones.end()) {
        LOG_WARN("AudioZone", "UpdateZoneBGM: zone {} not found", zoneId);
        return;
    }

    it->second.def.bgmPath = newBgmPath;

    // If player is currently in this zone, update BGM immediately
    if (m_currentZoneId == zoneId) {
        UpdateBGM();
    }
}

// ============================================================
// Player Position & Zone Detection
// ============================================================

void AudioZoneManager::SetPlayerPosition(float x, float y) {
    m_playerX = x;
    m_playerY = y;

    // Check all zones: last matching zone in registration order takes priority
    uint32_t newZoneId = 0;
    for (auto& [id, entry] : m_zones) {
        entry.playerInside = entry.def.bounds.Contains(x, y);
        if (entry.playerInside) {
            newZoneId = id;
        }
    }

    if (newZoneId != m_currentZoneId) {
        LOG_INFO("AudioZone", "Player entered zone {} (was zone {})", newZoneId, m_currentZoneId);
        m_currentZoneId = newZoneId;
        UpdateBGM();
    }
}

// ============================================================
// BGM Management
// ============================================================

void AudioZoneManager::UpdateBGM() {
    auto* dev = Engine::Get().GetAudioDevice();
    if (!dev) return;

    // Stop current BGM if playing
    if (m_activeBgmVoice != 0) {
        dev->Stop(m_activeBgmVoice);
        m_activeBgmVoice = 0;
    }

    // No zone → no BGM
    if (m_currentZoneId == 0) return;

    auto it = m_zones.find(m_currentZoneId);
    if (it == m_zones.end()) return;

    const auto& def = it->second.def;
    if (def.bgmPath.empty()) return;

    // Load clip
    auto clip = AudioAPI::LoadClip(def.bgmPath);
    if (!clip) {
        LOG_WARN("AudioZone", "Failed to load BGM: {}", def.bgmPath.c_str());
        return;
    }

    // Start playback
    PlayDesc desc;
    desc.volume = std::clamp(def.bgmVolume * m_masterBGMVolume, 0.0f, 1.0f);
    desc.loop = def.bgmLoop;
    desc.pitch = 1.0f;

    m_activeBgmVoice = dev->PlayClip(*clip, desc);
    LOG_INFO("AudioZone", "Started BGM zone {}: voice={} path={} vol={:.2f} loop={}",
             m_currentZoneId, m_activeBgmVoice, def.bgmPath.c_str(), desc.volume, desc.loop);
}

// ============================================================
// SFX Playback
// ============================================================

void AudioZoneManager::PlaySFXAtZone(uint32_t zoneId, const std::string& clipPath, float volume) {
    if (clipPath.empty()) return;

    auto it = m_zones.find(zoneId);
    if (it == m_zones.end()) {
        LOG_WARN("AudioZone", "PlaySFXAtZone: zone {} not found", zoneId);
        return;
    }

    auto* dev = Engine::Get().GetAudioDevice();
    if (!dev) return;

    auto clip = AudioAPI::LoadClip(clipPath);
    if (!clip) return;

    PlayDesc desc;
    desc.volume = std::clamp(volume * m_masterSFXVolume, 0.0f, 1.0f);
    desc.loop = false;

    dev->PlayClip(*clip, desc);
}

void AudioZoneManager::PlaySFX(const std::string& clipPath, float volume) {
    if (clipPath.empty()) return;

    auto* dev = Engine::Get().GetAudioDevice();
    if (!dev) return;

    auto clip = AudioAPI::LoadClip(clipPath);
    if (!clip) return;

    PlayDesc desc;
    desc.volume = std::clamp(volume * m_masterSFXVolume, 0.0f, 1.0f);
    desc.loop = false;

    dev->PlayClip(*clip, desc);
}

// ============================================================
// Volume Control
// ============================================================

void AudioZoneManager::SetMasterSFXVolume(float volume) {
    m_masterSFXVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioZoneManager::SetMasterBGMVolume(float volume) {
    m_masterBGMVolume = std::clamp(volume, 0.0f, 1.0f);

    auto* dev = Engine::Get().GetAudioDevice();
    if (dev && m_activeBgmVoice != 0 && m_currentZoneId != 0) {
        auto it = m_zones.find(m_currentZoneId);
        if (it != m_zones.end()) {
            float newVol = std::clamp(it->second.def.bgmVolume * m_masterBGMVolume, 0.0f, 1.0f);
            dev->SetVolume(m_activeBgmVoice, newVol);
        }
    }
}

void AudioZoneManager::StopAll() {
    auto* dev = Engine::Get().GetAudioDevice();
    if (dev) {
        dev->StopAll();
    }
    m_activeBgmVoice = 0;
    m_currentZoneId = 0;
}

} // namespace Prisma::Audio
