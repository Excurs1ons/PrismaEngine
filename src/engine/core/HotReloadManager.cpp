#include "HotReloadManager.h"

#include "Logger.h"

namespace PrismaEngine::Core {

class HotReloadManager {
public:
    struct AssetChange {
        std::string path;
        std::filesystem::file_time_type lastModified;
        std::function<void()> reloadCallback;
    };

    HotReloadManager() = default;
    ~HotReloadManager() = default;

    void Initialize();
    void Shutdown();
    void RegisterAsset(const std::string& path, std::function<void()> callback);
    void UnregisterAsset(const std::string& path);
    void TriggerReload(const std::string& path);
    
    std::unordered_map<std::string, AssetChange> GetWatchedFiles() const;
};

} // namespace Core
