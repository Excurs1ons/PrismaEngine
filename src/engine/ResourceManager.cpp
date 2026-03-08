#include "ResourceManager.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IResourceFactory.h"
#include <chrono>
#include <fstream>
#include <algorithm>

namespace PrismaEngine::Graphic {

ResourceManager::ResourceManager() 
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

ResourceManager::~ResourceManager() {
    Shutdown();
}

bool ResourceManager::Initialize() {
    return true;
}

bool ResourceManager::Initialize(IRenderDevice* device) {
    if (!device) return false;
    m_device = device;
    
    // 启动异步加载线程
    m_loadingThread = std::thread(&ResourceManager::LoadingThreadFunction, this);
    
    m_initialized = true;
    return true;
}

void ResourceManager::Update(float deltaTime) {
    if (!m_initialized) return;

    // 热重载定时检查
    if (m_hotReloadEnabled) {
        m_hotReloadTimer += deltaTime;
        if (m_hotReloadTimer >= HOT_RELOAD_INTERVAL) {
            CheckFileModifications();
            m_hotReloadTimer = 0.0f;
        }
    }

    // 更新统计信息（如果需要）
    if (m_statsDirty) {
        UpdateResourceStats();
    }
}

void ResourceManager::Shutdown() {
    m_shouldStopLoading = true;
    m_loadQueueCV.notify_all();
    if (m_loadingThread.joinable()) {
        m_loadingThread.join();
    }

    ReleaseAllResources();
    m_initialized = false;
}

std::shared_ptr<ITexture> ResourceManager::LoadTexture(const std::string& filename, bool generateMips) {
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

std::shared_ptr<ITexture> ResourceManager::CreateTexture(const TextureDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto texture = m_device->GetResourceFactory()->CreateTextureImpl(desc);
    if (texture) {
        std::shared_ptr<ITexture> sharedRes = std::move(texture);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<ITexture> ResourceManager::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto texture = m_device->GetResourceFactory()->CreateTextureFromMemory(data, dataSize, desc);
    if (texture) {
        std::shared_ptr<ITexture> sharedRes = std::move(texture);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<IBuffer> ResourceManager::CreateBuffer(const BufferDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto buffer = m_device->GetResourceFactory()->CreateBufferImpl(desc);
    if (buffer) {
        std::shared_ptr<IBuffer> sharedRes = std::move(buffer);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<IBuffer> ResourceManager::CreateDynamicBuffer(uint64_t size, BufferType type) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto buffer = m_device->GetResourceFactory()->CreateDynamicBuffer(size, type, BufferUsage::Dynamic);
    if (buffer) {
        std::shared_ptr<IBuffer> sharedRes = std::move(buffer);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<IShader> ResourceManager::LoadShader(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines) {
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

std::shared_ptr<IShader> ResourceManager::CreateShader(const std::string& source, const ShaderDesc& desc) {
    // 资源管理器不直接负责着色器编译逻辑，这应该由渲染后端在 CreateShaderImpl 中或者通过辅助工具完成
    // 此处调用工厂方法，假设后端已处理好或提供了特定实现
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    
    // 注意：IResourceFactory::CreateShaderImpl 需要字节码，
    // 此处暂存源码并标记需要编译，或调用后端的编译方法
    (void)source; (void)desc; 
    return nullptr;
}

bool ResourceManager::CompileShader(const ShaderDesc& desc, std::string* errors) {
    (void)desc; (void)errors;
    return false;
}

std::shared_ptr<IPipeline> ResourceManager::CreatePipeline(const PipelineDesc& desc) {
    (void)desc;
    return nullptr;
}

std::shared_ptr<IPipelineState> ResourceManager::CreatePipelineState(const PipelineStateDesc& desc) {
    (void)desc;
    return nullptr;
}

std::shared_ptr<IPipeline> ResourceManager::LoadPipeline(const std::string& filename) {
    (void)filename;
    return nullptr;
}

std::shared_ptr<ISampler> ResourceManager::CreateSampler(const SamplerDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    auto sampler = m_device->GetResourceFactory()->CreateSamplerImpl(desc);
    if (sampler) {
        std::shared_ptr<ISampler> sharedRes = std::move(sampler);
        RegisterResource(sharedRes);
        return sharedRes;
    }
    return nullptr;
}

std::shared_ptr<ISampler> ResourceManager::GetDefaultSampler() {
    if (!m_defaultSampler) {
        SamplerDesc desc;
        m_defaultSampler = CreateSampler(desc);
    }
    return m_defaultSampler;
}

void ResourceManager::ReleaseResource(ResourceId id) {
    std::unique_lock lock(m_resourceMutex);
    auto it = m_resources.find(id);
    if (it != m_resources.end()) {
        m_resources.erase(it);
        m_statsDirty = true;
    }
}

void ResourceManager::ReleaseAllResources() {
    std::unique_lock lock(m_resourceMutex);
    m_resources.clear();
    m_nameToId.clear();
    m_fileTimestamps.clear();
    m_statsDirty = true;
}

void ResourceManager::GarbageCollect() {
    std::unique_lock lock(m_resourceMutex);
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

ResourceId ResourceManager::LoadTextureAsync(const std::string& filename) {
    ResourceId id = GenerateId();
    ResourceLoadTask task;
    task.type = ResourceLoadTask::LoadTexture;
    task.path = filename;
    task.id = id;
    
    {
        std::lock_guard lock(m_loadQueueMutex);
        m_loadQueue.push(task);
    }
    m_loadQueueCV.notify_one();
    return id;
}

ResourceId ResourceManager::LoadShaderAsync(const std::string& filename) {
    ResourceId id = GenerateId();
    ResourceLoadTask task;
    task.type = ResourceLoadTask::LoadShader;
    task.path = filename;
    task.id = id;

    {
        std::lock_guard lock(m_loadQueueMutex);
        m_loadQueue.push(task);
    }
    m_loadQueueCV.notify_one();
    return id;
}

bool ResourceManager::IsAsyncLoadingComplete(ResourceId id) {
    std::shared_lock lock(m_resourceMutex);
    return m_resources.find(id) != m_resources.end();
}

ResourceStats ResourceManager::GetResourceStats() const {
    if (m_statsDirty) UpdateResourceStats();
    return m_cachedStats;
}

void ResourceManager::EnableHotReload(bool enable) {
    m_hotReloadEnabled = enable;
    if (enable) UpdateFileTimestamps();
}

void ResourceManager::CheckAndReloadResources() {
    CheckFileModifications();
}

std::shared_mutex& ResourceManager::GetResourceLock() {
    return m_resourceMutex;
}

ResourceId ResourceManager::GenerateId() {
    return m_nextId++;
}

void ResourceManager::LoadingThreadFunction() {
    while (!m_shouldStopLoading) {
        ResourceLoadTask task;
        {
            std::unique_lock lock(m_loadQueueMutex);
            m_loadQueueCV.wait(lock, [this]{ return !m_loadQueue.empty() || m_shouldStopLoading; });
            if (m_shouldStopLoading) break;
            task = m_loadQueue.front();
            m_loadQueue.pop();
        }
        ProcessLoadTask(task);
    }
}

void ResourceManager::ProcessLoadTask(const ResourceLoadTask& task) {
    std::shared_ptr<IResource> resource = nullptr;
    if (task.type == ResourceLoadTask::LoadTexture) {
        resource = LoadTextureSync(task.path, true);
    } else if (task.type == ResourceLoadTask::LoadShader) {
        resource = LoadShaderSync(task.path, "main", "ps_6_0", {});
    }

    if (resource) {
        std::unique_lock lock(m_resourceMutex);
        m_resources[task.id] = resource;
        if (!task.name.empty()) m_nameToId[task.name] = task.id;
        m_statsDirty = true;
    }
}

std::shared_ptr<ITexture> ResourceManager::LoadTextureSync(const std::string& filename, bool generateMips) {
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

std::shared_ptr<IShader> ResourceManager::LoadShaderSync(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    std::ifstream file(filename);
    if (!file.is_open()) return nullptr;
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    ShaderDesc desc;
    desc.entryPoint = entryPoint;
    desc.target = target;
    desc.defines = defines;
    // 此处需要字节码，暂留空或由工厂处理
    return nullptr;
}

void ResourceManager::UpdateResourceStats() const {
    std::shared_lock lock(m_resourceMutex);
    m_cachedStats = {};
    m_cachedStats.totalResources = static_cast<uint32_t>(m_resources.size());
    m_statsDirty = false;
}

void ResourceManager::UpdateFileTimestamps() {
    for (const auto& [name, id] : m_nameToId) {
        if (std::filesystem::exists(name)) {
            m_fileTimestamps[name] = std::filesystem::last_write_time(name);
        }
    }
}

void ResourceManager::CheckFileModifications() {
    for (auto it = m_fileTimestamps.begin(); it != m_fileTimestamps.end(); ++it) {
        if (std::filesystem::exists(it->first)) {
            auto currentTime = std::filesystem::last_write_time(it->first);
            if (currentTime > it->second) {
                it->second = currentTime;
            }
        }
    }
}

bool ResourceManager::LoadFromCache(const std::string& filename, std::vector<uint8_t>& data) {
    (void)filename; (void)data;
    return false; 
}

void ResourceManager::SaveToCache(const std::string& filename, const void* data, size_t size) {
    (void)filename; (void)data; (void)size;
}

bool ResourceManager::LoadImageFromFile(const std::string& filename, std::vector<uint8_t>& data, TextureDesc& desc) {
    (void)filename; (void)data; (void)desc;
    return false;
}

} // namespace PrismaEngine::Graphic