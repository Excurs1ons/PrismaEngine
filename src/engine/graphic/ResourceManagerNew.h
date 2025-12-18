#pragma once

#include "../ManagerBase.h"
#include "interfaces/IResourceManager.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IPipelineState.h"
#include <unordered_map>
#include <shared_mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <filesystem>

namespace PrismaEngine::Graphic {

/// @brief 资源加载任务
struct ResourceLoadTask {
    enum Type {
        LoadTexture,
        LoadShader,
        LoadPipeline
    };

    Type type;
    std::string path;
    std::string name;
    ResourceId id;
    std::function<void(ResourceId, std::shared_ptr<IResource>)> callback;
};

/// @brief 资源管理器实现
class ResourceManager : public ::Engine::ManagerBase<ResourceManager>, public IResourceManager {
public:
    friend class ::Engine::ManagerBase<ResourceManager>;
    static constexpr std::string GetName() { return "ResourceManager"; }
public:
    ResourceManager();
    ~ResourceManager() override;

    // ISubSystem接口实现（来自ManagerBase）
    bool Initialize() override;
    void Update(float deltaTime) override;

    // IResourceManager接口实现
    bool Initialize(IRenderDevice* device);
    void Shutdown() override;

    // 纹理管理
    std::shared_ptr<ITexture> LoadTexture(const std::string& filename, bool generateMips) override;
    std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc) override;
    std::shared_ptr<ITexture> CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) override;

    // 缓冲区管理
    std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) override;
    std::shared_ptr<IBuffer> CreateDynamicBuffer(uint64_t size, BufferType type) override;

    // 着色器管理
    std::shared_ptr<IShader> LoadShader(const std::string& filename,
                                        const std::string& entryPoint,
                                        const std::string& target,
                                        const std::vector<std::string>& defines) override;
    std::shared_ptr<IShader> CreateShader(const std::string& source, const ShaderDesc& desc) override;
    bool CompileShader(const ShaderDesc& desc, std::string& errors); // 不 override，IResourceManager 没有这个方法

    // 管线管理
    std::shared_ptr<IPipelineState> CreatePipelineState(); // 不 override，IResourceManager 没有这个方法
    std::shared_ptr<IPipeline> LoadPipeline(const std::string& filename) override;

    // 采样器管理
    std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<ISampler> GetDefaultSampler() override;

    // 资源查询
    std::shared_ptr<IResource> GetResource(ResourceId id) override;
    std::shared_ptr<IResource> GetResource(const std::string& name) override;
    void ReleaseResource(ResourceId id) override;
    void GarbageCollect() override;
    void ReleaseAllResources() override;

    // 异步加载
    ResourceId LoadTextureAsync(const std::string& filename) override;
    ResourceId LoadShaderAsync(const std::string& filename) override;
    bool IsAsyncLoadingComplete(ResourceId id) override;

    // 统计
    ResourceStats GetResourceStats() const override;

    // 热重载
    void EnableHotReload(bool enable) override;
    void CheckAndReloadResources() override;

    // 线程安全
    std::shared_mutex& GetResourceLock() override;

private:
    // === 核心数据结构 ===

    IRenderDevice* m_device;
    bool m_initialized = false;
    mutable std::shared_mutex m_resourceMutex;

    // 资源存储
    std::unordered_map<ResourceId, std::shared_ptr<IResource>> m_resources;
    std::unordered_map<std::string, ResourceId> m_nameToId;
    std::queue<ResourceId> m_pendingDeletion;
    ResourceId m_nextId = 1;

    // 默认资源
    std::shared_ptr<ISampler> m_defaultSampler;

    // === 异步加载系统 ===

    std::thread m_loadingThread;
    std::queue<ResourceLoadTask> m_loadQueue;
    std::mutex m_loadQueueMutex;
    std::condition_variable m_loadQueueCV;
    std::atomic<bool> m_shouldStopLoading{false};

    // 热重载
    bool m_hotReloadEnabled = false;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;

    // === 内部方法 ===

    // 资源ID管理
    ResourceId GenerateId();
    void RegisterResource(std::shared_ptr<IResource> resource, const std::string& name = "");
    void UnregisterResource(ResourceId id);

    // 异步加载
    void LoadingThreadFunction();
    void ProcessLoadTask(const ResourceLoadTask& task);
    std::shared_ptr<ITexture> LoadTextureSync(const std::string& filename, bool generateMips);
    std::shared_ptr<IShader> LoadShaderSync(const std::string& filename,
                                            const std::string& entryPoint,
                                            const std::string& target,
                                            const std::vector<std::string>& defines);

    // 热重载
    void UpdateFileTimestamps();
    void CheckFileModifications();

    // 纹理加载辅助
    bool LoadImageFromFile(const std::string& filename,
                          std::vector<uint8_t>& imageData,
                          TextureDesc& desc);
    bool LoadDDSFromFile(const std::string& filename,
                         std::vector<uint8_t>& imageData,
                         TextureDesc& desc);
    bool LoadSTBIFromFile(const std::string& filename,
                          std::vector<uint8_t>& imageData,
                          TextureDesc& desc);

    // 着色器编译辅助
    bool CompileHLSLShader(const ShaderDesc& desc,
                          std::vector<uint8_t>& bytecode,
                          ShaderReflection& reflection,
                          std::string& errors);
    bool CompileGLSLShader(const ShaderDesc& desc,
                          std::vector<uint8_t>& bytecode,
                          ShaderReflection& reflection,
                          std::string& errors);

    // 统计和调试
    void UpdateResourceStats() const;
    mutable ResourceStats m_cachedStats;
    mutable std::atomic<bool> m_statsDirty{true};

    // 缓存管理
    struct CacheEntry {
        std::filesystem::path cachePath;
        uint64_t sourceHash;
        std::time_t lastAccess;
    };
    std::unordered_map<std::string, CacheEntry> m_cacheEntries;
    std::string m_cacheDirectory;

    uint64_t CalculateFileHash(const std::string& filename);
    std::string GetCachePath(const std::string& filename);
    bool LoadFromCache(const std::string& filename, std::vector<uint8_t>& data);
    void SaveToCache(const std::string& filename, const void* data, size_t size);
};

} // namespace PrismaEngine::Graphic