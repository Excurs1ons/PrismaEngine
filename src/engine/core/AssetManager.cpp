#include "AssetManager.h"
#include <vector>

namespace Prisma {

// AssetManager::Impl 定义
struct AssetManager::Impl {
    std::filesystem::path projectRoot;
    std::vector<std::filesystem::path> searchPaths;
    
    struct AssetEntry {
        std::shared_ptr<Asset> asset;
        std::filesystem::file_time_type lastWriteTime;
    };
    
    std::unordered_map<Core::StringHash::HashType, AssetEntry> assets;
    bool initialized = false;
};

AssetManager::AssetManager() : m_Impl(std::make_unique<Impl>()) {}

AssetManager::~AssetManager() {
    Shutdown();
}

int AssetManager::Initialize() {
    // 在 Android 上，优先使用 SDL3 的 PrefPath 或解压路径
#ifdef __ANDROID__
    // 这里我们假设 Java 层解压到了 FilesDir
    // 由于 SDL3 尚未初始化完成时可能无法获取 PrefPath，我们先尝试标准路径
    // 或者在 Engine::Initialize 中显式传入
    return Initialize("/data/data/com.prismaengine.android/files") ? 0 : -1;
#else
    // 使用 exe 所在目录为基准，而非 CWD
    return Initialize(Platform::GetExecutablePath()) ? 0 : -1;
#endif
}

bool AssetManager::Initialize(const std::filesystem::path& projectRoot) {
    if (m_Impl->initialized) return true;
    
    m_Impl->projectRoot = projectRoot;
    m_Impl->searchPaths.push_back(projectRoot);
    m_Impl->searchPaths.push_back(projectRoot / "assets");

    m_Impl->initialized = true;
    LOG_INFO("AssetManager", "资源管理器已初始化，根目录: {0}", projectRoot.string());
    return true;
}

void AssetManager::Shutdown() {
    if (!m_Impl->initialized) return;
    UnloadAll();
    m_Impl->initialized = false;
}

void AssetManager::Update(Timestep ts) {
    m_lastCleanupTime += ts.GetSeconds();

    // 每 2 秒检查一次热重载 (Hot Reloading)
    if (m_lastCleanupTime >= 2.0f) {
        for (auto& [hash, entry] : m_Impl->assets) {
            if (!entry.asset || entry.asset->GetPath().empty()) continue;

            const auto& path = entry.asset->GetPath();
            if (std::filesystem::exists(path)) {
                auto currentWriteTime = std::filesystem::last_write_time(path);
                if (currentWriteTime > entry.lastWriteTime) {
                    LOG_INFO("AssetManager", "检测到资源变动: {0}。正在重新加载...", entry.asset->GetName());
                    entry.asset->Unload();
                    if (entry.asset->Load(path)) {
                        entry.lastWriteTime = currentWriteTime;
                        LOG_INFO("AssetManager", "资源重新加载成功。");
                    } else {
                        LOG_ERROR("AssetManager", "重新加载资源失败: {0}", entry.asset->GetName());
                    }
                }
            }
        }
        m_lastCleanupTime = 0.0f;
    }
}

void AssetManager::AddSearchPath(const std::filesystem::path& path) {
    m_Impl->searchPaths.push_back(path);
}

std::optional<std::filesystem::path> AssetManager::FindResource(const std::string& relativePath) const {
    for (const auto& path : m_Impl->searchPaths) {
        auto fullPath = path / relativePath;
        std::string pathStr = fullPath.string();
        if (Platform::FileExists(pathStr.c_str())) {
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
        return it->second.asset;
    }
    return nullptr;
}

void AssetManager::RegisterAsset(Core::StringHash::HashType hash, std::shared_ptr<Asset> asset) {
    AssetManager::Impl::AssetEntry entry;
    entry.asset = asset;
    if (std::filesystem::exists(asset->GetPath())) {
        entry.lastWriteTime = std::filesystem::last_write_time(asset->GetPath());
    }
    m_Impl->assets[hash] = entry;
}

// C API 导出实现
extern "C" {
    void* Prisma_AssetManager_GetAssetData(const char* path, size_t* outSize) {
        auto data = Platform::ReadBinaryFile(path);
        if (data.empty()) {
            if (outSize) *outSize = 0;
            return nullptr;
        }
        
        if (outSize) *outSize = data.size();
        void* buffer = malloc(data.size());
        std::memcpy(buffer, data.data(), data.size());
        return buffer;
    }

    void Prisma_AssetManager_FreeAssetData(void* data) {
        if (data) free(data);
    }
}

} // namespace Prisma
