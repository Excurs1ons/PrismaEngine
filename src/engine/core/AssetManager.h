#pragma once

#include "Asset.h"
#include "Logger.h"
#include "ISubSystem.h" // 继承自这个，而不是 ManagerBase
#include "StringHash.h"
#include "../JobSystem.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <functional>
#include <unordered_map>

namespace Prisma {

/**
 * @brief 资产管理器 (一个普通的子系统，受 Engine 调遣)
 */
class ENGINE_API AssetManager : public ISubSystem {
public:
    AssetManager();
    ~AssetManager() override;

    // ISubSystem 接口
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override {}

    // 显式初始化
    bool Initialize(const std::filesystem::path& projectRoot);
    
    void AddSearchPath(const std::filesystem::path& path);
    std::optional<std::filesystem::path> FindResource(const std::string& relativePath) const;

    template <typename T> 
    AssetHandle<T> GetCachedAsset(Core::StringHash::HashType hash) {
        auto asset = GetAssetFromCache(hash);
        return AssetHandle<T>(std::dynamic_pointer_cast<T>(asset));
    }

    template <typename T, typename... Args> 
    AssetHandle<T> Load(const std::string& relativePath, Args&&... args) {
        // 删掉那些愚蠢的“自动初始化”逻辑。
        // 如果这里没初始化，那就是程序员的错，应该让他直接 Crash 或者收到报错，而不是帮他遮掩。
        
        Core::StringHash hash(relativePath);
        auto cached = GetCachedAsset<T>(hash);
        if (cached)
            return cached;

        auto fullPath = FindResource(relativePath);
        if (!fullPath) {
            LOG_ERROR("AssetManager", "Asset not found: {0}", relativePath);
            return AssetHandle<T>();
        }

        auto asset = std::make_shared<T>(std::forward<Args>(args)...);
        asset->Name = relativePath;
        asset->Path = *fullPath;
        
        if (!asset->Load(*fullPath)) {
            LOG_ERROR("AssetManager", "Failed to load asset: {0}", relativePath);
            return AssetHandle<T>();
        }

        RegisterAsset(hash, asset);
        return AssetHandle<T>(asset);
    }

    // Async 加载暂时保留，但 JobSystem::Get() 也得赶紧干掉
    template <typename T, typename... Args> 
    void LoadAsync(const std::string& relativePath, std::function<void(AssetHandle<T>)> callback, Args... args) {
        Core::StringHash hash(relativePath);
        auto cached = GetCachedAsset<T>(hash);
        if (cached) {
            if (callback) callback(cached);
            return;
        }

        auto fullPath = FindResource(relativePath);
        if (!fullPath) {
            LOG_ERROR("AssetManager", "Asset not found: {0}", relativePath);
            if (callback) callback(AssetHandle<T>());
            return;
        }

        std::filesystem::path path = *fullPath;
        
        // 这里的 JobSystem 以后也要改
        JobSystem::Get()->SubmitJob([this, hash, relativePath, path, callback, args...]() {
            auto asset = std::make_shared<T>(args...);
            asset->Name = relativePath;
            asset->Path = path;

            if (!asset->Load(path)) {
                LOG_ERROR("AssetManager", "Async load failed: {0}", relativePath);
                if (callback) callback(AssetHandle<T>());
                return;
            }

            RegisterAsset(hash, asset);
            if (callback) callback(AssetHandle<T>(asset));
        });
    }

    void Unload(const std::string& name);
    void UnloadAll();
    bool IsInitialized() const;

private:
    std::shared_ptr<Asset> GetAssetFromCache(Core::StringHash::HashType hash);
    void RegisterAsset(Core::StringHash::HashType hash, std::shared_ptr<Asset> asset);

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // namespace Prisma
