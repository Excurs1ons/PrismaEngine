// AssetManager.h
#pragma once
#include "Logger.h"
#include "ManagerBase.h"
#include "AssetBase.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <shared_mutex>  // C++17
#include <string>
#include <type_traits>
#include <unordered_map>
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "StringHash.h"

#if defined(FindResource)
#undef FindResource
#endif

namespace PrismaEngine {
namespace Resource {
    class AssetFallback;
}
}

namespace PrismaEngine {
// ============================================================================
// 细粒度锁的资源管理器
// ============================================================================
class AssetManager : public ManagerBase<AssetManager> {
    friend class ISubSystem;
    friend class ManagerBase<AssetManager>;
public:
    bool Initialize() override { return Initialize(std::filesystem::current_path()); }

    void Shutdown() override { UnloadAll(); }
    static constexpr std::string GetName() { return R"(AssetManager)"; }

    /// @brief 初始化资源管理器
    bool Initialize(const std::filesystem::path& project_root) {
        LOG_INFO("Resource", "资源系统正在初始化...");
        std::unique_lock<std::mutex> config_lock(m_configMutex);

        if (m_initialized) {
            LOG_INFO("Resource", "资源系统已初始化");
            return true;
        }

        if (!std::filesystem::exists(project_root)) {
            LOG_ERROR("Resource", "项目根目录不存在: {0}", project_root.string());
            return false;
        }

        m_projectRoot = std::filesystem::absolute(project_root);
        LOG_INFO("Resource", "资源系统根目录: {0}", m_projectRoot.string());

        config_lock.unlock();

        AddSearchPath(m_projectRoot / "Assets");
        AddSearchPath(m_projectRoot / "Assets/Shaders");
        AddSearchPath(m_projectRoot / "Assets/Textures");
        AddSearchPath(m_projectRoot / "Assets/Models");
        AddSearchPath(m_projectRoot / "Assets/Audio");

        config_lock.lock();
        m_initialized = true;
        LOG_INFO("Resource", "资源系统初始化完成。");
        return true;
    }

    /// @brief 添加搜索路径
    void AddSearchPath(const std::filesystem::path& path) {
        auto absolute_path = std::filesystem::absolute(path);
        std::error_code ec;
        if (!std::filesystem::exists(absolute_path, ec)) {
            std::filesystem::create_directories(absolute_path, ec);
        }

        {
            std::unique_lock<std::shared_mutex> lock(m_pathsMutex);
            bool found = false;
            for (const auto& existing_path : m_searchPaths) {
                if (existing_path == absolute_path) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                m_searchPaths.push_back(absolute_path);
            }
        }
    }

    /// @brief 查找资源
    std::optional<std::filesystem::path> FindResource(const std::string& relative_path) const {
        std::filesystem::path absolute_path(relative_path);
        if (absolute_path.is_absolute()) {
            if (std::filesystem::exists(absolute_path)) return absolute_path;
        }

        std::vector<std::filesystem::path> search_paths_copy;
        {
            std::shared_lock<std::shared_mutex> lock(m_pathsMutex);
            search_paths_copy = m_searchPaths;
        }

        for (const auto& search_path : search_paths_copy) {
            std::filesystem::path full_path = search_path / relative_path;
            if (std::filesystem::exists(full_path)) return std::filesystem::absolute(full_path);
        }

        return std::nullopt;
    }

    /// @brief 查找缓存资源（使用哈希 ID 快速查找）
    template <typename T> ResourceHandle<T> GetCachedResource(Core::StringHash::HashType hash) {
        std::shared_lock<std::shared_mutex> read_lock(m_resourcesMutex);
        auto it = m_resources.find(hash);
        if (it != m_resources.end()) {
            auto resource = std::dynamic_pointer_cast<T>(it->second);
            if (resource && resource->IsLoaded()) {
                return ResourceHandle<T>(resource);
            }
        }
        return ResourceHandle<T>();
    }

    /// @brief 加载资源
    template <typename T, typename... Args> ResourceHandle<T> Load(const std::string& relative_path, Args&&... args) {
        if (!IsInitialized()) Initialize(std::filesystem::current_path());

        Core::StringHash hash(relative_path);
        auto cached = GetCachedResource<T>(hash);
        if (cached) return cached;

        auto fullPath = FindResource(relative_path);
        if (!fullPath) {
            LOG_ERROR("Resource", "资源未找到: {0}", relative_path);
            return ResourceHandle<T>();
        }

        auto resource = std::make_shared<T>(std::forward<Args>(args)...);
        if (!resource->Load(*fullPath)) {
            LOG_ERROR("Resource", "资源加载失败: {0}", relative_path);
            return ResourceHandle<T>();
        }

        {
            std::unique_lock<std::shared_mutex> write_lock(m_resourcesMutex);
            auto it = m_resources.find(hash);
            if (it != m_resources.end()) {
                return ResourceHandle<T>(std::dynamic_pointer_cast<T>(it->second));
            }
            m_resources[hash] = resource;
            m_pathCache[hash] = relative_path;
        }

        return ResourceHandle<T>(resource);
    }

    void CreateDefaultAssets();

    void Unload(const std::string& name) {
        Core::StringHash hash(name);
        std::unique_lock<std::shared_mutex> lock(m_resourcesMutex);
        auto it = m_resources.find(hash);
        if (it != m_resources.end()) {
            it->second->Unload();
            m_resources.erase(it);
            m_pathCache.erase(hash);
        }
    }

    void UnloadAll() {
        std::unique_lock<std::shared_mutex> lock(m_resourcesMutex);
        for (const auto& resource : m_resources | std::views::values) {
            resource->Unload();
        }
        m_resources.clear();
        m_pathCache.clear();

        {
            std::unique_lock<std::shared_mutex> paths_lock(m_pathsMutex);
            m_searchPaths.clear();
        }

        std::lock_guard<std::mutex> config_lock(m_configMutex);
        m_initialized = false;
    }

    bool IsInitialized() const {
        std::lock_guard<std::mutex> lock(m_configMutex);
        return m_initialized;
    }

    ~AssetManager() override { Shutdown(); }

private:
    void CreateDefaultMeshes(const std::filesystem::path& meshes_dir);
    void CreateDefaultShaders(const std::filesystem::path& shaders_dir);
    void CreateDefaultTextures(const std::filesystem::path& textures_dir);
    void CreateDefaultMaterials(const std::filesystem::path& materials_dir);

    mutable std::mutex m_configMutex;
    bool m_initialized = false;
    std::filesystem::path m_projectRoot;

    mutable std::shared_mutex m_pathsMutex;
    std::vector<std::filesystem::path> m_searchPaths;

    mutable std::shared_mutex m_resourcesMutex;
    std::unordered_map<Core::StringHash::HashType, std::shared_ptr<AssetBase>> m_resources;
    std::unordered_map<Core::StringHash::HashType, std::string> m_pathCache; // 调试用
};

}  // namespace PrismaEngine
