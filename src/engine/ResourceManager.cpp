// Windows platform must define NOMINMAX to prevent pollution from Windows.h
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "ResourceManager.h"
#include "Engine.h"
#include "Logger.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResource.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IFence.h"

#include <fstream>
#include <chrono>
#include <algorithm>
#include <filesystem>

#if defined(_WIN32)
#include <Windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#else
#include <sys/stat.h>
#endif

namespace PrismaEngine::Graphic {

ResourceManager::ResourceManager()
    : m_device(nullptr)
    , m_cacheDirectory("cache/resources") {
    std::filesystem::create_directories(m_cacheDirectory);
}

ResourceManager::~ResourceManager() {
    Shutdown();
}

bool ResourceManager::Initialize() {
    return true; 
}

void ResourceManager::Update(float deltaTime) {
    if (!m_initialized) return;

    if (m_hotReloadEnabled) {
        m_hotReloadTimer += deltaTime;
        if (m_hotReloadTimer >= HOT_RELOAD_INTERVAL) {
            CheckAndReloadResources();
            m_hotReloadTimer = 0.0f;
        }
    }

    static float gcTimer = 0.0f;
    gcTimer += deltaTime;
    if (gcTimer >= 10.0f) {
        GarbageCollect();
        gcTimer = 0.0f;
    }
}

bool ResourceManager::Initialize(IRenderDevice* device) {
    if (!device) {
        LOG_ERROR("Resource", "Device pointer is null, failed to initialize.");
        return false;
    }

    m_device = device;
    m_initialized = true;

    SamplerDesc defaultSamplerDesc;
    defaultSamplerDesc.filter = TextureFilter::Linear;
    defaultSamplerDesc.addressU = TextureAddressMode::Wrap;
    defaultSamplerDesc.addressV = TextureAddressMode::Wrap;
    defaultSamplerDesc.addressW = TextureAddressMode::Wrap;
    m_defaultSampler = CreateSampler(defaultSamplerDesc);

    m_shouldStopLoading = false;
    m_loadingThread = std::thread(&ResourceManager::LoadingThreadFunction, this);

    LOG_INFO("Resource", "Resource manager initialized successfully.");
    return true;
}

void ResourceManager::Shutdown() {
    if (!m_initialized) return;

    m_shouldStopLoading = true;
    m_loadQueueCV.notify_all();
    if (m_loadingThread.joinable()) {
        m_loadingThread.join();
    }

    ReleaseAllResources();
    m_device = nullptr;
    m_initialized = false;
    LOG_INFO("Resource", "Resource manager shut down.");
}

std::shared_ptr<ITexture> ResourceManager::LoadTexture(const std::string& filename, bool generateMips) {
    std::unique_lock lock(m_resourceMutex);
    auto it = m_nameToId.find(filename);
    if (it != m_nameToId.end()) {
        auto res = m_resources.find(it->second);
        if (res != m_resources.end()) {
            return std::dynamic_pointer_cast<ITexture>(res->second);
        }
    }

    auto texture = LoadTextureSync(filename, generateMips);
    if (texture) {
        RegisterResource(texture, filename);
    }
    return texture;
}

std::shared_ptr<ITexture> ResourceManager::CreateTexture(const TextureDesc& desc) {
    if (!m_device) return nullptr;
    auto factory = m_device->GetResourceFactory();
    if (!factory) return nullptr;

    auto textureUnique = factory->CreateTextureImpl(desc);
    if (textureUnique) {
        std::shared_ptr<ITexture> texture = std::move(textureUnique);
        RegisterResource(texture, desc.name);
        return texture;
    }
    return nullptr;
}

std::shared_ptr<ITexture> ResourceManager::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    auto texture = CreateTexture(desc);
    if (texture && data && dataSize > 0) {
        LOG_INFO("Resource", "Created texture from memory: {0}, size: {1} bytes", desc.name, dataSize);
    }
    return texture;
}

std::shared_ptr<IBuffer> ResourceManager::CreateBuffer(const BufferDesc& desc) {
    if (!m_device) return nullptr;
    auto factory = m_device->GetResourceFactory();
    if (!factory) return nullptr;

    auto bufferUnique = factory->CreateBufferImpl(desc);
    if (bufferUnique) {
        std::shared_ptr<IBuffer> buffer = std::move(bufferUnique);
        RegisterResource(buffer);
        return buffer;
    }
    return nullptr;
}

std::shared_ptr<IBuffer> ResourceManager::CreateDynamicBuffer(uint64_t size, BufferType type) {
    BufferDesc desc;
    desc.size = size;
    desc.type = type;
    desc.usage = BufferUsage::Dynamic;
    return CreateBuffer(desc);
}

std::shared_ptr<IShader> ResourceManager::LoadShader(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines) {
    std::string cacheKey = filename + "|" + entryPoint + "|" + target;
    std::shared_lock lock(m_resourceMutex);
    auto it = m_nameToId.find(cacheKey);
    if (it != m_nameToId.end()) {
        auto res = m_resources.find(it->second);
        if (res != m_resources.end()) {
            return std::dynamic_pointer_cast<IShader>(res->second);
        }
    }
    lock.unlock();

    auto shader = LoadShaderSync(filename, entryPoint, target, defines);
    if (shader) {
        RegisterResource(shader, cacheKey);
    }
    return shader;
}

std::shared_ptr<IShader> ResourceManager::CreateShader(const std::string& source, const ShaderDesc& desc) {
    if (!m_device) return nullptr;
    auto factory = m_device->GetResourceFactory();
    if (!factory) return nullptr;

    std::vector<uint8_t> bytecode;
    ShaderReflection reflection;
    
    LOG_INFO("Resource", "Creating shader: {0}", desc.name);

    auto shaderUnique = factory->CreateShaderImpl(desc, bytecode, reflection);
    if (shaderUnique) {
        std::shared_ptr<IShader> shader = std::move(shaderUnique);
        RegisterResource(shader, desc.name);
        return shader;
    }
    return nullptr;
}

std::shared_ptr<IPipeline> ResourceManager::LoadPipeline(const std::string& filename) {
    LOG_INFO("Resource", "Loading pipeline: {0}", filename);
    return nullptr;
}

std::shared_ptr<ISampler> ResourceManager::CreateSampler(const SamplerDesc& desc) {
    if (!m_device) return nullptr;
    auto factory = m_device->GetResourceFactory();
    if (!factory) return nullptr;

    auto samplerUnique = factory->CreateSamplerImpl(desc);
    if (samplerUnique) {
        std::shared_ptr<ISampler> sampler = std::move(samplerUnique);
        RegisterResource(sampler);
        return sampler;
    }
    return nullptr;
}

std::shared_ptr<ISampler> ResourceManager::GetDefaultSampler() {
    return m_defaultSampler;
}

void ResourceManager::ReleaseResource(ResourceId id) {
    std::unique_lock lock(m_resourceMutex);
    m_pendingDeletion.push(id);
}

void ResourceManager::GarbageCollect() {
    std::unique_lock lock(m_resourceMutex);
    while (!m_pendingDeletion.empty()) {
        ResourceId id = m_pendingDeletion.front();
        m_pendingDeletion.pop();
        
        auto it = m_resources.find(id);
        if (it != m_resources.end()) {
            for (auto nameIt = m_nameToId.begin(); nameIt != m_nameToId.end();) {
                if (nameIt->second == id) {
                    nameIt = m_nameToId.erase(nameIt);
                } else {
                    ++nameIt;
                }
            }
            m_resources.erase(it);
        }
    }
    m_statsDirty = true;
}

void ResourceManager::ReleaseAllResources() {
    std::unique_lock lock(m_resourceMutex);
    m_resources.clear();
    m_nameToId.clear();
    while(!m_pendingDeletion.empty()) m_pendingDeletion.pop();
    m_statsDirty = true;
}

ResourceId ResourceManager::LoadTextureAsync(const std::string& filename) {
    ResourceId id = GenerateId();
    ResourceLoadTask task{ ResourceLoadTask::LoadTexture, filename, filename, id, nullptr };
    {
        std::lock_guard<std::mutex> lock(m_loadQueueMutex);
        m_loadQueue.push(task);
    }
    m_loadQueueCV.notify_one();
    return id;
}

ResourceId ResourceManager::LoadShaderAsync(const std::string& filename) {
    ResourceId id = GenerateId();
    ResourceLoadTask task{ ResourceLoadTask::LoadShader, filename, filename, id, nullptr };
    {
        std::lock_guard<std::mutex> lock(m_loadQueueMutex);
        m_loadQueue.push(task);
    }
    m_loadQueueCV.notify_one();
    return id;
}

bool ResourceManager::IsAsyncLoadingComplete(ResourceId id) {
    std::shared_lock lock(m_resourceMutex);
    return m_resources.find(id) != m_resources.end();
}

ResourceManager::ResourceStats ResourceManager::GetResourceStats() const {
    if (m_statsDirty) UpdateResourceStats();
    return m_cachedStats;
}

void ResourceManager::EnableHotReload(bool enable) {
    m_hotReloadEnabled = enable;
    if (enable) UpdateFileTimestamps();
}

void ResourceManager::CheckAndReloadResources() {
    if (!m_hotReloadEnabled) return;
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
            std::unique_lock<std::mutex> lock(m_loadQueueMutex);
            m_loadQueueCV.wait(lock, [this] { return !m_loadQueue.empty() || m_shouldStopLoading; });
            
            if (m_shouldStopLoading && m_loadQueue.empty()) break;
            
            if (!m_loadQueue.empty()) {
                task = m_loadQueue.front();
                m_loadQueue.pop();
            }
        }
        
        if (!task.path.empty()) {
            ProcessLoadTask(task);
        }
    }
}

void ResourceManager::ProcessLoadTask(const ResourceLoadTask& task) {
    std::shared_ptr<IResource> resource;
    if (task.type == ResourceLoadTask::LoadTexture) {
        auto texture = LoadTextureSync(task.path, true);
        if (texture) {
            resource = std::static_pointer_cast<IResource>(texture);
        }
    } else if (task.type == ResourceLoadTask::LoadShader) {
        auto shader = LoadShaderSync(task.path, "main", "vs_5_0", {});
        if (shader) {
            resource = std::static_pointer_cast<IResource>(shader);
        }
    }

    if (resource) {
        std::unique_lock lock(m_resourceMutex);
        m_resources[task.id] = resource;
        m_nameToId[task.name] = task.id;
        if (task.callback) task.callback(task.id, resource);
    }
}

std::shared_ptr<ITexture> ResourceManager::LoadTextureSync(const std::string& filename, bool generateMips) {
    std::vector<uint8_t> imageData;
    TextureDesc desc;
    desc.name = filename;

    if (LoadFromCache(filename, imageData)) {
        return CreateTexture(desc);
    }

    if (LoadImageFromFile(filename, imageData, desc)) {
        SaveToCache(filename, imageData.data(), imageData.size());
        auto texture = CreateTexture(desc);
        if (texture && generateMips) {
            texture->GenerateMips();
        }
        return texture;
    }
    return nullptr;
}

std::shared_ptr<IShader> ResourceManager::LoadShaderSync(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines) {
    ShaderDesc desc;
    desc.name = filename;
    desc.entryPoint = entryPoint;
    desc.target = target;
    desc.defines = defines;
    return CreateShader("", desc);
}

void ResourceManager::UpdateResourceStats() const {
    std::shared_lock lock(m_resourceMutex);
    m_cachedStats = {};
    m_cachedStats.totalResources = static_cast<uint32_t>(m_resources.size());
    for (auto const& [id, res] : m_resources) {
        m_cachedStats.totalMemoryUsage += res->GetSize();
    }
    m_statsDirty = false;
}

void ResourceManager::UpdateFileTimestamps() {}
void ResourceManager::CheckFileModifications() {}
bool ResourceManager::LoadFromCache(const std::string& filename, std::vector<uint8_t>& data) { return false; }
void ResourceManager::SaveToCache(const std::string& filename, const void* data, size_t size) {}
bool ResourceManager::LoadImageFromFile(const std::string& filename, std::vector<uint8_t>& data, TextureDesc& desc) { return false; }

} // namespace PrismaEngine::Graphic