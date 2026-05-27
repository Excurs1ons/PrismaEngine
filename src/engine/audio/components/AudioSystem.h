#pragma once

#include <Engine/core/ECS.h>
#include <Engine/audio/AudioAPI.h>
#include <Engine/audio/AudioTypes.h>
#include <Engine/audio/components/AudioSourceComponent.h>
#include <Engine/audio/components/AudioListenerComponent.h>
#include <Engine/audio/components/ReverbZoneComponent.h>
#include <unordered_map>
#include <memory>

namespace Prisma::Audio::Components {

class AudioSystem : public Core::ECS::ISystem {
public:
    void Initialize() override {
        AudioDesc desc;
        desc.deviceType = AudioDeviceType::SDL3;
        desc.outputFormat = AudioFormat(48000, 2, 32);
        m_device = AudioAPI::CreateDevice(desc.deviceType, desc);
        if (m_device) m_device->Initialize(desc);
    }

    void Update(Prisma::Timestep ts) override {
        if (!m_device) return;

        m_device->Update();

        auto& world = Core::ECS::World::Get();
        auto& cm = world.GetComponentManager();
        auto* pool = cm.GetPool<AudioSourceComponent>();
        if (!pool) return;

        for (auto& source : pool->GetData()) {
            if (!source.playing || source.paused) continue;

            source.playTime += ts.GetSeconds();

            if (source.looping && source.duration > 0.0f) {
                if (source.playTime >= source.duration) {
                    source.playTime = 0.0f;
                }
            }
        }
    }

    void Shutdown() override {
        if (m_device) {
            m_device->Shutdown();
            m_device.reset();
        }
    }

    AudioVoiceId Play(const std::string& path, float volume = 1.0f) {
        if (!m_device) return INVALID_VOICE_ID;
        PlayDesc desc;
        desc.volume = volume;
        return m_device->Play(path, desc);
    }

    AudioVoiceId PlayClip(const std::shared_ptr<AudioClip>& clip, float volume = 1.0f) {
        if (!m_device) return INVALID_VOICE_ID;
        PlayDesc desc;
        desc.volume = volume;
        return m_device->PlayClip(clip, desc);
    }

    void Stop(AudioVoiceId id) {
        if (m_device) m_device->Stop(id);
    }

    void StopAll() {
        if (m_device) m_device->StopAll();
    }

    void SetMasterVolume(float vol) {
        if (m_device) m_device->SetMasterVolume(vol);
    }

    IAudioDevice* GetDevice() const { return m_device.get(); }

private:
    std::unique_ptr<IAudioDevice> m_device;
};

} // namespace Prisma::Audio::Components
