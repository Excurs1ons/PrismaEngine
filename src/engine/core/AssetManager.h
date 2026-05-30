#pragma once

#include "../app/Engine.h"
#include "../logger/Logger.h"
#include "../threading/JobSystem.h"
#include "Asset.h"
#include "ISubSystem.h"  // 继承自这个，而不是 ManagerBase
#include "StringHash.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace Prisma {

// 资产管理器 (一个普通的子系统，受 Engine 调遣)
class ENGINE_API AssetManager : public ISubSystem {
public:
    AssetManager();
    ~AssetManager() override;

    // ISubSystem 接口
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    const char* GetName() const override { return "AssetManager"; }

    // 显式初始化
    bool Initialize(const std::filesystem::path& projectRoot);

    void AddSearchPath(const std::filesystem::path& path);
    std::optional<std::filesystem::path> FindResource(const std::string& relativePath) const;

    template <typename T> AssetHandle<T> GetCachedAsset(Core::StringHash::HashType hash) {
        auto asset = GetAssetFromCache(hash);
        return AssetHandle<T>(std::dynamic_pointer_cast<T>(asset));
    }

    template <typename T, typename... Args> AssetHandle<T> Load(const std::string& relativePath, Args&&... args) {
        // 删掉那些愚蠢的“自动初始化”逻辑。
        // 如果这里没初始化，那就是程序员的错，应该让他直接 Crash 或者收到报错，而不是帮他遮掩。

        Core::StringHash hash(relativePath);
        auto cached = GetCachedAsset<T>(hash);
        if (cached)
            return cached;

        auto fullPath = FindResource(relativePath);
        if (!fullPath) {
            LOG_ERROR("AssetManager", "未找到资源: {0}", relativePath);
            return AssetHandle<T>();
        }

        auto asset  = std::make_shared<T>(std::forward<Args>(args)...);
        asset->SetName(relativePath);
        asset->SetPath(*fullPath);

        // 优先尝试内存加载以适配 Android 资产
        auto data = Platform::ReadBinaryFile(fullPath->string().c_str());
        if (!data.empty()) {
            if (!asset->LoadFromMemory(data.data(), data.size())) {
                LOG_ERROR("AssetManager", "从内存加载资源失败: {0}", relativePath);
                return AssetHandle<T>();
            }
        } else {
            // 回退到路径加载
            if (!asset->Load(*fullPath)) {
                LOG_ERROR("AssetManager", "通过路径加载资源失败: {0}", relativePath);
                return AssetHandle<T>();
            }
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
            if (callback)
                callback(cached);
            return;
        }

        auto fullPath = FindResource(relativePath);
        if (!fullPath) {
            LOG_ERROR("AssetManager", "未找到资源: {0}", relativePath);
            if (callback)
                callback(AssetHandle<T>());
            return;
        }

        std::filesystem::path path = *fullPath;

        // 使用引擎统一管理的 JobSystem
        Engine::Get().GetJobSystem()->SubmitJob([this, hash, relativePath, path, callback, args...]() {
            auto asset  = std::make_shared<T>(args...);
            asset->SetName(relativePath);
            asset->SetPath(path);

            if (!asset->Load(path)) {
                LOG_ERROR("AssetManager", "异步加载失败: {0}", relativePath);
                if (callback)
                    callback(AssetHandle<T>());
                return;
            }

            RegisterAsset(hash, asset);
            if (callback)
                callback(AssetHandle<T>(asset));
        });
    }

    void Unload(const std::string& name);
    void UnloadAll();
    bool IsInitialized() const;

private:
    float m_lastCleanupTime = 0.0f;
    std::shared_ptr<Asset> GetAssetFromCache(Core::StringHash::HashType hash);
    void RegisterAsset(Core::StringHash::HashType hash, std::shared_ptr<Asset> asset);

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

// C API 导出，供 C# 内存加载程序集使用
extern "C" {
    ENGINE_API void* Prisma_AssetManager_GetAssetData(const char* path, size_t* outSize);
    ENGINE_API void  Prisma_AssetManager_FreeAssetData(void* data);
}

}  // namespace Prisma
