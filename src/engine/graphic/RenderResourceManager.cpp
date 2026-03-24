#include "RenderResourceManager.h"
#include "Logger.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/pipelines/forward/ForwardPipeline.h"
#include <chrono>
#include <fstream>
#include <algorithm>
#include <stb_image.h>

namespace Prisma::Graphic {

std::shared_ptr<IRenderResourceManager> RenderResourceManager::Get() {
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
    return 0;
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
    if (!m_device || !m_device->GetResourceFactory()) {
        return nullptr;
    }

    ShaderDesc resolvedDesc = desc;
    resolvedDesc.source = source;
    resolvedDesc.compileTimestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    if (!resolvedDesc.filename.empty()) {
        auto shader = LoadShaderSync(
            resolvedDesc.filename,
            resolvedDesc.entryPoint,
            resolvedDesc.target,
            resolvedDesc.defines
        );
        if (shader) {
            RegisterResource(shader, resolvedDesc.filename);
        }
        return shader;
    }

    if (resolvedDesc.language != ShaderLanguage::SPIRV || resolvedDesc.source.empty()) {
        LOG_ERROR("RenderResourceManager", "Only precompiled SPIR-V shader creation is supported without an external compiler");
        return nullptr;
    }

    if (resolvedDesc.source.size() % sizeof(uint32_t) != 0) {
        LOG_ERROR("RenderResourceManager", "SPIR-V shader source size must be 4-byte aligned");
        return nullptr;
    }

    std::vector<uint8_t> bytecode(resolvedDesc.source.begin(), resolvedDesc.source.end());
    auto shader = m_device->GetResourceFactory()->CreateShaderImpl(resolvedDesc, bytecode, ShaderReflection{});
    if (!shader) {
        return nullptr;
    }

    std::shared_ptr<IShader> sharedShader = std::move(shader);
    RegisterResource(sharedShader, resolvedDesc.filename);
    return sharedShader;
}

bool RenderResourceManager::CompileShader(const ShaderDesc& desc, std::string* errors) {
    if (desc.entryPoint.empty()) {
        if (errors) {
            *errors = "Shader entry point is required";
        }
        return false;
    }

    if (!desc.filename.empty()) {
        if (!std::filesystem::exists(desc.filename)) {
            if (errors) {
                *errors = "Shader file not found: " + desc.filename;
            }
            return false;
        }

        if (std::filesystem::path(desc.filename).extension() == ".spv") {
            if (errors) {
                errors->clear();
            }
            return true;
        }
    }

    if (errors) {
        *errors = "Runtime shader compilation is not available; provide precompiled SPIR-V (.spv)";
    }
    return false;
}

std::shared_ptr<IPipeline> RenderResourceManager::CreatePipeline(const PipelineDesc& desc) {
    if (!desc.name.empty()) {
        LOG_INFO("RenderResourceManager", "Creating pipeline: {0}", desc.name);
    }

    if (desc.computeShader && !desc.vertexShader && !desc.pixelShader) {
        LOG_WARNING("RenderResourceManager", "Compute-only pipeline requests are not mapped to a high-level IPipeline yet");
        return nullptr;
    }

    auto pipeline = std::make_shared<ForwardPipeline>();
    if (pipeline->Initialize(m_device) != 0) {
        return nullptr;
    }
    return pipeline;
}

std::shared_ptr<IPipelineState> RenderResourceManager::CreatePipelineState(const PipelineStateDesc& desc) {
    if (!m_device || !m_device->GetResourceFactory()) {
        return nullptr;
    }

    auto pipelineState = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pipelineState) {
        return nullptr;
    }

    pipelineState->SetPrimitiveTopology(desc.primitiveTopology);
    pipelineState->SetShader(ShaderType::Vertex, desc.vertexShader);
    pipelineState->SetShader(ShaderType::Pixel, desc.pixelShader);
    pipelineState->SetShader(ShaderType::Geometry, desc.geometryShader);
    pipelineState->SetShader(ShaderType::Hull, desc.hullShader);
    pipelineState->SetShader(ShaderType::Domain, desc.domainShader);
    pipelineState->SetShader(ShaderType::Compute, desc.computeShader);

    BlendState blendState;
    blendState.blendEnable = desc.blendState.blendEnable;
    blendState.logicOpEnable = desc.blendState.logicOpEnable;
    blendState.writeMask = desc.blendState.writeMask;
    blendState.blendOp = desc.blendState.blendOp;
    blendState.srcBlend = desc.blendState.srcBlend;
    blendState.destBlend = desc.blendState.destBlend;
    blendState.blendOpAlpha = desc.blendState.blendOpAlpha;
    blendState.srcBlendAlpha = desc.blendState.srcBlendAlpha;
    blendState.destBlendAlpha = desc.blendState.destBlendAlpha;
    pipelineState->SetBlendState(blendState);

    RasterizerState rasterizerState;
    rasterizerState.cullEnable = desc.rasterizerState.cullEnable;
    rasterizerState.frontCounterClockwise = desc.rasterizerState.frontCounterClockwise;
    rasterizerState.depthClipEnable = desc.rasterizerState.depthClipEnable;
    rasterizerState.fillMode = desc.rasterizerState.fillMode;
    rasterizerState.cullMode = desc.rasterizerState.cullMode;
    rasterizerState.depthBias = static_cast<int>(desc.rasterizerState.depthBias);
    rasterizerState.depthBiasClamp = desc.rasterizerState.depthBiasClamp;
    rasterizerState.slopeScaledDepthBias = desc.rasterizerState.slopeScaledDepthBias;
    pipelineState->SetRasterizerState(rasterizerState);

    DepthStencilState depthStencilState;
    depthStencilState.depthEnable = desc.depthStencilState.depthEnable;
    depthStencilState.depthWriteEnable = desc.depthStencilState.depthWriteEnable;
    depthStencilState.stencilEnable = desc.depthStencilState.stencilEnable;
    depthStencilState.depthFunc = desc.depthStencilState.depthFunc;
    depthStencilState.stencilReadMask = desc.depthStencilState.stencilReadMask;
    depthStencilState.stencilWriteMask = desc.depthStencilState.stencilWriteMask;
    depthStencilState.frontFace.failOp = desc.depthStencilState.frontFaceFail;
    depthStencilState.frontFace.depthFailOp = desc.depthStencilState.frontFaceDepthFail;
    depthStencilState.frontFace.passOp = desc.depthStencilState.frontFacePass;
    depthStencilState.frontFace.func = desc.depthStencilState.frontFaceFunc;
    depthStencilState.backFace.failOp = desc.depthStencilState.backFaceFail;
    depthStencilState.backFace.depthFailOp = desc.depthStencilState.backFaceDepthFail;
    depthStencilState.backFace.passOp = desc.depthStencilState.backFacePass;
    depthStencilState.backFace.func = desc.depthStencilState.backFaceFunc;
    pipelineState->SetDepthStencilState(depthStencilState);

    std::vector<VertexInputAttribute> inputLayout;
    inputLayout.reserve(desc.inputLayout.size());
    for (const auto& attribute : desc.inputLayout) {
        VertexInputAttribute converted;
        converted.semanticName = attribute.semanticName;
        converted.semanticIndex = attribute.semanticIndex;
        converted.format = attribute.format;
        converted.inputSlot = attribute.inputSlot;
        converted.alignedByteOffset = attribute.alignedByteOffset;
        converted.inputSlotClass = attribute.isPerInstance ? 1u : 0u;
        converted.instanceDataStepRate = attribute.instanceDataStepRate;
        inputLayout.push_back(converted);
    }
    pipelineState->SetInputLayout(inputLayout);

    std::vector<TextureFormat> renderTargetFormats;
    for (uint32_t i = 0; i < desc.numRenderTargets; ++i) {
        renderTargetFormats.push_back(desc.renderTargetFormats[i]);
    }
    pipelineState->SetRenderTargetFormats(renderTargetFormats);
    pipelineState->SetDepthStencilFormat(desc.depthStencilFormat);
    pipelineState->SetSampleCount(desc.sampleCount, desc.sampleQuality);
    pipelineState->SetDebugName(desc.name);

    if (!pipelineState->Create(m_device)) {
        LOG_ERROR("RenderResourceManager", "Failed to create pipeline state: {0}", pipelineState->GetErrors());
        return nullptr;
    }

    std::shared_ptr<IPipelineState> sharedPipelineState = std::move(pipelineState);
    return sharedPipelineState;
}

std::shared_ptr<IPipeline> RenderResourceManager::LoadPipeline(const std::string& filename) {
    PipelineDesc desc;
    desc.name = filename;
    return CreatePipeline(desc);
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
    desc.filename = filename;
    desc.source = source;
    desc.entryPoint = entryPoint;
    desc.target = target;
    desc.defines = defines;
    desc.language = ShaderLanguage::SPIRV;

    if (std::filesystem::path(filename).extension() != ".spv") {
        LOG_WARNING("RenderResourceManager", "Only precompiled SPIR-V shader loading is supported: {0}", filename);
        return nullptr;
    }

    std::vector<uint8_t> bytecode(source.begin(), source.end());
    auto shader = m_device->GetResourceFactory()->CreateShaderImpl(desc, bytecode, ShaderReflection{});
    return shader ? std::shared_ptr<IShader>(std::move(shader)) : nullptr;
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
    const std::filesystem::path cachePath = std::filesystem::path(m_cacheDirectory) / (std::filesystem::path(filename).filename().string() + ".cache");
    if (!std::filesystem::exists(cachePath)) {
        return false;
    }

    std::ifstream file(cachePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !data.empty();
}

void RenderResourceManager::SaveToCache(const std::string& filename, const void* data, size_t size) {
    if (!data || size == 0) {
        return;
    }

    std::filesystem::create_directories(m_cacheDirectory);
    const std::filesystem::path cachePath = std::filesystem::path(m_cacheDirectory) / (std::filesystem::path(filename).filename().string() + ".cache");
    std::ofstream file(cachePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return;
    }

    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
}

bool RenderResourceManager::LoadImageFromFile(const std::string& filename, std::vector<uint8_t>& data, TextureDesc& desc) {
    std::vector<uint8_t> cachedData;
    if (LoadFromCache(filename, cachedData)) {
        data = std::move(cachedData);
    } else {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* imageData = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!imageData) {
            return false;
        }

        data.assign(imageData, imageData + static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
        stbi_image_free(imageData);
        SaveToCache(filename, data.data(), data.size());
        desc.width = static_cast<uint64_t>(width);
        desc.height = static_cast<uint64_t>(height);
    }

    if (desc.width == 0 || desc.height == 0) {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* imageData = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!imageData) {
            return false;
        }
        desc.width = static_cast<uint64_t>(width);
        desc.height = static_cast<uint64_t>(height);
        data.assign(imageData, imageData + static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
        stbi_image_free(imageData);
    }

    desc.depth = 1;
    desc.arraySize = 1;
    desc.format = TextureFormat::RGBA8_UNorm;
    desc.allowShaderResource = true;
    desc.filename = filename;
    return !data.empty();
}

} // namespace Prisma::Graphic
