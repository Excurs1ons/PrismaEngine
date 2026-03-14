#pragma once

#include "ManagerBase.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/RenderDesc.h"
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace Prisma::Graphic {

class ITexture;
class IShader;
class IBuffer;
class IPipeline;
class ISampler;

struct ResourceLoadTask {
    enum Type { LoadTexture, LoadShader, LoadMesh } type;
    std::string path;
    std::string name;
    ResourceId id;
    std::function<void(ResourceId, std::shared_ptr<IResource>)> callback;
};

class ENGINE_API RenderResourceManager : public IResourceManager, public ManagerBase<RenderResourceManager> {
public:
    static std::shared_ptr<RenderResourceManager> Get();

    RenderResourceManager();
    ~RenderResourceManager() override;

    int Initialize() override;
    int Initialize(IRenderDevice* device) override;
    void Update(Timestep ts) override;
    void Shutdown() override;

    // === 纹理管理 ===
    std::shared_ptr<ITexture> LoadTexture(const std::string& filename, bool generateMips = true) override;
    std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc) override;
    std::shared_ptr<ITexture> CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) override;

    // === 缓冲区管理 ===
    std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) override;
    std::shared_ptr<IBuffer> CreateDynamicBuffer(uint64_t size, BufferType type) override;

    // === 着色器管理 ===
    std::shared_ptr<IShader> LoadShader(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines = {}) override;
    std::shared_ptr<IShader> CreateShader(const std::string& source, const ShaderDesc& desc) override;
    bool CompileShader(const ShaderDesc& desc, std::string* errors = nullptr) override;

    // === 管线管理 ===
    std::shared_ptr<IPipeline> CreatePipeline(const PipelineDesc& desc) override;
    std::shared_ptr<IPipeline> LoadPipeline(const std::string& filename) override;
    std::shared_ptr<IPipelineState> CreatePipelineState(const PipelineStateDesc& desc) override;
    
    // === 采样器管理 ===
    std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc) override;
    std::shared_ptr<ISampler> GetDefaultSampler() override;

    // === 生命周期管理 ===
    void ReleaseResource(ResourceId id) override;
    void ReleaseAllResources() override;
    void GarbageCollect() override;

    // === 异步加载 ===
    ResourceId LoadTextureAsync(const std::string& filename) override;
    ResourceId LoadShaderAsync(const std::string& filename) override;
    bool IsAsyncLoadingComplete(ResourceId id) override;

    // === 统计与调试 ===
    ResourceStats GetResourceStats() const override;
    void EnableHotReload(bool enable) override;
    void CheckAndReloadResources() override;

    // === 线程安全 ===
    std::shared_mutex& GetResourceLock() override;

    // === 内部辅助 ===
    template<typename T>
    std::shared_ptr<T> GetResource(const std::string& name) {
        std::shared_lock lock(m_resourceMutex);
        auto it = m_nameToId.find(name);
        if (it == m_nameToId.end()) {
            return nullptr;
        }

        auto resIt = m_resources.find(it->second);
        if (resIt == m_resources.end()) {
            return nullptr;
        }

        // 听着，如果你在这里传错了类型，那是你自己的问题。
        // 我们用 static_pointer_cast 追求极致速度，不要 RTTI 这种垃圾。
        return std::static_pointer_cast<T>(resIt->second);
    }
    
    void RegisterResource(std::shared_ptr<IResource> resource, const std::string& name = "");

private:
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

private:
    IRenderDevice* m_device;
    bool m_initialized;
    std::atomic<ResourceId> m_nextId;

    std::unordered_map<ResourceId, std::shared_ptr<IResource>> m_resources;
    std::unordered_map<std::string, ResourceId> m_nameToId;
    mutable std::shared_mutex m_resourceMutex;

    std::queue<ResourceLoadTask> m_loadQueue;
    std::mutex m_loadQueueMutex;
    std::condition_variable m_loadQueueCV;
    std::thread m_loadingThread;
    std::atomic<bool> m_shouldStopLoading;

    bool m_hotReloadEnabled;
    float m_hotReloadTimer;
    const float HOT_RELOAD_INTERVAL = 2.0f;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;

    mutable ResourceStats m_cachedStats;
    mutable std::atomic<bool> m_statsDirty{true};
    std::string m_cacheDirectory;

    std::shared_ptr<ISampler> m_defaultSampler;
};

} // namespace Prisma::Graphic