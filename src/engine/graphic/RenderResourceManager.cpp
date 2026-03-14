#include "RenderResourceManager.h"
#include "Logger.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IResourceFactory.h"
#include <chrono>
#include <fstream>
#include <algorithm>

namespace Prisma::Graphic {

std::shared_ptr<RenderResourceManager> RenderResourceManager::Get() {
    static std::shared_ptr<RenderResourceManager> instance = std::make_shared<RenderResourceManager>();
    return instance;
}

RenderResourceManager::RenderResourceManager() 
    : m_device(nullptr)
    , m_initialized(false)
    , m_nextId(1)
    , m_shouldStopLoading(false)
    , m_hotReloadEnabled(false)
    , m_hotReloadTimer(0.0f)
    , m_statsDirty(true)
{
    m_cacheDirectory = ".resource_cache";
}

RenderResourceManager::~RenderResourceManager() {
    Shutdown();
}

int RenderResourceManager::Initialize() {
    return 0;
}

int RenderResourceManager::Initialize(IRenderDevice* device) {
    if (!device) return 1;
    m_device = device;
    
    // 启动异步加载线程
    m_loadingThread = std::thread(&RenderResourceManager::LoadingThreadFunction, this);
    
    m_initialized = true;
    LOG_INFO("RenderResourceManager", "Resource manager initialized.");
    return true;
}

void RenderResourceManager::RegisterResource(std::shared_ptr<IResource> resource, const std::string& name) {
    if (!resource) return;

    ResourceId id = GenerateId();
    
    {
        std::unique_lock<std::shared_mutex> lock(m_resourceMutex);
        m_resources[id] = resource;
        if (!name.empty()) {
            m_nameToId[name] = id;
        }
    }
    
    m_statsDirty = true;
}

void RenderResourceManager::Update(Timestep ts) {
    if (!m_initialized) return;

    if (m_hotReloadEnabled) {
        m_hotReloadTimer += ts;
        if (m_hotReloadTimer >= HOT_RELOAD_INTERVAL) {
            CheckFileModifications();
            m_hotReloadTimer = 0.0f;
        }
    }
}

void RenderResourceManager::Shutdown() {
    m_shouldStopLoading = true;
    m_loadQueueCV.notify_all();
    if (m_loadingThread.joinable()) {
        m_loadingThread.join();
    }

    ReleaseAllResources();
    m_initialized = false;
}

std::shared_ptr<ITexture> RenderResourceManager::LoadTexture(const std::string& filename, bool generateMips) {
    auto res = GetResource<ITexture>(filename);
    if (res) return res;

    res = LoadTextureSync(filename, generateMips);
    if (res) {
        RegisterResource(res, filename);
        if (std::filesystem::exists(filename)) {
            m_fileTimestamps[filename] = std::filesystem::last_write_time(filename);
        }
    }
    return res;
}

std::shared_ptr<ITexture> RenderResourceManager::CreateTexture(const TextureDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto texture = m_device->GetResourceFactory()->CreateTextureImpl(desc);
    if (texture) {
        std::shared_ptr<ITexture> sharedRes = std::move(texture);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<ITexture> RenderResourceManager::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto texture = m_device->GetResourceFactory()->CreateTextureFromMemory(data, dataSize, desc);
    if (texture) {
        std::shared_ptr<ITexture> sharedRes = std::move(texture);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<IBuffer> RenderResourceManager::CreateBuffer(const BufferDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto buffer = m_device->GetResourceFactory()->CreateBufferImpl(desc);
    if (buffer) {
        std::shared_ptr<IBuffer> sharedRes = std::move(buffer);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<IBuffer> RenderResourceManager::CreateDynamicBuffer(uint64_t size, BufferType type) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto buffer = m_device->GetResourceFactory()->CreateDynamicBuffer(size, type, BufferUsage::Dynamic);
    if (buffer) {
        std::shared_ptr<IBuffer> sharedRes = std::move(buffer);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<IShader> RenderResourceManager::LoadShader(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines) {
    auto res = GetResource<IShader>(filename);
    if (res) return res;

    res = LoadShaderSync(filename, entryPoint, target, defines);
    if (res) {
        RegisterResource(res, filename);
        if (std::filesystem::exists(filename)) {
            m_fileTimestamps[filename] = std::filesystem::last_write_time(filename);
        }
    }
    return res;
}

std::shared_ptr<IShader> RenderResourceManager::CreateShader(const std::string& source, const ShaderDesc& desc) {
    (void)source; (void)desc; 
    return nullptr;
}

bool RenderResourceManager::CompileShader(const ShaderDesc& desc, std::string* errors) {
    (void)desc; (void)errors;
    return false;
}

std::shared_ptr<IPipeline> RenderResourceManager::CreatePipeline(const PipelineDesc& desc) {
    (void)desc;
    return nullptr;
}

std::shared_ptr<IPipelineState> RenderResourceManager::CreatePipelineState(const PipelineStateDesc& desc) {
    (void)desc;
    return nullptr;
}

std::shared_ptr<IPipeline> RenderResourceManager::LoadPipeline(const std::string& filename) {
    (void)filename;
    return nullptr;
}

std::shared_ptr<ISampler> RenderResourceManager::CreateSampler(const SamplerDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto sampler = m_device->GetResourceFactory()->CreateSamplerImpl(desc);
    if (sampler) {
        std::shared_ptr<ISampler> sharedRes = std::move(sampler);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<ISampler> RenderResourceManager::GetDefaultSampler() {
    if (!m_defaultSampler) {
        SamplerDesc desc;
        m_defaultSampler = CreateSampler(desc);
    }
    return m_defaultSampler;
}

void RenderResourceManager::ReleaseResource(ResourceId id) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);
    auto it = m_resources.find(id);
    if (it != m_resources.end()) {
        m_resources.erase(it);
        m_statsDirty = true;
    }
}

void RenderResourceManager::ReleaseAllResources() {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);
    m_resources.clear();
    m_nameToId.clear();
    m_fileTimestamps.clear();
    m_statsDirty = true;
}

void RenderResourceManager::GarbageCollect() {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);
    for (auto it = m_resources.begin(); it != m_resources.end(); ) {
        if (it->second.use_count() == 1) {
            for (auto nameIt = m_nameToId.begin(); nameIt != m_nameToId.end(); ) {
                if (nameIt->second == it->first) {
                    nameIt = m_nameToId.erase(nameIt);
                } else {
                    ++nameIt;
                }
            }
            it = m_resources.erase(it);
            m_statsDirty = true;
        } else {
            ++it;
        }
    }
}

ResourceId RenderResourceManager::LoadTextureAsync(const std::string& filename) {
    ResourceId id = GenerateId();
    ResourceLoadTask task;
    task.type = ResourceLoadTask::LoadTexture;
    task.path = filename;
    task.id = id;
    
    {
        std::lock_guard<std::mutex> lock(m_loadQueueMutex);
        m_loadQueue.push(task);
    }
    m_loadQueueCV.notify_one();
    return id;
}

ResourceId RenderResourceManager::LoadShaderAsync(const std::string& filename) {
    ResourceId id = GenerateId();
    ResourceLoadTask task;
    task.type = ResourceLoadTask::LoadShader;
    task.path = filename;
    task.id = id;

    {
        std::lock_guard<std::mutex> lock(m_loadQueueMutex);
        m_loadQueue.push(task);
    }
    m_loadQueueCV.notify_one();
    return id;
}

bool RenderResourceManager::IsAsyncLoadingComplete(ResourceId id) {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);
    return m_resources.find(id) != m_resources.end();
}

ResourceStats RenderResourceManager::GetResourceStats() const {
    if (m_statsDirty) UpdateResourceStats();
    return m_cachedStats;
}

void RenderResourceManager::EnableHotReload(bool enable) {
    m_hotReloadEnabled = enable;
    if (enable) UpdateFileTimestamps();
}

void RenderResourceManager::CheckAndReloadResources() {
    CheckFileModifications();
}

std::shared_mutex& RenderResourceManager::GetResourceLock() {
    return m_resourceMutex;
}

ResourceId RenderResourceManager::GenerateId() {
    return m_nextId++;
}

void RenderResourceManager::LoadingThreadFunction() {
    while (!m_shouldStopLoading) {
        ResourceLoadTask task;
        {
            std::unique_lock<std::mutex> lock(m_loadQueueMutex);
            m_loadQueueCV.wait(lock, [this]{ return !m_loadQueue.empty() || m_shouldStopLoading; });
            if (m_shouldStopLoading) break;
            task = m_loadQueue.front();
            m_loadQueue.pop();
        }
        ProcessLoadTask(task);
    }
}

void RenderResourceManager::ProcessLoadTask(const ResourceLoadTask& task) {
    std::shared_ptr<IResource> resource = nullptr;
    if (task.type == ResourceLoadTask::LoadTexture) {
        resource = LoadTextureSync(task.path, true);
    } else if (task.type == ResourceLoadTask::LoadShader) {
        resource = LoadShaderSync(task.path, "main", "ps_6_0", {});
    }

    if (resource) {
        std::unique_lock<std::shared_mutex> lock(m_resourceMutex);
        m_resources[task.id] = resource;
        if (!task.name.empty()) m_nameToId[task.name] = task.id;
        m_statsDirty = true;
    } else {
        LOG_ERROR("RenderResourceManager", "Failed to load resource asynchronously: {0}", task.path);
    }
}

std::shared_ptr<ITexture> RenderResourceManager::LoadTextureSync(const std::string& filename, bool generateMips) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    TextureDesc desc;
    std::vector<uint8_t> data;
    if (LoadImageFromFile(filename, data, desc)) {
        desc.mipLevels = generateMips ? 0 : 1;
        auto texture = m_device->GetResourceFactory()->CreateTextureFromMemory(data.data(), data.size(), desc);
        return std::shared_ptr<ITexture>(std::move(texture));
    }
    return nullptr;
}

std::shared_ptr<IShader> RenderResourceManager::LoadShaderSync(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    std::ifstream file(filename);
    if (!file.is_open()) return nullptr;
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    ShaderDesc desc;
    desc.entryPoint = entryPoint;
    desc.target = target;
    desc.defines = defines;
    return nullptr;
}

void RenderResourceManager::UpdateResourceStats() const {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);
    m_cachedStats = {};
    m_cachedStats.totalResources = static_cast<uint32_t>(m_resources.size());
    m_statsDirty = false;
}

void RenderResourceManager::UpdateFileTimestamps() {
    for (const auto& [name, id] : m_nameToId) {
        if (std::filesystem::exists(name)) {
            m_fileTimestamps[name] = std::filesystem::last_write_time(name);
        }
    }
}

void RenderResourceManager::CheckFileModifications() {
    for (auto it = m_fileTimestamps.begin(); it != m_fileTimestamps.end(); ++it) {
        if (std::filesystem::exists(it->first)) {
            auto currentTime = std::filesystem::last_write_time(it->first);
            if (currentTime > it->second) {
                it->second = currentTime;
            }
        }
    }
}

bool RenderResourceManager::LoadFromCache(const std::string& filename, std::vector<uint8_t>& data) {
    (void)filename; (void)data;
    return false; 
}

void RenderResourceManager::SaveToCache(const std::string& filename, const void* data, size_t size) {
    (void)filename; (void)data; (void)size;
}

bool RenderResourceManager::LoadImageFromFile(const std::string& filename, std::vector<uint8_t>& data, TextureDesc& desc) {
    (void)filename; (void)data; (void)desc;
    return false;
}

} // namespace Prisma::Graphic