#pragma once

#include "ManagerBase.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResource.h"
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <filesystem>

namespace PrismaEngine::Graphic {

class ITexture;
class IBuffer;
class IShader;
class IPipeline;
class ISampler;

struct ResourceLoadTask {
    enum Type { LoadTexture, LoadShader, LoadMesh } type;
    std::string path;
    std::string name;
    ResourceId id;
    std::function<void(ResourceId, std::shared_ptr<IResource>)> callback;
};

class ENGINE_API ResourceManager : public IResourceManager, public ManagerBase<ResourceManager> {
public:
    ResourceManager();
    ~ResourceManager() override;

    bool Initialize() override;
    bool Initialize(IRenderDevice* device) override;
    void Update(float deltaTime) override;
    void Shutdown() override;

    // IResourceManager 接口实现
    std::shared_ptr<ITexture> LoadTexture(const std::string& filename, bool generateMips = true) override;
    std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc) override;
    std::shared_ptr<ITexture> CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) override;

    std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) override;
    std::shared_ptr<IBuffer> CreateDynamicBuffer(uint64_t size, BufferType type) override;

    std::shared_ptr<IShader> LoadShader(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines = {}) override;
    std::shared_ptr<IShader> CreateShader(const std::string& source, const ShaderDesc& desc) override;
    bool CompileShader(const ShaderDesc& desc, std::string* errors = nullptr) override;

    std::shared_ptr<IPipeline> CreatePipeline(const PipelineDesc& desc) override;
    std::shared_ptr<IPipeline> LoadPipeline(const std::string& filename) override;
    std::shared_ptr<IPipelineState> CreatePipelineState(const PipelineStateDesc& desc) override;
    
    std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<ISampler> GetDefaultSampler() override;

    // 模板资源获取
    template<typename T>
    std::shared_ptr<T> GetResource(ResourceId id) {
        std::shared_lock lock(m_resourceMutex);
        auto it = m_resources.find(id);
        if (it != m_resources.end()) {
            return std::dynamic_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    template<typename T>
    std::shared_ptr<T> GetResource(const std::string& name) {
        std::shared_lock lock(m_resourceMutex);
        auto it = m_nameToId.find(name);
        if (it != m_nameToId.end()) {
            return GetResource<T>(it->second);
        }
        return nullptr;
    }

    void ReleaseResource(ResourceId id) override;
    void ReleaseAllResources() override;
    void GarbageCollect() override;

    ResourceId LoadTextureAsync(const std::string& filename) override;
    ResourceId LoadShaderAsync(const std::string& filename) override;
    bool IsAsyncLoadingComplete(ResourceId id) override;

    ResourceStats GetResourceStats() const override;
    void EnableHotReload(bool enable) override;
    void CheckAndReloadResources() override;

    std::shared_mutex& GetResourceLock() override;

private:
    template<typename T>
    void RegisterResource(std::shared_ptr<T> resource, const std::string& name = "") {
        if (!resource) return;
        std::unique_lock lock(m_resourceMutex);
        ResourceId id = GenerateId();
        m_resources[id] = std::static_pointer_cast<IResource>(resource);
        if (!name.empty()) m_nameToId[name] = id;
        m_statsDirty = true;
    }

    ResourceId GenerateId();
    void LoadingThreadFunction();
    void ProcessLoadTask(const ResourceLoadTask& task);
    std::shared_ptr<ITexture> LoadTextureSync(const std::string& filename, bool generateMips);
    std::shared_ptr<IShader> LoadShaderSync(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines);

    void UpdateResourceStats() const;
    void UpdateFileTimestamps();
    void CheckFileModifications();
    
    bool LoadFromCache(const std::string& filename, std::vector<uint8_t>& data);
    void SaveToCache(const std::string& filename, const void* data, size_t size);
    bool LoadImageFromFile(const std::string& filename, std::vector<uint8_t>& data, TextureDesc& desc);

    IRenderDevice* m_device;
    bool m_initialized = false;

    // 资源存储
    std::unordered_map<ResourceId, std::shared_ptr<IResource>> m_resources;
    std::unordered_map<std::string, ResourceId> m_nameToId;
    std::queue<ResourceId> m_pendingDeletion;
    mutable std::shared_mutex m_resourceMutex;
    ResourceId m_nextId = 1;

    // 默认资源
    std::shared_ptr<ISampler> m_defaultSampler;

    // 异步加载
    std::thread m_loadingThread;
    std::queue<ResourceLoadTask> m_loadQueue;
    std::mutex m_loadQueueMutex;
    std::condition_variable m_loadQueueCV;
    std::atomic<bool> m_shouldStopLoading{false};

    // 热重载
    bool m_hotReloadEnabled = false;
    float m_hotReloadTimer = 0.0f;
    static constexpr float HOT_RELOAD_INTERVAL = 2.0f;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;

    // 统计
    mutable ResourceStats m_cachedStats;
    mutable std::atomic<bool> m_statsDirty{true};
    std::string m_cacheDirectory;
};

} // namespace PrismaEngine::Graphic