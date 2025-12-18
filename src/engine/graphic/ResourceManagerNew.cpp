#include "ResourceManagerNew.h"
#include "Logger.h"
#include "../Engine.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "interfaces/IShader.h"
#include "interfaces/IResource.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IResourceFactory.h"

// TODO: Implement image loading without stb_image
// #define STB_IMAGE_IMPLEMENTATION
// #include <stb/stb_image.h>
//
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include <stb/stb_image_write.h>

#include <fstream>
#include <chrono>
#include <algorithm>

#if defined(_WIN32)
#include <Windows.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <wrl/client.h>
namespace fs = std::filesystem;
#else
#include <sys/stat.h>
#endif

namespace PrismaEngine::Graphic {

ResourceManager::ResourceManager()
    : m_device(nullptr)
    , m_cacheDirectory("cache/resources") {

    // 创建缓存目录
    fs::create_directories(m_cacheDirectory);
}

ResourceManager::~ResourceManager() {
    Shutdown();
}

// ISubSystem 接口实现
bool ResourceManager::Initialize() {
    // 使用默认设备初始化
    // 这里可能需要从全局或上下文获取设备
    return true; // 临时返回true
}

void ResourceManager::Update(float deltaTime) {
    // 检查热重载
    if (m_hotReloadEnabled) {
        CheckAndReloadResources();
    }

    // 清理待删除的资源
    GarbageCollect();
}

// IResourceManager 接口实现
bool ResourceManager::Initialize(IRenderDevice* device) {
    if (!device) {
        LOG_ERROR("Resource", "设备指针为空，无法初始化资源管理器");
        return false;
    }

    m_device = device;
    m_initialized = true;

    // 创建默认采样器
    SamplerDesc defaultSamplerDesc;
    defaultSamplerDesc.filter = TextureFilter::Linear;
    defaultSamplerDesc.addressU = TextureAddressMode::Wrap;
    defaultSamplerDesc.addressV = TextureAddressMode::Wrap;
    defaultSamplerDesc.addressW = TextureAddressMode::Wrap;
    m_defaultSampler = CreateSampler(defaultSamplerDesc);

    // 启动异步加载线程
    m_loadingThread = std::thread(&ResourceManager::LoadingThreadFunction, this);

    LOG_INFO("Resource", "资源管理器初始化完成");
    return true;
}

void ResourceManager::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // 停止异步加载线程
    m_shouldStopLoading = true;
    m_loadQueueCV.notify_all();
    if (m_loadingThread.joinable()) {
        m_loadingThread.join();
    }

    // 清理所有资源
    ReleaseAllResources();

    m_device = nullptr;
    m_initialized = false;

    LOG_INFO("Resource", "资源管理器已关闭");
}

std::shared_ptr<ITexture> ResourceManager::LoadTexture(const std::string& filename, bool generateMips) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    // 检查是否已加载
    auto it = m_nameToId.find(filename);
    if (it != m_nameToId.end()) {
        auto resource = GetResource(it->second);
        if (resource) {
            return std::static_pointer_cast<ITexture>(resource);
        }
    }

    // 同步加载纹理
    auto texture = LoadTextureSync(filename, generateMips);
    if (texture) {
        RegisterResource(texture, filename);
    }

    return texture;
}

std::shared_ptr<ITexture> ResourceManager::CreateTexture(const TextureDesc& desc) {
    if (!m_device) {
        LOG_ERROR("Resource", "设备未初始化");
        return nullptr;
    }

    // 使用设备创建纹理
    // 这里需要实现具体逻辑
    LOG_INFO("Resource", "创建纹理: {0}x{1}", desc.width, desc.height);

    // 暂时返回nullptr，需要在适配器中实现
    return nullptr;
}

std::shared_ptr<ITexture> ResourceManager::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    if (!m_device) {
        LOG_ERROR("Resource", "设备未初始化");
        return nullptr;
    }

    auto factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("Resource", "无法获取资源工厂");
        return nullptr;
    }

    auto textureUnique = factory->CreateTextureImpl(desc);
    std::shared_ptr<ITexture> texture;
    if (textureUnique) {
        texture = std::move(textureUnique);
        // TODO: 从内存数据初始化纹理
    }

    if (!texture) {
        LOG_ERROR("Resource", "从内存创建纹理失败");
        return nullptr;
    }

    RegisterResource(texture);
    return texture;
}

std::shared_ptr<IBuffer> ResourceManager::CreateBuffer(const BufferDesc& desc) {
    if (!m_device) {
        LOG_ERROR("Resource", "设备未初始化");
        return nullptr;
    }

    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    auto factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("Resource", "无法获取资源工厂");
        return nullptr;
    }

    auto bufferUnique = factory->CreateBufferImpl(desc);
    std::shared_ptr<IBuffer> buffer;
    if (bufferUnique) {
        buffer = std::move(bufferUnique);
        // TODO: 初始化缓冲区
    }

    if (!buffer) {
        LOG_ERROR("Resource", "创建缓冲区失败");
        return nullptr;
    }

    RegisterResource(buffer);
    return buffer;
}

std::shared_ptr<IBuffer> ResourceManager::CreateDynamicBuffer(uint64_t size, BufferType type) {
    BufferDesc desc;
    desc.type = type;
    desc.size = size;
    desc.usage = BufferUsage::Dynamic;

    return CreateBuffer(desc);
}

std::shared_ptr<IShader> ResourceManager::LoadShader(const std::string& filename,
                                                    const std::string& entryPoint,
                                                    const std::string& target,
                                                    const std::vector<std::string>& defines) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    // 检查是否已加载
    std::string cacheKey = filename + ":" + entryPoint + ":" + target;
    for (const auto& define : defines) {
        cacheKey += ":" + define;
    }

    auto it = m_nameToId.find(cacheKey);
    if (it != m_nameToId.end()) {
        auto resource = GetResource(it->second);
        if (resource) {
            return std::static_pointer_cast<IShader>(resource);
        }
    }

    // 加载着色器
    auto shader = LoadShaderSync(filename, entryPoint, target, defines);
    if (shader) {
        RegisterResource(shader, cacheKey);
    }

    return shader;
}

std::shared_ptr<IShader> ResourceManager::CreateShader(const std::string& source, const ShaderDesc& desc) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    std::vector<uint8_t> bytecode;
    ShaderReflection reflection;
    std::string errors;

    // 编译着色器
    bool success = false;
    if (desc.language == ShaderLanguage::HLSL) {
        success = CompileHLSLShader(desc, bytecode, reflection, errors);
    } else if (desc.language == ShaderLanguage::GLSL) {
        success = CompileGLSLShader(desc, bytecode, reflection, errors);
    }

    if (!success) {
        LOG_ERROR("Resource", "着色器编译失败: {0}", errors);
        return nullptr;
    }

    // 创建着色器对象
    auto factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("Resource", "无法获取资源工厂");
        return nullptr;
    }

    // TODO: 提供编译后的字节码和反射信息
    // 目前使用空的字节码和反射信息
    std::vector<uint8_t> emptyBytecode;
    ShaderReflection emptyReflection = {};

    auto shaderUnique = factory->CreateShaderImpl(desc, emptyBytecode, emptyReflection);
    std::shared_ptr<IShader> shader;
    if (shaderUnique) {
        shader = std::move(shaderUnique);
        // TODO: 使用编译结果初始化着色器
    }

    if (!shader) {
        LOG_ERROR("Resource", "创建着色器对象失败");
        return nullptr;
    }

    RegisterResource(shader);
    return shader;
}

bool ResourceManager::CompileShader(const ShaderDesc& desc, std::string& errors) {
    std::vector<uint8_t> bytecode;
    ShaderReflection reflection;

    bool success = false;
    if (desc.language == ShaderLanguage::HLSL) {
        success = CompileHLSLShader(desc, bytecode, reflection, errors);
    } else if (desc.language == ShaderLanguage::GLSL) {
        success = CompileGLSLShader(desc, bytecode, reflection, errors);
    }

    return success;
}

std::shared_ptr<IPipelineState> ResourceManager::CreatePipelineState() {
    if (!m_device) {
        LOG_ERROR("Resource", "设备未初始化");
        return nullptr;
    }

    // 使用设备创建管线状态
    auto factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("Resource", "无法获取资源工厂");
        return nullptr;
    }

    return factory->CreatePipelineStateImpl();
}

std::shared_ptr<IPipeline> ResourceManager::LoadPipeline(const std::string& filename) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    // 检查是否已加载
    auto it = m_nameToId.find(filename);
    if (it != m_nameToId.end()) {
        auto resource = GetResource(it->second);
        if (resource) {
            return std::static_pointer_cast<IPipeline>(resource);
        }
    }

    // TODO: 实现管线加载
    LOG_INFO("Resource", "加载渲染流程: {0}", filename);
    return nullptr;
}

std::shared_ptr<ISampler> ResourceManager::CreateSampler(const SamplerDesc& desc) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    if (!m_device) {
        LOG_ERROR("Resource", "设备未初始化");
        return nullptr;
    }

    // 使用设备创建采样器
    auto factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("Resource", "无法获取资源工厂");
        return nullptr;
    }

    auto samplerUnique = factory->CreateSamplerImpl(desc);
    std::shared_ptr<ISampler> sampler;
    if (samplerUnique) {
        // 转换 unique_ptr 到 shared_ptr
        sampler = std::move(samplerUnique);
        // RegisterResource 期望 IResource，需要显式转换 shared_ptr
        RegisterResource(std::static_pointer_cast<IResource>(sampler));
    }

    return sampler;
}

std::shared_ptr<ISampler> ResourceManager::GetDefaultSampler() {
    return m_defaultSampler;
}

std::shared_ptr<IResource> ResourceManager::GetResource(ResourceId id) {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);

    auto it = m_resources.find(id);
    if (it != m_resources.end()) {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<IResource> ResourceManager::GetResource(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);

    auto it = m_nameToId.find(name);
    if (it != m_nameToId.end()) {
        return GetResource(it->second);
    }

    return nullptr;
}

void ResourceManager::ReleaseResource(ResourceId id) {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    auto it = m_resources.find(id);
    if (it != m_resources.end()) {
        // 从名称映射中移除
        if (!it->second->GetName().empty()) {
            m_nameToId.erase(it->second->GetName());
        }

        // 加入待删除队列
        m_pendingDeletion.push(id);
        m_resources.erase(it);

        m_statsDirty = true;
    }
}

void ResourceManager::GarbageCollect() {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    // 清理待删除的资源
    while (!m_pendingDeletion.empty()) {
        m_pendingDeletion.pop();
    }

    // 清理引用计数为0的资源
    for (auto it = m_resources.begin(); it != m_resources.end();) {
        if (it->second.use_count() == 1) {  // 只有 ResourceManager持有引用
            if (!it->second->GetName().empty()) {
                m_nameToId.erase(it->second->GetName());
            }
            it = m_resources.erase(it);
        } else {
            ++it;
        }
    }
}

void ResourceManager::ReleaseAllResources() {
    std::unique_lock<std::shared_mutex> lock(m_resourceMutex);

    m_resources.clear();
    m_nameToId.clear();
    m_statsDirty = true;
}

ResourceId ResourceManager::LoadTextureAsync(const std::string& filename) {
    ResourceId id = GenerateId();

    ResourceLoadTask task;
    task.type = ResourceLoadTask::LoadTexture;
    task.path = filename;
    task.name = filename;
    task.id = id;

    {
        std::lock_guard<std::mutex> lock(m_loadQueueMutex);
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
    task.name = filename;
    task.id = id;

    {
        std::lock_guard<std::mutex> lock(m_loadQueueMutex);
        m_loadQueue.push(task);
    }
    m_loadQueueCV.notify_one();

    return id;
}

bool ResourceManager::IsAsyncLoadingComplete(ResourceId id) {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);
    return m_resources.find(id) != m_resources.end();
}

ResourceManager::ResourceStats ResourceManager::GetResourceStats() const {
    if (m_statsDirty) {
        UpdateResourceStats();
    }
    return m_cachedStats;
}

void ResourceManager::EnableHotReload(bool enable) {
    m_hotReloadEnabled = enable;
    if (enable) {
        UpdateFileTimestamps();
    }
}

void ResourceManager::CheckAndReloadResources() {
    if (!m_hotReloadEnabled) {
        return;
    }

    CheckFileModifications();
}

std::shared_mutex& ResourceManager::GetResourceLock() {
    return m_resourceMutex;
}

// === 私有方法实现 ===

ResourceId ResourceManager::GenerateId() {
    return m_nextId++;
}

void ResourceManager::RegisterResource(std::shared_ptr<IResource> resource, const std::string& name) {
    if (!resource) {
        return;
    }

    ResourceId id = GenerateId();
    // TODO: resource->SetId(id); // IResource 接口需要添加 SetId 方法

    if (!name.empty()) {
        resource->SetName(name);
        m_nameToId[name] = id;
    }

    m_resources[id] = resource;
    m_statsDirty = true;
}

void ResourceManager::UnregisterResource(ResourceId id) {
    auto it = m_resources.find(id);
    if (it != m_resources.end()) {
        if (!it->second->GetName().empty()) {
            m_nameToId.erase(it->second->GetName());
        }
        m_resources.erase(it);
        m_statsDirty = true;
    }
}

void ResourceManager::LoadingThreadFunction() {
    while (!m_shouldStopLoading) {
        std::unique_lock<std::mutex> lock(m_loadQueueMutex);
        m_loadQueueCV.wait(lock, [this] { return !m_loadQueue.empty() || m_shouldStopLoading; });

        while (!m_loadQueue.empty()) {
            ResourceLoadTask task = m_loadQueue.front();
            m_loadQueue.pop();
            lock.unlock();

            ProcessLoadTask(task);

            lock.lock();
        }
    }
}

void ResourceManager::ProcessLoadTask(const ResourceLoadTask& task) {
    std::shared_ptr<IResource> resource = nullptr;

    switch (task.type) {
        case ResourceLoadTask::LoadTexture:
            resource = LoadTextureSync(task.path, true);
            break;
        case ResourceLoadTask::LoadShader:
            // TODO: 实现着色器异步加载
            break;
        default:
            break;
    }

    if (resource) {
        RegisterResource(resource, task.name);
        if (task.callback) {
            task.callback(task.id, resource);
        }
    }
}

std::shared_ptr<ITexture> ResourceManager::LoadTextureSync(const std::string& filename, bool generateMips) {
    std::vector<uint8_t> imageData;
    TextureDesc desc;
    desc.name = filename;

    // 检查缓存
    if (LoadFromCache(filename, imageData)) {
        LOG_INFO("Resource", "从缓存加载纹理: {0}", filename);
        // TODO: 从缓存数据创建纹理
    } else {
        // 从文件加载
        if (!LoadImageFromFile(filename, imageData, desc)) {
            LOG_ERROR("Resource", "加载纹理失败: {0}", filename);
            return nullptr;
        }

        // 保存到缓存
        SaveToCache(filename, imageData.data(), imageData.size());
    }

    // 创建纹理对象
    auto texture = CreateTexture(desc);
    if (texture && !imageData.empty()) {
        // TODO: 上传纹理数据
    }

    return texture;
}

bool ResourceManager::LoadImageFromFile(const std::string& filename,
                                     std::vector<uint8_t>& imageData,
                                     TextureDesc& desc) {
    // 尝试加载DDS文件
    if (filename.ends_with(".dds")) {
        return LoadDDSFromFile(filename, imageData, desc);
    }

    // 使用stb_image加载其他格式
    return LoadSTBIFromFile(filename, imageData, desc);
}

bool ResourceManager::LoadDDSFromFile(const std::string& filename,
                                     std::vector<uint8_t>& imageData,
                                     TextureDesc& desc) {
    // TODO: 实现DDS文件加载
    return false;
}

bool ResourceManager::LoadSTBIFromFile(const std::string& filename,
                                      std::vector<uint8_t>& imageData,
                                      TextureDesc& desc) {
    // TODO: Implement image loading without stb_image
    LOG_ERROR("Resource", "Image loading from file not implemented yet: {0}", filename);
    return false;
}

bool ResourceManager::CompileHLSLShader(const ShaderDesc& desc,
                                        std::vector<uint8_t>& bytecode,
                                        ShaderReflection& reflection,
                                        std::string& errors) {
#if defined(_WIN32)
    // 设置编译选项
    D3D_SHADER_MACRO macros[16] = {};
    for (size_t i = 0; i < desc.defines.size() && i < 15; ++i) {
        size_t equalPos = desc.defines[i].find('=');
        if (equalPos != std::string::npos) {
            macros[i].Name = desc.defines[i].substr(0, equalPos).c_str();
            macros[i].Definition = desc.defines[i].substr(equalPos + 1).c_str();
        } else {
            macros[i].Name = desc.defines[i].c_str();
            macros[i].Definition = "1";
        }
    }

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    // TODO: ShaderDesc 需要添加 compileOptions 成员
    bool debug = false;
    bool skipValidation = false;
    if (debug) {
        compileFlags |= D3DCOMPILE_DEBUG;
    }
    if (skipValidation) {
        compileFlags |= D3DCOMPILE_SKIP_VALIDATION;
    }

    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    // 编译着色器
    std::wstring filenameW(desc.filename.begin(), desc.filename.end());
    std::string entryPointA(desc.entryPoint.begin(), desc.entryPoint.end());
    std::string targetA;

    // 转换 target 从 wstring 到 string
    switch (desc.type) {
        case ShaderType::Vertex: targetA = "vs_5_0"; break;
        case ShaderType::Pixel: targetA = "ps_5_0"; break;
        case ShaderType::Geometry: targetA = "gs_5_0"; break;
        case ShaderType::Hull: targetA = "hs_5_0"; break;
        case ShaderType::Domain: targetA = "ds_5_0"; break;
        case ShaderType::Compute: targetA = "cs_5_0"; break;
        default: targetA = "vs_5_0"; break;
    }

    HRESULT hr = D3DCompileFromFile(
        filenameW.c_str(),
        macros,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPointA.c_str(),
        targetA.c_str(),
        compileFlags,
        0,
        shaderBlob.GetAddressOf(),
        errorBlob.GetAddressOf()
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            errors = std::string(
                static_cast<char*>(errorBlob->GetBufferPointer()),
                errorBlob->GetBufferSize()
            );
        }
        return false;
    }

    // 复制字节码
    bytecode.resize(shaderBlob->GetBufferSize());
    memcpy(bytecode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

    // TODO: 提取着色器反射信息

    return true;
#else
    errors = "HLSL编译仅在Windows上支持";
    return false;
#endif
}

bool ResourceManager::CompileGLSLShader(const ShaderDesc& desc,
                                        std::vector<uint8_t>& bytecode,
                                        ShaderReflection& reflection,
                                        std::string& errors) {
    // TODO: 实现GLSL编译
    errors = "GLSL编译尚未实现";
    return false;
}

uint64_t ResourceManager::CalculateFileHash(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return 0;
    }

    // 简单的哈希计算
    uint64_t hash = 0;
    char buffer[1024];
    while (file.read(buffer, sizeof(buffer))) {
        for (size_t i = 0; i < file.gcount(); ++i) {
            hash = hash * 31 + static_cast<uint64_t>(buffer[i]);
        }
    }

    return hash;
}

std::string ResourceManager::GetCachePath(const std::string& filename) {
    uint64_t hash = CalculateFileHash(filename);
    std::string cacheDir = m_cacheDirectory;
    if (!cacheDir.empty() && cacheDir.back() != '/' && cacheDir.back() != '\\') {
        cacheDir += '/';
    }
    return cacheDir + std::to_string(hash) + ".cache";
}

bool ResourceManager::LoadFromCache(const std::string& filename, std::vector<uint8_t>& data) {
    std::string cachePath = GetCachePath(filename);
    std::ifstream file(cachePath, std::ios::binary);

    if (!file) {
        return false;
    }

    // 读取文件大小
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 读取数据
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    return true;
}

void ResourceManager::SaveToCache(const std::string& filename, const void* data, size_t size) {
    std::string cachePath = GetCachePath(filename);
    std::ofstream file(cachePath, std::ios::binary);

    if (file) {
        file.write(static_cast<const char*>(data), size);
    }
}

void ResourceManager::UpdateResourceStats() const {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);

    m_cachedStats.totalResources = static_cast<uint32_t>(m_resources.size());
    m_cachedStats.loadedResources = m_cachedStats.totalResources;
    m_cachedStats.loadingResources = 0;  // 暂时计算
    m_cachedStats.totalMemoryUsage = 0;  // 需要计算所有资源大小
    m_cachedStats.textureMemoryUsage = 0;
    m_cachedStats.bufferMemoryUsage = 0;

    // 计算内存使用情况
    for (const auto& [id, resource] : m_resources) {
        uint64_t size = resource->GetSize();
        m_cachedStats.totalMemoryUsage += size;

        switch (resource->GetType()) {
            case ResourceType::Texture:
                m_cachedStats.textureMemoryUsage += size;
                break;
            case ResourceType::Buffer:
                m_cachedStats.bufferMemoryUsage += size;
                break;
            default:
                break;
        }
    }

    m_statsDirty = false;
}

void ResourceManager::UpdateFileTimestamps() {
    m_fileTimestamps.clear();

    for (const auto& [name, id] : m_nameToId) {
        auto resource = GetResource(id);
        if (resource && resource->GetType() == ResourceType::Shader) {
            std::error_code ec;
            auto ftime = fs::last_write_time(name, ec);
            if (!ec) {
                m_fileTimestamps[name] = ftime;
            }
        }
    }
}

void ResourceManager::CheckFileModifications() {
    bool needsUpdate = false;

    for (auto& [name, timestamp] : m_fileTimestamps) {
        std::error_code ec;
        auto currentFtime = fs::last_write_time(name, ec);
        if (!ec && currentFtime != timestamp) {
            timestamp = currentFtime;
            needsUpdate = true;

            // TODO: 重新加载着色器
            LOG_INFO("Resource", "检测到文件修改: {0}", name);
        }
    }

    if (needsUpdate) {
        UpdateFileTimestamps();
    }

    m_statsDirty = false;
}

} // namespace PrismaEngine::Graphic