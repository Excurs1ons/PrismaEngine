#pragma once
#include "Logger.h"
#include "ManagerBase.h"
#include "AssetBase.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include "StringHash.h"

namespace PrismaEngine {

class ENGINE_API AssetManager : public ManagerBase<AssetManager> {
public:
    static std::shared_ptr<AssetManager> GetInstance();

    AssetManager();
    ~AssetManager() override;

    int Initialize() override;
    void Shutdown() override;
    static constexpr const char* GetStaticName() { return "AssetManager"; }

    bool Initialize(const std::filesystem::path& project_root);
    void AddSearchPath(const std::filesystem::path& path);
    std::optional<std::filesystem::path> FindResource(const std::string& relative_path) const;

    template <typename T> ResourceHandle<T> GetCachedResource(Core::StringHash::HashType hash) {
        auto asset = GetCachedAsset(hash);
        return ResourceHandle<T>(std::dynamic_pointer_cast<T>(asset));
    }

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

        RegisterAsset(hash, relative_path, resource);
        return ResourceHandle<T>(resource);
    }

    void CreateDefaultAssets();
    void Unload(const std::string& name);
    void UnloadAll();
    bool IsInitialized() const;

private:
    // Pimpl 支持方法
    std::shared_ptr<AssetBase> GetCachedAsset(Core::StringHash::HashType hash);
    void RegisterAsset(Core::StringHash::HashType hash, const std::string& path, std::shared_ptr<AssetBase> asset);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace PrismaEngine
