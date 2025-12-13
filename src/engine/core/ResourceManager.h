#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <future>
#include <atomic>

namespace Engine {
namespace Core {

// 资源状态
enum class ResourceState : uint32_t {
    Unloaded,   // 未加载
    Loading,    // 加载中
    Loaded,     // 已加载
    Failed,     // 加载失败
    Unloading   // 卸载中
};

// 资源类型
enum class ResourceType : uint32_t {
    Unknown = 0,
    Texture,
    Mesh,
    Material,
    Shader,
    Audio,
    Animation,
    Scene,
    Script,
    Config,
    Font,
    Count
};

// 资源基类
class IResource {
public:
    virtual ~IResource() = default;

    // 获取资源路径
    virtual const std::string& GetPath() const = 0;

    // 获取资源类型
    virtual ResourceType GetType() const = 0;

    // 获取资源大小（字节）
    virtual uint64_t GetSize() const = 0;

    // 获取资源状态
    virtual ResourceState GetState() const = 0;

    // 加载资源
    virtual bool Load() = 0;

    // 卸载资源
    virtual void Unload() = 0;

    // 重新加载
    virtual bool Reload() = 0;

    // 检查是否有效
    virtual bool IsValid() const = 0;

    // 获取引用计数
    virtual uint32_t GetRefCount() const = 0;

    // 获取最后使用时间
    virtual double GetLastUsedTime() const = 0;

protected:
    // 设置资源状态
    virtual void SetState(ResourceState state) = 0;

    friend class ResourceManager;
};

// 资源加载器接口
class IResourceLoader {
public:
    virtual ~IResourceLoader() = default;

    // 支持的文件扩展名
    virtual std::vector<std::string> GetSupportedExtensions() const = 0;

    // 创建资源
    virtual std::shared_ptr<IResource> CreateResource(const std::string& path) = 0;

    // 异步加载资源
    virtual std::future<bool> LoadResourceAsync(std::shared_ptr<IResource> resource) = 0;
};

// 资源管理器
class ResourceManager {
public:
    static ResourceManager& GetInstance();

    // 注册资源加载器
    void RegisterLoader(ResourceType type, std::unique_ptr<IResourceLoader> loader);

    // 加载资源（同步）
    template<typename T>
    std::shared_ptr<T> Load(const std::string& path);

    // 加载资源（异步）
    template<typename T>
    std::future<std::shared_ptr<T>> LoadAsync(const std::string& path);

    // 通用资源加载
    std::shared_ptr<IResource> LoadResource(const std::string& path);
    std::future<std::shared_ptr<IResource>> LoadResourceAsync(const std::string& path);

    // 获取已加载的资源
    std::shared_ptr<IResource> GetResource(const std::string& path);

    // 释放资源
    void ReleaseResource(const std::string& path);

    // 预加载资源
    void Preload(const std::string& pattern);

    // 卸载未使用的资源
    void UnloadUnused();

    // 设置资源搜索路径
    void AddSearchPath(const std::string& path);

    // 清空搜索路径
    void ClearSearchPaths();

    // 获取资源统计
    struct ResourceStats {
        uint32_t totalResources = 0;
        uint32_t loadedResources = 0;
        uint32_t loadingResources = 0;
        uint32_t failedResources = 0;
        uint64_t totalMemoryUsage = 0;  // 字节
        uint64_t textureMemoryUsage = 0;
        uint64_t meshMemoryUsage = 0;
        uint64_t audioMemoryUsage = 0;
    };

    ResourceStats GetStats() const;

    // 设置内存限制
    void SetMemoryLimit(uint64_t limit);

    // 启用/禁用缓存
    void SetCacheEnabled(bool enabled);

    // 设置缓存大小限制
    void SetCacheSizeLimit(uint64_t size);

    // 清理缓存
    void ClearCache();

    // 保存资源
    bool SaveResource(const std::string& path, std::shared_ptr<IResource> resource);

    // 热重载
    void EnableHotReload(bool enable);

private:
    ResourceManager();
    ~ResourceManager();

    // 禁止拷贝
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // 更新统计
    void UpdateStats() const;

    // 查找资源路径
    std::string FindResourcePath(const std::string& relativePath) const;

    // LRU缓存清理
    void EvictLRU();

    // 线程安全
    mutable std::mutex m_mutex;

    // 资源缓存
    std::unordered_map<std::string, std::shared_ptr<IResource>> m_resources;

    // 资源加载器
    std::unordered_map<ResourceType, std::unique_ptr<IResourceLoader>> m_loaders;

    // 搜索路径
    std::vector<std::string> m_searchPaths;

    // 统计信息
    mutable ResourceStats m_stats;

    // 配置
    uint64_t m_memoryLimit = 1024 * 1024 * 1024;  // 1GB
    bool m_cacheEnabled = true;
    uint64_t m_cacheSizeLimit = 512 * 1024 * 1024;  // 512MB
    bool m_hotReloadEnabled = false;

    // 后台任务
    std::vector<std::future<void>> m_asyncTasks;
    std::atomic<bool> m_running{true};
};

// 资源智能指针包装器
template<typename T>
class ResourcePtr {
public:
    ResourcePtr() = default;
    ResourcePtr(std::shared_ptr<T> resource) : m_resource(resource) {}

    // 访问资源
    T* get() const { return static_cast<T*>(m_resource.get()); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }

    // 检查有效性
    bool IsValid() const { return m_resource && m_resource->IsValid(); }

    // 获取路径
    const std::string& GetPath() const { return m_resource ? m_resource->GetPath() : ""; }

    // 获取状态
    ResourceState GetState() const { return m_resource ? m_resource->GetState() : ResourceState::Unloaded; }

    // 显式转换
    explicit operator bool() const { return IsValid(); }

    // 重置
    void reset() { m_resource.reset(); }

private:
    std::shared_ptr<IResource> m_resource;
};

// 模板实现
template<typename T>
std::shared_ptr<T> ResourceManager::Load(const std::string& path) {
    auto resource = LoadResource(path);
    return std::static_pointer_cast<T>(resource);
}

template<typename T>
std::future<std::shared_ptr<T>> ResourceManager::LoadAsync(const std::string& path) {
    auto future = LoadResourceAsync(path);
    return std::async(std::launch::deferred, [future = std::move(future)]() mutable {
        auto resource = future.get();
        return std::static_pointer_cast<T>(resource);
    });
}

// 便利函数
inline ResourceManager& GetResourceManager() {
    return ResourceManager::GetInstance();
}

} // namespace Core
} // namespace Engine