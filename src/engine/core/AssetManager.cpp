#include "AssetManager.h"
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace PrismaEngine {

struct AssetManager::Impl {
    mutable std::mutex configMutex;
    bool initialized = false;
    std::filesystem::path projectRoot;

    mutable std::shared_mutex pathsMutex;
    std::vector<std::filesystem::path> searchPaths;

    mutable std::shared_mutex resourcesMutex;
    std::unordered_map<Core::StringHash::HashType, std::shared_ptr<AssetBase>> resources;
    std::unordered_map<Core::StringHash::HashType, std::string> pathCache;
};

std::shared_ptr<AssetManager> AssetManager::GetInstance() {
    static std::shared_ptr<AssetManager> instance = std::shared_ptr<AssetManager>(new AssetManager());
    return instance;
}

AssetManager::AssetManager() : m_impl(std::make_unique<Impl>()) {
}

AssetManager::~AssetManager() {
    Shutdown();
}

int AssetManager::Initialize() {
    return Initialize(std::filesystem::current_path()) ? 0 : -1;
}

void AssetManager::Shutdown() {
    UnloadAll();
}

bool AssetManager::Initialize(const std::filesystem::path& project_root) {
    LOG_INFO("Resource", "资源系统正在初始化...");
    std::unique_lock<std::mutex> config_lock(m_impl->configMutex);

    if (m_impl->initialized) {
        LOG_INFO("Resource", "资源系统已初始化");
        return true;
    }

    if (!std::filesystem::exists(project_root)) {
        LOG_ERROR("Resource", "项目根目录不存在: {0}", project_root.string());
        return false;
    }

    m_impl->projectRoot = std::filesystem::absolute(project_root);
    LOG_INFO("Resource", "资源系统根目录: {0}", m_impl->projectRoot.string());

    config_lock.unlock();

    AddSearchPath(m_impl->projectRoot / "Assets");
    AddSearchPath(m_impl->projectRoot / "Assets/Shaders");
    AddSearchPath(m_impl->projectRoot / "Assets/Textures");
    AddSearchPath(m_impl->projectRoot / "Assets/Models");
    AddSearchPath(m_impl->projectRoot / "Assets/Audio");

    config_lock.lock();
    m_impl->initialized = true;
    LOG_INFO("Resource", "资源系统初始化完成。");
    return true;
}

void AssetManager::AddSearchPath(const std::filesystem::path& path) {
    auto absolute_path = std::filesystem::absolute(path);
    {
        std::unique_lock<std::shared_mutex> lock(m_impl->pathsMutex);
        if (std::find(m_impl->searchPaths.begin(), m_impl->searchPaths.end(), absolute_path) == m_impl->searchPaths.end()) {
            m_impl->searchPaths.push_back(absolute_path);
        }
    }
}

std::optional<std::filesystem::path> AssetManager::FindResource(const std::string& relative_path) const {
    std::filesystem::path path(relative_path);
    if (path.is_absolute() && std::filesystem::exists(path)) return path;

    std::shared_lock<std::shared_mutex> lock(m_impl->pathsMutex);
    for (const auto& search_path : m_impl->searchPaths) {
        std::filesystem::path full_path = search_path / relative_path;
        if (std::filesystem::exists(full_path)) return std::filesystem::absolute(full_path);
    }
    return std::nullopt;
}

std::shared_ptr<AssetBase> AssetManager::GetCachedAsset(Core::StringHash::HashType hash) {
    std::shared_lock<std::shared_mutex> lock(m_impl->resourcesMutex);
    auto it = m_impl->resources.find(hash);
    if (it != m_impl->resources.end() && it->second->IsLoaded()) {
        return it->second;
    }
    return nullptr;
}

void AssetManager::RegisterAsset(Core::StringHash::HashType hash, const std::string& path, std::shared_ptr<AssetBase> asset) {
    std::unique_lock<std::shared_mutex> lock(m_impl->resourcesMutex);
    m_impl->resources[hash] = asset;
    m_impl->pathCache[hash] = path;
}

void AssetManager::Unload(const std::string& name) {
    Core::StringHash hash(name);
    std::unique_lock<std::shared_mutex> lock(m_impl->resourcesMutex);
    auto it = m_impl->resources.find(hash);
    if (it != m_impl->resources.end()) {
        it->second->Unload();
        m_impl->resources.erase(it);
        m_impl->pathCache.erase(hash);
    }
}

void AssetManager::UnloadAll() {
    std::unique_lock<std::shared_mutex> lock(m_impl->resourcesMutex);
    for (auto& pair : m_impl->resources) {
        if (pair.second) pair.second->Unload();
    }
    m_impl->resources.clear();
    m_impl->pathCache.clear();

    std::unique_lock<std::shared_mutex> paths_lock(m_impl->pathsMutex);
    m_impl->searchPaths.clear();

    std::lock_guard<std::mutex> config_lock(m_impl->configMutex);
    m_impl->initialized = false;
}

bool AssetManager::IsInitialized() const {
    std::lock_guard<std::mutex> lock(m_impl->configMutex);
    return m_impl->initialized;
}

void AssetManager::CreateDefaultAssets() {}

} // namespace PrismaEngine
