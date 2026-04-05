#include "GameResourceManager.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IResourceManager.h"
#include "audio/AudioAPI.h"
#include "Logger.h"

namespace PacMan {

std::shared_ptr<Prisma::Graphic::ITexture> GameResourceManager::GetTexture(const std::string& key) {
    auto it = m_textures.find(key);
    return (it != m_textures.end()) ? it->second : nullptr;
}

void GameResourceManager::LoadTexture(const std::string& key, const std::string& path) {
    auto* rm = Prisma::Engine::Get().GetRenderResourceManager();
    if (!rm) {
        LOG_ERROR("PacMan", "无法获取渲染资源管理器");
        return;
    }

    auto texture = rm->LoadTexture(path);
    if (texture) {
        m_textures[key] = texture;
    } else {
        LOG_ERROR("PacMan", "无法加载纹理: {0} ({1})", key, path);
    }
}

void GameResourceManager::InitializeAudio(Prisma::Audio::IAudioDevice* device) {
    m_audioDevice = device;
}

void GameResourceManager::LoadAudio(const std::string& key, const std::string& path) {
    auto clip = Prisma::Audio::AudioAPI::LoadClip(path);
    if (clip) {
        m_audioClips[key] = clip;
    } else {
        LOG_ERROR("PacMan", "无法加载音频: {0} ({1})", key, path);
    }
}

void GameResourceManager::PlayAudio(const std::string& key, bool loop, float volume) {
    if (!m_audioDevice) return;
    
    auto it = m_audioClips.find(key);
    if (it != m_audioClips.end() && it->second) {
        Prisma::Audio::PlayDesc desc;
        desc.loop = loop;
        desc.volume = volume;
        m_audioDevice->PlayClip(*it->second, desc);
    }
}

void GameResourceManager::StopAllAudio() {
    if (m_audioDevice) {
        m_audioDevice->StopAll();
    }
}

} // namespace PacMan
