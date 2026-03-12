#include "HotReloadManager.h"
#include "Logger.h"
#include <functional>
#include <mutex>
#include <memory>
#include <unordered_map>

namespace PrismaEngine::Core {

class ResourceManager {
private:
    std::unique_ptr<HotReloadManager> m_hotReloadManager;
    std::unordered_map<std::string, std::shared_ptr<AssetBase>> m_loadedAssets;
    std::mutex m_mutex;

public:
    ResourceManager() {
        m_hotReloadManager = std::make_unique<HotReloadManager>();
        m_hotReloadManager->Initialize();
    }

    ~ResourceManager() {
        m_hotReloadManager->Shutdown();
    }

    void CheckFileModifications() {
        m_hotReloadManager->WatchFilesystem();
        
        for (auto& [path, change] : m_hotReloadManager->GetWatchedFiles()) {
            if (!std::filesystem::exists(path.path)) {
                LOG_WARNING("ResourceManager", "Asset file removed: {}", path.path);
                UnregisterAsset(path.path);
                continue;
            }
            
            auto currentTime = std::filesystem::last_write_time(path.path);
            if (currentTime > change.lastModified) {
                LOG_INFO("ResourceManager", "Asset modified, reloading: {}", path.path);
                
                if (change.reloadCallback) {
                    change.reloadCallback();
                } else {
                    LOG_INFO("ResourceManager", "Asset modified but no callback: {}", path.path);
                }
            }
        }
    }

    void RegisterAssetForHotReload(const std::string& path, std::shared_ptr<AssetBase> asset, std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loadedAssets[path] = asset;
        m_hotReloadManager->RegisterAsset(path, [asset, callback](const std::string& modifiedPath) {
            asset->Reload();
        });
    }

    void UpdateAsset(const std::string& path, std::shared_ptr<AssetBase> newAsset) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loadedAssets[path] = newAsset;
    }

    std::shared_ptr<AssetBase> GetAsset(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_loadedAssets.find(path);
        return it != m_loadedAssets.end() ? it->second : nullptr;
    }

    bool HasAsset(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_loadedAssets.find(path) != m_loadedAssets.end();
    }

    size_t GetLoadedAssetCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_loadedAssets.size();
    }
};

} // namespace PrismaEngine::Core
