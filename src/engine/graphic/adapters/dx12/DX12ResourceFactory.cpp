#include "DX12ResourceFactory.h"
#include "DX12RenderDevice.h"
#include "DX12Texture.h"
#include "DX12Buffer.h"
#include "DX12Shader.h"
#include "DX12PipelineState.h"
#include "DX12Sampler.h"
#include "DX12SwapChain.h"
#include "DX12Fence.h"
#include "../Logger.h"
#include <directx/d3dx12.h>
#include <directx/d3d12shader.h>
// TODO: Use DXC for shader compilation when available
#include <wrl/client.h>
// TODO: Implement image loading without stb_image
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace PrismaEngine::Graphic {
    // 获取每个像素的字节数
    uint32_t GetBytesPerPixel(TextureFormat format) {
        switch (format) {
            case TextureFormat::R8_UNorm:
                return 1;
            case TextureFormat::R16_Float:
            case TextureFormat::R16_UNorm:
            case TextureFormat::RG16_UNorm:
            case TextureFormat::RGB8_UNorm:
                return 2;
            case TextureFormat::R32_Float:
            case TextureFormat::RG16_Float:
            case TextureFormat::D24_UNorm_S8_UInt:
                return 3;
            case TextureFormat::RGBA8_UNorm:
            case TextureFormat::RGBA8_SNorm:
            case TextureFormat::RGBA8_UInt:
            case TextureFormat::RGBA8_SInt:
            case TextureFormat::RGBA32_Float:
            case TextureFormat::D32_Float:
                return 4;
            case TextureFormat::RGB32_Float:
                return 12; // RGB32 = 3 * 4 bytes
            default:
                return 4; // 默认4字节
        }
    }
}

using namespace PrismaEngine::Graphic;

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

DX12ResourceFactory::DX12ResourceFactory(DX12RenderDevice* device)
    : m_device(device) {
}

DX12ResourceFactory::~DX12ResourceFactory() {
    Shutdown();
}

// IResourceFactory接口实现
bool DX12ResourceFactory::Initialize(IRenderDevice* device) {
    if (m_initialized) {
        return true;
    }

    if (!m_device) {
        LOG_ERROR("DX12ResourceFactory", "Device not set");
        return false;
    }

    // 创建默认描述符堆
    CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 512, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    m_initialized = true;
    LOG_INFO("DX12ResourceFactory", "Resource factory initialized successfully");
    return true;
}

void DX12ResourceFactory::Shutdown() {
    if (!m_initialized) {
        return;
    }

    // 清理延迟销毁资源
    ProcessDeferredDestructions();

    // 清理资源池
    m_texturePools.clear();

    // 清理描述符堆
    m_descriptorHeaps.clear();

    // 重置统计信息
    ResetStats();

    m_initialized = false;
    LOG_INFO("DX12ResourceFactory", "Resource factory shutdown");
}

std::unique_ptr<ITexture> DX12ResourceFactory::CreateTextureImpl(const TextureDesc& desc) {
    if (!m_initialized || !m_device) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    // 验证描述
    std::string errorMsg;
    if (!ValidateTextureDesc(desc, errorMsg)) {
        LOG_ERROR("DX12ResourceFactory", "Invalid texture description: {0}", errorMsg);
        return nullptr;
    }

    // 检查资源池
    if (m_resourcePoolingEnabled && desc.width * desc.height * GetBytesPerPixel(desc.format) >= m_poolingThreshold) {
        uint64_t poolKey = CalculateTexturePoolKey(desc);
        auto poolIt = m_texturePools.find(poolKey);
        if (poolIt != m_texturePools.end() && !poolIt->second->freeTextures.empty()) {
            auto texture = std::move(poolIt->second->freeTextures.back());
            poolIt->second->freeTextures.pop_back();
            m_stats.texturesPooled++;  // 修改为有效的成员变量
            return std::move(texture);
        }
    }

    // 创建D3D12资源描述
    auto d3d12Desc = GetD3D12TextureDesc(desc);

    // 确定堆类型和初始状态
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST;

    if (desc.allowRenderTarget) {
        initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    } else if (desc.allowDepthStencil) {
        initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    } else if (desc.allowShaderResource) {
        initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // 创建资源
    ComPtr<ID3D12Resource> resource = CreateCommittedResource(
        d3d12Desc, heapType, D3D12_HEAP_FLAG_NONE, initialState);

    if (!resource) {
        LOG_ERROR("DX12ResourceFactory", "Failed to create D3D12 texture resource");
        return nullptr;
    }

    // 创建纹理适配器
    auto texture = std::make_unique<DX12Texture>(m_device, resource, desc);

    // 创建必要的视图
    if (desc.allowRenderTarget) {
        auto rtvHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].get();
        if (rtvHeap) {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
            uint32_t index = AllocateDescriptor(rtvHeap->heap.Get(), cpuHandle, gpuHandle);
            texture->CreateRTV(cpuHandle);
        }
    }

    if (desc.allowDepthStencil) {
        auto dsvHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].get();
        if (dsvHeap) {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
            uint32_t index = AllocateDescriptor(dsvHeap->heap.Get(), cpuHandle, gpuHandle);
            texture->CreateDSV(cpuHandle);
        }
    }

    if (desc.allowShaderResource) {
        auto srvHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].get();
        if (srvHeap) {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
            uint32_t index = AllocateDescriptor(srvHeap->heap.Get(), cpuHandle, gpuHandle);
            texture->CreateSRV(cpuHandle);
        }
    }

    if (desc.allowUnorderedAccess) {
        auto uavHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].get();
        if (uavHeap) {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
            uint32_t index = AllocateDescriptor(uavHeap->heap.Get(), cpuHandle, gpuHandle);
            texture->CreateUAV(cpuHandle);
        }
    }

    // 更新统计信息
    m_stats.texturesCreated++;
    m_stats.totalMemoryAllocated += texture->GetSize();

    LOG_INFO("DX12ResourceFactory", "Created texture: {0}x{1}, format: {2}",
             desc.width, desc.height, static_cast<uint32_t>(desc.format));

    return std::move(texture);
}

std::unique_ptr<ITexture> DX12ResourceFactory::CreateTextureFromFile(const std::string& filename,
                                                                    const TextureDesc* desc) {
    if (!m_initialized) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    // 检查文件是否存在
    if (!std::filesystem::exists(filename)) {
        LOG_ERROR("DX12ResourceFactory", "Texture file not found: {0}", filename);
        return nullptr;
    }

    // 加载图像数据
    std::vector<uint8_t> imageData;
    TextureDesc loadDesc;
    if (!LoadImageFromFile(filename, imageData, loadDesc)) {
        LOG_ERROR("DX12ResourceFactory", "Failed to load image from file: {0}", filename);
        return nullptr;
    }

    // 使用提供的描述或从文件加载的描述
    TextureDesc finalDesc = desc ? *desc : loadDesc;
    finalDesc.filename = filename;

    // 创建纹理
    auto texture = CreateTextureImpl(finalDesc);
    if (!texture) {
        return nullptr;
    }

    // 上传数据
    auto dx12Texture = static_cast<DX12Texture*>(texture.get());
    dx12Texture->UpdateData(imageData.data(), imageData.size(), 0, 0, 0, 0, 0, finalDesc.width, finalDesc.height, 1);

    LOG_INFO("DX12ResourceFactory", "Loaded texture from file: {0}", filename);
    return std::move(texture);
}

std::unique_ptr<ITexture> DX12ResourceFactory::CreateTextureFromMemory(const void* data,
                                                                     uint64_t dataSize,
                                                                     const TextureDesc& desc) {
    if (!m_initialized || !data) {
        LOG_ERROR("DX12ResourceFactory", "Invalid parameters for texture creation from memory");
        return nullptr;
    }

    // 创建纹理
    auto texture = CreateTextureImpl(desc);
    if (!texture) {
        return nullptr;
    }

    // 上传数据
    auto dx12Texture = static_cast<DX12Texture*>(texture.get());
    dx12Texture->UpdateData(data, dataSize, 0, 0, 0, 0, 0, desc.width, desc.height, 1);

    LOG_INFO("DX12ResourceFactory", "Created texture from memory: {0} bytes", dataSize);
    return std::move(texture);
}

std::unique_ptr<IBuffer> DX12ResourceFactory::CreateBufferImpl(const BufferDesc& desc) {
    if (!m_initialized || !m_device) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    // 验证描述
    std::string errorMsg;
    if (!ValidateBufferDesc(desc, errorMsg)) {
        LOG_ERROR("DX12ResourceFactory", "Invalid buffer description: {0}", errorMsg);
        return nullptr;
    }

    // 创建D3D12资源描述
    auto d3d12Desc = GetD3D12BufferDesc(desc);

    // 确定堆类型和状态
    D3D12_HEAP_TYPE heapType = GetHeapType(desc.usage);
    D3D12_HEAP_FLAGS heapFlags = GetHeapFlags(desc.usage);
    D3D12_RESOURCE_STATES initialState = GetInitialResourceState(desc.type, desc.usage);

    // 创建资源
    ComPtr<ID3D12Resource> resource = CreateCommittedResource(
        d3d12Desc, heapType, heapFlags, initialState);

    if (!resource) {
        LOG_ERROR("DX12ResourceFactory", "Failed to create D3D12 buffer resource");
        return nullptr;
    }

    // 创建缓冲区适配器
    auto buffer = std::make_unique<DX12Buffer>(m_device, resource, desc);

    // 创建必要的视图
    if (HasFlag(desc.usage, BufferUsage::ShaderResource)) {
        auto srvHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].get();
        if (srvHeap) {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
            uint32_t index = AllocateDescriptor(srvHeap->heap.Get(), cpuHandle, gpuHandle);
            // buffer->CreateSRV(cpuHandle);
        }
    }

    if (HasFlag(desc.usage, BufferUsage::UnorderedAccess)) {
        auto uavHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].get();
        if (uavHeap) {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
            uint32_t index = AllocateDescriptor(uavHeap->heap.Get(), cpuHandle, gpuHandle);
            // buffer->CreateUAV(cpuHandle);
        }
    }

    if (desc.type == BufferType::Constant) {
        auto cbvHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].get();
        if (cbvHeap) {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
            uint32_t index = AllocateDescriptor(cbvHeap->heap.Get(), cpuHandle, gpuHandle);
            // buffer->CreateCBV(cpuHandle);
        }
    }

    // 更新统计信息
    m_stats.buffersCreated++;
    m_stats.totalMemoryAllocated += buffer->GetSize();

    LOG_INFO("DX12ResourceFactory", "Created buffer: {0} bytes, type: {1}",
             desc.size, static_cast<uint32_t>(desc.type));

    return std::move(buffer);
}

std::unique_ptr<IBuffer> DX12ResourceFactory::CreateDynamicBuffer(uint64_t size,
                                                                 BufferType type,
                                                                 BufferUsage usage) {
    BufferDesc desc = {};
    desc.type = type;
    desc.size = size;
    desc.usage = BufferUsage(usage | BufferUsage::Dynamic);
    desc.stride = 0; // 将由构造函数自动计算

    return CreateBufferImpl(desc);
}

std::unique_ptr<IShader> DX12ResourceFactory::CreateShaderImpl(const ShaderDesc& desc,
                                                             const std::vector<uint8_t>& bytecode,
                                                             const ShaderReflection& reflection) {
    if (!m_initialized) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    // 验证描述
    std::string errorMsg;
    if (!ValidateShaderDesc(desc, errorMsg)) {
        LOG_ERROR("DX12ResourceFactory", "Invalid shader description: {0}", errorMsg);
        return nullptr;
    }

    // 创建着色器适配器
    auto shader = std::make_unique<DX12Shader>(m_device, desc, bytecode, reflection);

    // 更新统计信息
    m_stats.shadersCreated++;

    LOG_INFO("DX12ResourceFactory", "Created shader: {0}, type: {1}",
             desc.filename.empty() ? "From bytecode" : desc.filename,
             static_cast<uint32_t>(desc.type));

    return std::move(shader);
}

std::unique_ptr<IPipelineState> DX12ResourceFactory::CreatePipelineStateImpl() {
    if (!m_initialized) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    auto pipelineState = std::make_unique<DX12PipelineState>(m_device);

    // 更新统计信息

    LOG_INFO("DX12ResourceFactory", "Created pipeline state object");
    return std::move(pipelineState);
}

std::unique_ptr<ISampler> DX12ResourceFactory::CreateSamplerImpl(const SamplerDesc& desc) {
    if (!m_initialized) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    auto sampler = std::make_unique<DX12Sampler>(m_device, desc);

    // 创建采样器描述符
    auto samplerHeap = m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].get();
    if (samplerHeap) {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
        uint32_t index = AllocateDescriptor(samplerHeap->heap.Get(), cpuHandle, gpuHandle);
        if (index != 0) {
            sampler->CreateSampler(cpuHandle);
        }
    }

    // 更新统计信息
    m_stats.samplersCreated++;

    LOG_INFO("DX12ResourceFactory", "Created sampler");
    return std::move(sampler);
}

std::unique_ptr<ISwapChain> DX12ResourceFactory::CreateSwapChainImpl(void* windowHandle,
                                                                    uint32_t width,
                                                                    uint32_t height,
                                                                    TextureFormat format,
                                                                    uint32_t bufferCount,
                                                                    bool vsync) {
    if (!m_initialized || !m_device) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    // 创建交换链适配器
    auto swapChain = std::make_unique<DX12SwapChain>(m_device);

    // 设置交换链属性
    swapChain->SetMode(vsync ? SwapChainMode::VSync : SwapChainMode::Immediate);

    // 更新统计信息

    LOG_INFO("DX12ResourceFactory", "Created swap chain: {0}x{1}, buffers: {2}",
             width, height, bufferCount);

    return std::move(swapChain);
}

std::unique_ptr<IFence> DX12ResourceFactory::CreateFenceImpl() {
    if (!m_initialized || !m_device) {
        LOG_ERROR("DX12ResourceFactory", "Factory not initialized");
        return nullptr;
    }

    // 创建D3D12围栏
    ComPtr<ID3D12Fence> fence;
    auto d3d12Device = m_device->GetD3D12Device();
    if (!d3d12Device) {
        LOG_ERROR("DX12ResourceFactory", "D3D12 device not available");
        return nullptr;
    }

    HRESULT hr = d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr)) {
        LOG_ERROR("DX12ResourceFactory", "Failed to create D3D12 fence");
        return nullptr;
    }

    auto dx12Fence = std::make_unique<DX12Fence>(fence);

    // 更新统计信息

    LOG_INFO("DX12ResourceFactory", "Created fence");
    return std::move(dx12Fence);
}

std::vector<std::unique_ptr<ITexture>> DX12ResourceFactory::CreateTexturesBatch(const TextureDesc* descs,
                                                                            uint32_t count) {
    std::vector<std::unique_ptr<ITexture>> textures;
    textures.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        auto texture = CreateTextureImpl(descs[i]);
        if (texture) {
            textures.push_back(std::move(texture));
        }
    }

    LOG_INFO("DX12ResourceFactory", "Created {0} textures in batch", textures.size());
    return textures;
}

std::vector<std::unique_ptr<IBuffer>> DX12ResourceFactory::CreateBuffersBatch(const BufferDesc* descs,
                                                                             uint32_t count) {
    std::vector<std::unique_ptr<IBuffer>> buffers;
    buffers.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        auto buffer = CreateBufferImpl(descs[i]);
        if (buffer) {
            buffers.push_back(std::move(buffer));
        }
    }

    LOG_INFO("DX12ResourceFactory", "Created {0} buffers in batch", buffers.size());
    return buffers;
}

uint64_t DX12ResourceFactory::GetOrCreateTexturePool(TextureFormat format,
                                                    uint32_t width,
                                                    uint32_t height,
                                                    uint32_t mipLevels,
                                                    uint32_t arraySize) {
    TextureDesc desc = {};
    desc.type = TextureType::Texture2D;
    desc.format = format;
    desc.width = width;
    desc.height = height;
    desc.mipLevels = mipLevels;
    desc.arraySize = arraySize;
    desc.allowShaderResource = true;

    uint64_t poolKey = CalculateTexturePoolKey(desc);

    auto it = m_texturePools.find(poolKey);
    if (it == m_texturePools.end()) {
        auto pool = std::make_unique<TexturePool>();
        pool->format = format;
        pool->width = width;
        pool->height = height;
        pool->mipLevels = mipLevels;
        pool->arraySize = arraySize;

        uint64_t poolId = m_nextPoolId++;
        m_texturePools[poolKey] = std::move(pool);

        LOG_INFO("DX12ResourceFactory", "Created texture pool: {0}", poolId);
        return poolId;
    }

    return poolKey; // 返回池的键而不是ID
}

std::unique_ptr<ITexture> DX12ResourceFactory::AllocateFromTexturePool(uint64_t poolId) {
    auto it = m_texturePools.find(poolId);
    if (it == m_texturePools.end()) {
        LOG_ERROR("DX12ResourceFactory", "Texture pool not found: {0}", poolId);
        return nullptr;
    }

    auto& pool = it->second;
    if (!pool->freeTextures.empty()) {
        auto texture = std::move(pool->freeTextures.back());
        pool->freeTextures.pop_back();
        pool->totalAllocated++;
        pool->peakUsage = std::max(pool->peakUsage, pool->totalAllocated);
        return std::move(texture);
    }

    // 池中没有可用纹理，创建新的
    TextureDesc desc = {};
    desc.type = TextureType::Texture2D;
    desc.format = pool->format;
    desc.width = pool->width;
    desc.height = pool->height;
    desc.mipLevels = pool->mipLevels;
    desc.arraySize = pool->arraySize;
    desc.allowShaderResource = true;

    auto texture = CreateTextureImpl(desc);
    if (texture) {
        pool->totalAllocated++;
        pool->peakUsage = std::max(pool->peakUsage, pool->totalAllocated);
    }

    return std::move(texture);
}

void DX12ResourceFactory::DeallocateToTexturePool(uint64_t poolId, ITexture* texture) {
    if (!texture) {
        return;
    }

    auto it = m_texturePools.find(poolId);
    if (it == m_texturePools.end()) {
        LOG_ERROR("DX12ResourceFactory", "Texture pool not found: {0}", poolId);
        return;
    }

    auto& pool = it->second;
    auto dx12Texture = static_cast<DX12Texture*>(texture);
    pool->freeTextures.emplace_back(
        std::unique_ptr<DX12Texture>(dx12Texture)
    );
    pool->totalAllocated--;
}

void DX12ResourceFactory::CleanupResourcePools() {
    for (auto& pair : m_texturePools) {
        auto& pool = pair.second;
        if (pool->freeTextures.empty() && pool->totalAllocated == 0) {
            // 可以删除空池
            LOG_INFO("DX12ResourceFactory", "Cleaning up empty texture pool: {0}", pair.first);
        }
    }
}

bool DX12ResourceFactory::ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) {
    if (desc.width == 0 || desc.height == 0 || desc.depth == 0) {
        errorMsg = "Texture dimensions must be greater than 0";
        return false;
    }

    if (desc.format == TextureFormat::Unknown) {
        errorMsg = "Texture format cannot be unknown";
        return false;
    }

    if (desc.mipLevels == 0) {
        errorMsg = "Texture must have at least 1 mip level";
        return false;
    }

    if (desc.arraySize == 0) {
        errorMsg = "Texture array size must be greater than 0";
        return false;
    }

    return true;
}

bool DX12ResourceFactory::ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) {
    if (desc.size == 0) {
        errorMsg = "Buffer size must be greater than 0";
        return false;
    }

    if (desc.type == BufferType::Unknown) {
        errorMsg = "Buffer type cannot be unknown";
        return false;
    }

    // 检查常量缓冲区对齐
    if (desc.type == BufferType::Constant && (desc.size % 256 != 0)) {
        errorMsg = "Constant buffer size must be 256-byte aligned";
        return false;
    }

    return true;
}

bool DX12ResourceFactory::ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) {
    if (desc.type == ShaderType::Unknown) {
        errorMsg = "Shader type cannot be unknown";
        return false;
    }

    if (desc.entryPoint.empty()) {
        errorMsg = "Shader entry point cannot be empty";
        return false;
    }

    if (desc.target.empty()) {
        errorMsg = "Shader target cannot be empty";
        return false;
    }

    return true;
}

bool DX12ResourceFactory::ValidatePipelineDesc(const PipelineDesc& desc, std::string& errorMsg) {
    // 管线描述验证逻辑
    return true;
}

void DX12ResourceFactory::GetMemoryBudget(uint64_t& budget, uint64_t& usage) const {
    // TODO: 实现内存预算查询
    budget = 0;
    usage = m_stats.totalMemoryAllocated;
}

void DX12ResourceFactory::SetMemoryLimit(uint64_t limit) {
    m_memoryLimit = limit;
    LOG_INFO("DX12ResourceFactory", "Memory limit set to: {0} MB", limit / (1024 * 1024));
}

bool DX12ResourceFactory::IsMemoryLimitExceeded() const {
    if (m_memoryLimit == 0) {
        return false;
    }

    uint64_t usage = m_stats.totalMemoryAllocated;
    return usage > m_memoryLimit;
}

void DX12ResourceFactory::ForceGarbageCollection() {
    ProcessDeferredDestructions();
    CleanupResourcePools();
    LOG_INFO("DX12ResourceFactory", "Forced garbage collection completed");
}

ResourceCreationStats DX12ResourceFactory::GetCreationStats() const {
    return m_stats;
}

void DX12ResourceFactory::ResetStats() {
    m_stats = {};
    LOG_INFO("DX12ResourceFactory", "Stats reset");
}

void DX12ResourceFactory::EnableResourcePooling(bool enable) {
    m_resourcePoolingEnabled = enable;
    LOG_INFO("DX12ResourceFactory", "Resource pooling: {0}", enable ? "enabled" : "disabled");
}

void DX12ResourceFactory::SetPoolingThreshold(uint64_t threshold) {
    m_poolingThreshold = threshold;
    LOG_INFO("DX12ResourceFactory", "Pooling threshold set to: {0} bytes", threshold);
}

void DX12ResourceFactory::EnableDeferredDestruction(bool enable, uint32_t delayFrames) {
    m_deferredDestructionEnabled = enable;
    m_destructionDelayFrames = delayFrames;
    LOG_INFO("DX12ResourceFactory", "Deferred destruction: {0}, delay: {1} frames",
             enable ? "enabled" : "disabled", delayFrames);
}

void DX12ResourceFactory::ProcessDeferredDestructions() {
    if (!m_deferredDestructionEnabled) {
        return;
    }

    for (auto it = m_deferredResources.begin(); it != m_deferredResources.end();) {
        if (it->framesRemaining == 0) {
            it = m_deferredResources.erase(it);
        } else {
            it->framesRemaining--;
            ++it;
        }
    }
}

// DirectX12特定方法实现

bool DX12ResourceFactory::CompileShader(const ShaderDesc& desc,
                                       std::vector<uint8_t>& bytecode,
                                       ShaderReflection& reflection,
                                       std::string* errors) {
    // TODO: Implement shader compilation when DXC is available
    // This method should call DXC to compile shaders and generate reflection info
    if (errors) *errors = "Shader compilation not implemented yet - please provide pre-compiled bytecode";
    return false;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DX12ResourceFactory::CreateDescriptorHeap(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32_t numDescriptors,
    D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
    if (!m_device) {
        return nullptr;
    }

    auto d3d12Device = m_device->GetD3D12Device();
    if (!d3d12Device) {
        return nullptr;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = type;
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Flags = flags;
    heapDesc.NodeMask = 0;

    ComPtr<ID3D12DescriptorHeap> heap;
    HRESULT hr = d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
    if (FAILED(hr)) {
        LOG_ERROR("DX12ResourceFactory", "Failed to create descriptor heap");
        return nullptr;
    }

    return heap;
}

uint32_t DX12ResourceFactory::AllocateDescriptor(ID3D12DescriptorHeap* heap,
                                               D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
                                               D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle) {
    // TODO: 实现描述符分配
    // 这里需要维护一个描述符分配器
    return 0;
}

std::string DX12ResourceFactory::GetShaderModel(const std::string& target) const {
    // 从目标中提取着色器模型
    if (target.find("6_") != std::string::npos) {
        return "6_0";
    } else if (target.find("5_") != std::string::npos) {
        return "5_1";
    }
    return "6_0"; // 默认使用Shader Model 6.0
}

// 私有辅助方法实现

D3D12_RESOURCE_DESC DX12ResourceFactory::GetD3D12TextureDesc(const TextureDesc& desc) const {
    D3D12_RESOURCE_DESC d3d12Desc = {};
    d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d3d12Desc.Alignment = 0;
    d3d12Desc.Width = desc.width;
    d3d12Desc.Height = desc.height;
    d3d12Desc.DepthOrArraySize = desc.arraySize;
    d3d12Desc.MipLevels = desc.mipLevels;
    d3d12Desc.Format = GetDXGIFormat(desc.format);
    d3d12Desc.SampleDesc.Count = desc.sampleCount;
    d3d12Desc.SampleDesc.Quality = desc.sampleQuality;
    d3d12Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d3d12Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (desc.allowRenderTarget) {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (desc.allowDepthStencil) {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (desc.allowUnorderedAccess) {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return d3d12Desc;
}

D3D12_RESOURCE_DESC DX12ResourceFactory::GetD3D12BufferDesc(const BufferDesc& desc) const {
    D3D12_RESOURCE_DESC d3d12Desc = {};
    d3d12Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3d12Desc.Alignment = 0;
    d3d12Desc.Width = desc.size;
    d3d12Desc.Height = 1;
    d3d12Desc.DepthOrArraySize = 1;
    d3d12Desc.MipLevels = 1;
    d3d12Desc.Format = DXGI_FORMAT_UNKNOWN;
    d3d12Desc.SampleDesc.Count = 1;
    d3d12Desc.SampleDesc.Quality = 0;
    d3d12Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    d3d12Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (HasFlag(desc.usage, BufferUsage::ShaderResource)) {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SHADER_RESOURCE;
    }
    if (HasFlag(desc.usage, BufferUsage::UnorderedAccess)) {
        d3d12Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return d3d12Desc;
}

D3D12_HEAP_TYPE DX12ResourceFactory::GetHeapType(BufferUsage usage) const {
    if (HasFlag(usage, BufferUsage::Upload)) {
        return D3D12_HEAP_TYPE_UPLOAD;
    } else if (HasFlag(usage, BufferUsage::Readback)) {
        return D3D12_HEAP_TYPE_READBACK;
    } else {
        return D3D12_HEAP_TYPE_DEFAULT;
    }
}

D3D12_HEAP_FLAGS DX12ResourceFactory::GetHeapFlags(BufferUsage usage) const {
    return D3D12_HEAP_FLAG_NONE;
}

D3D12_RESOURCE_STATES DX12ResourceFactory::GetInitialResourceState(BufferType type, BufferUsage usage) const {
    if (HasFlag(usage, BufferUsage::Upload)) {
        return D3D12_RESOURCE_STATE_GENERIC_READ;
    } else if (HasFlag(usage, BufferUsage::Readback)) {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    } else if (HasFlag(usage, BufferUsage::ShaderResource)) {
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    } else if (HasFlag(usage, BufferUsage::UnorderedAccess)) {
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    } else {
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
}

Microsoft::WRL::ComPtr<ID3D12Resource> DX12ResourceFactory::CreateCommittedResource(
    const D3D12_RESOURCE_DESC& desc,
    D3D12_HEAP_TYPE heapType,
    D3D12_HEAP_FLAGS heapFlags,
    D3D12_RESOURCE_STATES initialState) {
    if (!m_device) {
        return nullptr;
    }

    auto d3d12Device = m_device->GetD3D12Device();
    if (!d3d12Device) {
        return nullptr;
    }

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 0;
    heapProps.VisibleNodeMask = 0;

    ComPtr<ID3D12Resource> resource;
    HRESULT hr = d3d12Device->CreateCommittedResource(
        &heapProps,
        heapFlags,
        &desc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&resource)
    );

    if (FAILED(hr)) {
        LOG_ERROR("DX12ResourceFactory", "Failed to create committed resource: HRESULT 0x{0:X}", hr);
        return nullptr;
    }

    return resource;
}

bool DX12ResourceFactory::LoadImageFromFile(const std::string& filename,
                                           std::vector<uint8_t>& imageData,
                                           TextureDesc& desc) {
    // TODO: Implement proper image loading without stb_image
    LOG_ERROR("DX12ResourceFactory", "Image loading from file not implemented yet: {0}", filename);
    return false;
}

uint64_t DX12ResourceFactory::CalculateTexturePoolKey(const TextureDesc& desc) const {
    uint64_t key = 0;
    key = (key << 8) | static_cast<uint64_t>(desc.type);
    key = (key << 8) | static_cast<uint64_t>(desc.format);
    key = (key << 16) | desc.width;
    key = (key << 16) | desc.height;
    key = (key << 8) | desc.mipLevels;
    key = (key << 8) | desc.arraySize;
    return key;
}

void DX12ResourceFactory::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t initialCapacity) {
    if (m_descriptorHeaps.find(type) != m_descriptorHeaps.end()) {
        return; // 已存在
    }

    auto heap = std::make_unique<DescriptorHeap>();
    heap->type = type;
    heap->capacity = initialCapacity;
    heap->usedCount = 0;

    heap->heap = CreateDescriptorHeap(type, initialCapacity);
    if (!heap->heap) {
        LOG_ERROR("DX12ResourceFactory", "Failed to create descriptor heap for type: {0}", type);
        return;
    }

    auto d3d12Device = m_device->GetD3D12Device();
    heap->descriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(type);
    heap->cpuStart = heap->heap->GetCPUDescriptorHandleForHeapStart();
    heap->gpuStart = heap->heap->GetGPUDescriptorHandleForHeapStart();

    m_descriptorHeaps[type] = std::move(heap);
    LOG_INFO("DX12ResourceFactory", "Created descriptor heap type: {0}, capacity: {1}", type, initialCapacity);
}

DXGI_FORMAT DX12ResourceFactory::GetDXGIFormat(TextureFormat format) const {
    switch (format) {
        case TextureFormat::Unknown: return DXGI_FORMAT_UNKNOWN;
        case TextureFormat::R32_Float: return DXGI_FORMAT_R32_FLOAT;
        case TextureFormat::R32_UInt: return DXGI_FORMAT_R32_UINT;
        case TextureFormat::R32_SInt: return DXGI_FORMAT_R32_SINT;
        case TextureFormat::R16_Float: return DXGI_FORMAT_R16_FLOAT;
        case TextureFormat::R16_UInt: return DXGI_FORMAT_R16_UINT;
        case TextureFormat::R16_SInt: return DXGI_FORMAT_R16_SINT;
        case TextureFormat::R8_UNorm: return DXGI_FORMAT_R8_UNORM;
        case TextureFormat::R8_SNorm: return DXGI_FORMAT_R8_SNORM;
        case TextureFormat::RG32_Float: return DXGI_FORMAT_R32G32_FLOAT;
        case TextureFormat::RG32_UInt: return DXGI_FORMAT_R32G32_UINT;
        case TextureFormat::RG32_SInt: return DXGI_FORMAT_R32G32_SINT;
        case TextureFormat::RG16_Float: return DXGI_FORMAT_R16G16_FLOAT;
        case TextureFormat::RG16_UInt: return DXGI_FORMAT_R16G16_UINT;
        case TextureFormat::RG16_SInt: return DXGI_FORMAT_R16G16_SINT;
        case TextureFormat::RG8_UNorm: return DXGI_FORMAT_R8G8_UNORM;
        case TextureFormat::RG8_SNorm: return DXGI_FORMAT_R8G8_SNORM;
        case TextureFormat::RGB32_Float: return DXGI_FORMAT_R32G32B32_FLOAT;
        case TextureFormat::RGB32_UInt: return DXGI_FORMAT_R32G32B32_UINT;
        case TextureFormat::RGB32_SInt: return DXGI_FORMAT_R32G32B32_SINT;
        case TextureFormat::RGBA8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_UNorm_sRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case TextureFormat::RGBA8_SNorm: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case TextureFormat::RGBA8_UInt: return DXGI_FORMAT_R8G8B8A8_UINT;
        case TextureFormat::RGBA8_SInt: return DXGI_FORMAT_R8G8B8A8_SINT;
        case TextureFormat::RGBA16_Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case TextureFormat::RGBA16_UInt: return DXGI_FORMAT_R16G16B16A16_UINT;
        case TextureFormat::RGBA16_SInt: return DXGI_FORMAT_R16G16B16A16_SINT;
        case TextureFormat::RGBA32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case TextureFormat::RGBA32_UInt: return DXGI_FORMAT_R32G32B32A32_UINT;
        case TextureFormat::RGBA32_SInt: return DXGI_FORMAT_R32G32B32A32_SINT;
        case TextureFormat::BGRA8_UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::BGRA8_UNorm_sRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case TextureFormat::D32_Float: return DXGI_FORMAT_D32_FLOAT;
        case TextureFormat::D24_UNorm_S8_UInt: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_Float_S8_UInt: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

} // namespace PrismaEngine::Graphic::DX12