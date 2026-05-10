#include "RenderResourceManager.h"
#include "logger/Logger.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/IPipeline.h"
#include "graphic/interfaces/ISampler.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/pipelines/forward/ForwardPipeline.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <stb_image.h>

#include "adapters/vulkan/VulkanEmbeddedShaders.h"

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
    LOG_INFO("RenderResourceManager", "资源管理器已初始化。");
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
    LOG_INFO("RenderResourceManager", "正在关闭渲染资源管理器...");
    m_shouldStopLoading = true;
    m_loadQueueCV.notify_all();
    if (m_loadingThread.joinable()) {
        LOG_INFO("RenderResourceManager", "等待加载线程结束...");
        m_loadingThread.join();
        LOG_INFO("RenderResourceManager", "加载线程已结束");
    }

    LOG_INFO("RenderResourceManager", "释放所有 GPU 资源...");
    ReleaseAllResources();
    m_defaultSampler.reset();  // 在设备销毁前释放默认采样器
    m_initialized = false;
    LOG_INFO("RenderResourceManager", "渲染资源管理器已关闭");
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
        LOG_ERROR("RenderResourceManager", "在没有外部编译器的情况下，仅支持创建预编译的 SPIR-V 着色器");
        return nullptr;
    }

    if (resolvedDesc.source.size() % sizeof(uint32_t) != 0) {
        LOG_ERROR("RenderResourceManager", "SPIR-V 着色器源文件大小必须 4 字节对齐");
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
            *errors = "着色器入口点是必需的";
        }
        return false;
    }

    if (!desc.filename.empty()) {
        if (!std::filesystem::exists(desc.filename)) {
            if (errors) {
                *errors = "未找到着色器文件: " + desc.filename;
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
        *errors = "运行时着色器编译不可用；请提供预编译的 SPIR-V (.spv)";
    }
    return false;
}

std::shared_ptr<IPipeline> RenderResourceManager::CreatePipeline(const PipelineDesc& desc) {
    if (!desc.name.empty()) {
        LOG_INFO("RenderResourceManager", "正在创建管线: {0}", desc.name);
    }

    if (desc.computeShader && !desc.vertexShader && !desc.pixelShader) {
        LOG_WARNING("RenderResourceManager", "仅计算管线的请求尚未映射到高级 IPipeline");
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
        LOG_ERROR("RenderResourceManager", "创建管线状态失败: {0}", pipelineState->GetErrors());
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
        LOG_ERROR("RenderResourceManager", "异步加载资源失败: {0}", task.path);
    }
}

std::shared_ptr<ITexture> RenderResourceManager::LoadTextureSync(const std::string& filename, bool generateMips) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;
    TextureDesc desc;
    std::vector<uint8_t> data;
    if (LoadImageFromFile(filename, data, desc)) {
        // ValidateTextureDesc 要求 mipLevels > 0。
        // 当 generateMips=true 时暂使用 1，待 mip 生成管线实现后再改为完整层数计算。
        desc.mipLevels = 1;
        auto texture = m_device->GetResourceFactory()->CreateTextureFromMemory(data.data(), data.size(), desc);
        return std::shared_ptr<ITexture>(std::move(texture));
    }
    return nullptr;
}

std::shared_ptr<IShader> RenderResourceManager::GetShader(const std::string& name) {
    std::shared_lock<std::shared_mutex> lock(m_resourceMutex);
    auto it = m_nameToId.find(name);
    if (it == m_nameToId.end()) return nullptr;

    auto resIt = m_resources.find(it->second);
    if (resIt == m_resources.end()) return nullptr;

    return std::dynamic_pointer_cast<IShader>(resIt->second);
}

std::shared_ptr<IShader> RenderResourceManager::LoadShaderSync(const std::string& filename, const std::string& entryPoint, const std::string& target, const std::vector<std::string>& defines) {
    if (!m_device || !m_device->GetResourceFactory()) return nullptr;

    // [修复] 优先从缓存获取
    auto cached = GetShader(filename);
    if (cached) return cached;

    // [修复] 处理内置默认 Shader
    if (filename == "Default" || filename == "DefaultPixel") {
        ShaderDesc desc;
        desc.name = filename;
        desc.entryPoint = "main";
        desc.language = ShaderLanguage::SPIRV;
        
        // 创建默认反射
        ShaderReflection reflection;
        
        const uint32_t* spirv_ptr = nullptr;
        size_t spirv_size = 0;

        if (filename == "Default") {
            desc.type = ShaderType::Vertex;
            spirv_ptr = Vulkan::VULKAN_DEFAULT_VERT_SPV;
            spirv_size = sizeof(Vulkan::VULKAN_DEFAULT_VERT_SPV);
            
            // 保持 Transform 作为备选，或者由 Push Constants 处理
            ShaderResource transformRes;
            transformRes.Name = "Transform";
            transformRes.ResourceType = ShaderResource::Type::UniformBuffer;
            transformRes.Set = 0;
            transformRes.Binding = 1; 
            reflection.Resources.push_back(transformRes);
        } else {
            desc.type = ShaderType::Pixel;
            spirv_ptr = Vulkan::VULKAN_DEFAULT_FRAG_SPV;
            spirv_size = sizeof(Vulkan::VULKAN_DEFAULT_FRAG_SPV);

            // 2D 渲染主要使用纹理，将其放在 Set 0, Binding 0
            ShaderResource albedoRes;
            albedoRes.Name = "AlbedoMap";
            albedoRes.ResourceType = ShaderResource::Type::Sampler2D;
            albedoRes.Set = 0;
            albedoRes.Binding = 0;
            reflection.Resources.push_back(albedoRes);
        }

        // === 日志：内嵌 SPIR-V 着色器信息 ===
        {
            auto shaderTypeName = (desc.type == ShaderType::Vertex) ? "Vertex" : "Pixel";
            auto magicHex = static_cast<unsigned long>(spirv_ptr[0]);
            LOG_INFO("RenderResourceManager", "加载内嵌 SPIR-V: name={0} type={1} | size={2} bytes ({3} words) | magic=0x{4:08X}",
                filename, shaderTypeName, spirv_size, spirv_size / sizeof(uint32_t), magicHex);
            if (spirv_ptr[0] != 0x07230203) {
                LOG_WARNING("RenderResourceManager", "内嵌 SPIR-V magic number 异常: 0x{0:08X} (预期 0x07230203)", magicHex);
            }
        }

        // 使用硬编码的字节码
        std::vector<uint8_t> bytecode(
            reinterpret_cast<const uint8_t*>(spirv_ptr),
            reinterpret_cast<const uint8_t*>(spirv_ptr) + spirv_size
        );

        auto shader = m_device->GetResourceFactory()->CreateShaderImpl(desc, bytecode, reflection);
        if (!shader) return nullptr;
        
        auto sharedShader = std::shared_ptr<IShader>(std::move(shader));
        RegisterResource(sharedShader, filename);
        return sharedShader;
    }

    // === 详细日志：记录 SPIR-V 文件的绝对路径、大小和 magic number ===
    std::error_code ec;
    auto absPath = std::filesystem::absolute(std::filesystem::path(filename), ec);
    if (ec) {
        LOG_WARNING("RenderResourceManager", "无法解析 SPIR-V 文件绝对路径: {0} (err={1})", filename, ec.message());
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        if (!absPath.empty()) {
            LOG_ERROR("RenderResourceManager", "无法打开 SPIR-V 文件: {0}", absPath.string());
        } else {
            LOG_ERROR("RenderResourceManager", "无法打开 SPIR-V 文件: {0}", filename);
        }
        return nullptr;
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streamoff rawSize = file.tellg();
    size_t size = static_cast<size_t>(rawSize);
    file.seekg(0, std::ios::beg);

    // 读取前 4 字节验证 SPIR-V magic number
    uint32_t magic = 0;
    if (size >= 4) {
        file.read(reinterpret_cast<char*>(&magic), 4);
        file.seekg(0, std::ios::beg); // 重新定位到文件头
    }

    {
        auto magicHex = static_cast<unsigned long>(magic);
        if (!absPath.empty()) {
            LOG_INFO("RenderResourceManager", "加载 SPIR-V: {0} | size={1} bytes | magic=0x{2:08X}",
                absPath.string(), size, magicHex);
        } else {
            LOG_INFO("RenderResourceManager", "加载 SPIR-V: {0} | magic=0x{1:08X}", filename, magicHex);
        }

        if (magic != 0x07230203) {
            LOG_WARNING("RenderResourceManager", "SPIR-V magic number 异常: 0x{0:08X} (预期 0x07230203)", magicHex);
        }
    }

    if (size == 0 || size % 4 != 0) {
        LOG_ERROR("RenderResourceManager", "SPIR-V 文件大小无效或未 4 字节对齐: {0} ({1} bytes, {2}%4={3})",
            filename, size, size, size % 4);
        return nullptr;
    }

    std::vector<uint8_t> bytecode(size);
    file.read(reinterpret_cast<char*>(bytecode.data()), size);
    size_t bytesRead = static_cast<size_t>(file.gcount());
    if (bytesRead != size) {
        LOG_ERROR("RenderResourceManager", "SPIR-V 文件读取不完整: 期望 {0} bytes, 实际读取 {1} bytes", size, bytesRead);
        return nullptr;
    }
    
    ShaderDesc desc;
    desc.filename = filename;
    desc.entryPoint = entryPoint;
    desc.target = target;
    desc.defines = defines;
    desc.language = ShaderLanguage::SPIRV;

    // [修复] 为内置的 Renderer2D 着色器提供手动反射信息，
    // 因为目前引擎还没有自动 SPIR-V 反射解析器。
    ShaderReflection reflection;
    if (filename.find("Renderer2D.frag.spv") != std::string::npos) {
        ShaderResource albedoRes;
        albedoRes.Name = "AlbedoMap";
        albedoRes.ResourceType = ShaderResource::Type::Sampler2D;
        albedoRes.Set = 0;
        albedoRes.Binding = 0;
        reflection.Resources.push_back(albedoRes);
        desc.type = ShaderType::Pixel;
    } else if (filename.find("Renderer2D.vert.spv") != std::string::npos) {
        desc.type = ShaderType::Vertex;
    }

    auto shader = m_device->GetResourceFactory()->CreateShaderImpl(desc, bytecode, reflection);
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
        // 我们还需要获取宽度和高度，缓存中应该保存这些信息，但目前简化处理
        // 如果缓存命中，暂时通过 stbi_info 获取基本信息而不完全解码
        int width = 0, height = 0, channels = 0;
        if (stbi_info(filename.c_str(), &width, &height, &channels)) {
            desc.width = static_cast<uint64_t>(width);
            desc.height = static_cast<uint64_t>(height);
        } else {
            LOG_ERROR("RenderResourceManager", "无法获取缓存文件的元数据: {0}", filename);
            return false;
        }
    } else {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* imageData = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!imageData) {
            LOG_ERROR("RenderResourceManager", "stbi_load 失败: {0}, 原因: {1}", filename, stbi_failure_reason());
            return false;
        }

        data.assign(imageData, imageData + static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
        stbi_image_free(imageData);
        SaveToCache(filename, data.data(), data.size());
        desc.width = static_cast<uint64_t>(width);
        desc.height = static_cast<uint64_t>(height);
    }

    if (desc.width == 0 || desc.height == 0) {
        LOG_ERROR("RenderResourceManager", "图像尺寸无效 (0): {0}", filename);
        return false;
    }

    desc.depth = 1;
    desc.arraySize = 1;
    desc.format = TextureFormat::RGBA8_UNorm;
    desc.allowShaderResource = true;
    desc.filename = filename;
    return !data.empty();
}

} // namespace Prisma::Graphic
