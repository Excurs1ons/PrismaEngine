#pragma once

#include "graphic/interfaces/ITexture.h"
#include "audio/IAudioDevice.h"
#include "audio/AudioTypes.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace PacMan {

/**
 * @brief 游戏资源管理器
 * 统一管理吃豆人项目的纹理和音效
 */
class GameResourceManager {
public:
    static GameResourceManager& Get() {
        static GameResourceManager instance;
        return instance;
    }

    // ========== 纹理管理 ==========
    
    std::shared_ptr<Prisma::Graphic::ITexture> GetTexture(const std::string& key);
    void LoadTexture(const std::string& key, const std::string& path);

    // ========== 音频管理 ==========
    
    void InitializeAudio(Prisma::Audio::IAudioDevice* device);
    void LoadAudio(const std::string& key, const std::string& path);
    void PlayAudio(const std::string& key, bool loop = false, float volume = 1.0f);
    void StopAllAudio();

private:
    GameResourceManager() = default;
    
    std::unordered_map<std::string, std::shared_ptr<Prisma::Graphic::ITexture>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<Prisma::Audio::AudioClip>> m_audioClips;
    
    Prisma::Audio::IAudioDevice* m_audioDevice = nullptr;
};

} // namespace PacMan
