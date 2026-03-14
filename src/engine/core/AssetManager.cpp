#include "AssetManager.h"
#include <vector>

namespace Prisma {

// AssetManager::Impl 定义
struct AssetManager::Impl {
    std::filesystem::path projectRoot;
    std::vector<std::filesystem::path> searchPaths;
    std::unordered_map<Core::StringHash::HashType, std::shared_ptr<Asset>> assets;
    bool initialized = false;
};

AssetManager::AssetManager() : m_Impl(std::make_unique<Impl>()) {}

AssetManager::~AssetManager() {
    Shutdown();
}

int AssetManager::Initialize() {
    // 默认初始化到当前工作目录
    return Initialize(std::filesystem::current_path()) ? 0 : -1;
}

bool AssetManager::Initialize(const std::filesystem::path& projectRoot) {
    if (m_Impl->initialized) return true;
    
    m_Impl->projectRoot = projectRoot;
    m_Impl->searchPaths.push_back(projectRoot);
    m_Impl->searchPaths.push_back(projectRoot / "assets");

    m_Impl->initialized = true;
    LOG_INFO("AssetManager", "AssetManager initialized with root: {0}", projectRoot.string());
    return true;
}

void AssetManager::Shutdown() {
    if (!m_Impl->initialized) return;
    UnloadAll();
    m_Impl->initialized = false;
}

void AssetManager::AddSearchPath(const std::filesystem::path& path) {
    m_Impl->searchPaths.push_back(path);
}

std::optional<std::filesystem::path> AssetManager::FindResource(const std::string& relativePath) const {
    for (const auto& path : m_Impl->searchPaths) {
        auto fullPath = path / relativePath;
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }
    return std::nullopt;
}

void AssetManager::Unload(const std::string& name) {
    Core::StringHash hash(name);
    m_Impl->assets.erase(hash);
}

void AssetManager::UnloadAll() {
    m_Impl->assets.clear();
}

bool AssetManager::IsInitialized() const {
    return m_Impl->initialized;
}

std::shared_ptr<Asset> AssetManager::GetAssetFromCache(Core::StringHash::HashType hash) {
    auto it = m_Impl->assets.find(hash);
    if (it != m_Impl->assets.end()) {
        return it->second;
    }
    return nullptr;
}

void AssetManager::RegisterAsset(Core::StringHash::HashType hash, std::shared_ptr<Asset> asset) {
    m_Impl->assets[hash] = asset;
}

} // namespace Prisma
