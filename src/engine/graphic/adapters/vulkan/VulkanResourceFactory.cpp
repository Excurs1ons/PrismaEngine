#include "VulkanResourceFactory.h"
#include "RenderDeviceVulkan.h"
#include "VulkanResources.h"
#include "VulkanShader.h"
#include "VulkanPipelineState.h"
#include "VulkanSampler.h"
#include "VulkanSwapChain.h"
#include "VulkanFence.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <vk_mem_alloc.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include "Logger.h"
#include <fstream>

namespace Prisma::Graphic::Vulkan {

VulkanResourceFactory::VulkanResourceFactory(RenderDeviceVulkan* device)
    : m_device(device), m_vkDevice(device->GetVkDevice()), m_vmaAllocator(device->GetAllocator()) {
    LOG_INFO("Vulkan", "创建 Vulkan 资源工厂实例");
}

VulkanResourceFactory::~VulkanResourceFactory() {
    Shutdown();
}

bool VulkanResourceFactory::Initialize(IRenderDevice* device) {
    m_device = dynamic_cast<RenderDeviceVulkan*>(device);
    if (m_device) {
        m_vkDevice = m_device->GetVkDevice();
        m_vmaAllocator = m_device->GetAllocator();
    }
    return true;
}

void VulkanResourceFactory::Shutdown() {
    if (m_vkDevice != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_vkDevice);
    }
    m_device = nullptr;
    m_vkDevice = VK_NULL_HANDLE;
    // 不建议在这里将 m_vmaAllocator 置为 NULL，因为由它分配的资源（如 VulkanBuffer）
    // 可能会在此之后才执行析构函数（例如被 Application 成员引用）。
}

void VulkanResourceFactory::Reset() {
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureImpl(const TextureDesc& desc) {
    std::string errorMsg;
    if (!ValidateTextureDesc(desc, errorMsg)) {
        LOG_ERROR("Vulkan", "Invalid texture description: {0}", errorMsg);
        return nullptr;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth = desc.depth > 0 ? desc.depth : 1;
    imageInfo.mipLevels = desc.mipLevels > 0 ? desc.mipLevels : 1;
    imageInfo.arrayLayers = desc.arraySize;
    imageInfo.format = static_cast<VkFormat>(desc.format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = 0;

    if (desc.allowRenderTarget) imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (desc.allowDepthStencil) imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (desc.allowShaderResource) imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (desc.allowUnorderedAccess) imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (desc.allowRenderTarget || desc.allowDepthStencil) imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    imageInfo.samples = static_cast<VkSampleCountFlagBits>(desc.sampleCount);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    if (vmaCreateImage(m_vmaAllocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) return nullptr;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = desc.arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = desc.allowDepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = desc.arraySize;

    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        vmaDestroyImage(m_vmaAllocator, image, allocation);
        return nullptr;
    }

    auto texture = std::make_unique<VulkanTexture>(m_vmaAllocator, image, allocation, imageView, desc);
    ++m_creationStats.texturesCreated;
    const uint64_t estimatedBytes = static_cast<uint64_t>(desc.width) * desc.height *
                                    std::max<uint32_t>(1, desc.depth) *
                                    std::max<uint32_t>(1, desc.arraySize) * 4ull;
    m_creationStats.totalMemoryAllocated += estimatedBytes;
    m_creationStats.peakMemoryUsage = std::max(m_creationStats.peakMemoryUsage, m_creationStats.totalMemoryAllocated);
    return texture;
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromFile(const std::string& filename, const TextureDesc* desc) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        LOG_ERROR("Vulkan", "Failed to open texture file: {0}", filename);
        return nullptr;
    }

    const auto fileSize = static_cast<uint64_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
    if (fileSize > 0) {
        file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(fileSize));
    }

    TextureDesc resolvedDesc = desc ? *desc : TextureDesc{};
    if (resolvedDesc.width == 0) {
        resolvedDesc.width = 1;
    }
    if (resolvedDesc.height == 0) {
        resolvedDesc.height = 1;
    }
    if (resolvedDesc.depth == 0) {
        resolvedDesc.depth = 1;
    }
    if (resolvedDesc.arraySize == 0) {
        resolvedDesc.arraySize = 1;
    }
    if (resolvedDesc.mipLevels == 0) {
        resolvedDesc.mipLevels = 1;
    }
    resolvedDesc.allowShaderResource = true;

    return CreateTextureFromMemory(bytes.data(), bytes.size(), resolvedDesc);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    if (!data || dataSize == 0) {
        LOG_ERROR("Vulkan", "CreateTextureFromMemory requires non-empty source data");
        return nullptr;
    }

    return CreateTextureImpl(desc);
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateBufferImpl(const BufferDesc& desc) {
    std::string errorMsg;
    if (!ValidateBufferDesc(desc, errorMsg)) {
        LOG_ERROR("Vulkan", "Invalid buffer description: {0}", errorMsg);
        return nullptr;
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = 0;

    // 根据 BufferType 设置 Vulkan buffer usage
    switch (desc.type) {
        case BufferType::Vertex:
            bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case BufferType::Index:
            bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            break;
        case BufferType::Constant:
            bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;
        case BufferType::Structured:
            bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
        case BufferType::IndirectArgument:
            bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            break;
        default:
            break;
    }

    // 根据 BufferUsage 添加额外的 usage flags
    if (static_cast<uint32_t>(desc.usage) & static_cast<uint32_t>(BufferUsage::ShaderResource))
        bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    if (vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) return nullptr;

    auto createdBuffer = std::make_unique<VulkanBuffer>(m_vmaAllocator, buffer, allocation, desc);
    ++m_creationStats.buffersCreated;
    m_creationStats.totalMemoryAllocated += desc.size;
    m_creationStats.peakMemoryUsage = std::max(m_creationStats.peakMemoryUsage, m_creationStats.totalMemoryAllocated);
    return createdBuffer;
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateDynamicBuffer(uint64_t size, BufferType type, BufferUsage usage) {
    BufferDesc desc{};
    desc.size = size;
    desc.type = type;
    desc.usage = usage;
    return CreateBufferImpl(desc);
}

std::unique_ptr<IShader> VulkanResourceFactory::CreateShaderImpl(const ShaderDesc& desc, const std::vector<uint8_t>& bytecode, const ShaderReflection& reflection) {
    // 将 uint8_t 字节码转换为 uint32_t SPIR-V
    std::vector<uint32_t> spirv;
    if (bytecode.size() % 4 == 0) {
        spirv.resize(bytecode.size() / 4);
        memcpy(spirv.data(), bytecode.data(), bytecode.size());
    }
    ++m_creationStats.shadersCreated;
    return std::make_unique<VulkanShader>(m_device, desc, spirv, reflection);
}

std::unique_ptr<IPipelineState> VulkanResourceFactory::CreatePipelineStateImpl() {
    ++m_creationStats.pipelinesCreated;
    return std::make_unique<VulkanPipelineState>();
}

std::unique_ptr<ISampler> VulkanResourceFactory::CreateSamplerImpl(const SamplerDesc& desc) {
    ++m_creationStats.samplersCreated;
    return std::make_unique<VulkanSampler>(m_vkDevice, desc);
}

std::unique_ptr<ISwapChain> VulkanResourceFactory::CreateSwapChainImpl(void* windowHandle, uint32_t width, uint32_t height, TextureFormat format, uint32_t bufferCount, bool vsync) {
    if (!m_device || !windowHandle || width == 0 || height == 0) {
        return nullptr;
    }

    if (format != TextureFormat::RGBA8_UNorm) {
        LOG_WARNING("Vulkan", "Swapchain format override is not supported yet, falling back to RGBA8_UNorm");
    }
    if (bufferCount != 0 && bufferCount != 3) {
        LOG_WARNING("Vulkan", "Swapchain buffer count override is not supported yet, requested: {0}", bufferCount);
    }

    auto swapChain = std::make_unique<VulkanSwapChain>(m_device);
    if (swapChain->Initialize(windowHandle, width, height, vsync) != 0) {
        return nullptr;
    }
    return swapChain;
}
std::unique_ptr<IFence> VulkanResourceFactory::CreateFenceImpl() { return std::make_unique<VulkanFence>(m_vkDevice); }

std::vector<std::unique_ptr<ITexture>> VulkanResourceFactory::CreateTexturesBatch(const TextureDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<ITexture>> results;
    for (uint32_t i = 0; i < count; ++i) {
        auto texture = CreateTextureImpl(descs[i]);
        if (texture) results.push_back(std::move(texture));
    }
    return results;
}

std::vector<std::unique_ptr<IBuffer>> VulkanResourceFactory::CreateBuffersBatch(const BufferDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<IBuffer>> results;
    for (uint32_t i = 0; i < count; ++i) {
        auto buffer = CreateBufferImpl(descs[i]);
        if (buffer) results.push_back(std::move(buffer));
    }
    return results;
}

uint64_t VulkanResourceFactory::GetOrCreateTexturePool(TextureFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arraySize) {
    for (const auto& [poolId, desc] : m_texturePoolDescs) {
        if (desc.format == format && desc.width == width && desc.height == height &&
            desc.mipLevels == mipLevels && desc.arraySize == arraySize) {
            return poolId;
        }
    }

    TextureDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mipLevels = mipLevels == 0 ? 1 : mipLevels;
    desc.arraySize = arraySize == 0 ? 1 : arraySize;
    desc.format = format;
    desc.allowShaderResource = true;

    const uint64_t poolId = m_nextTexturePoolId++;
    m_texturePoolDescs.emplace(poolId, desc);
    m_texturePools.emplace(poolId, std::vector<std::unique_ptr<ITexture>>{});
    return poolId;
}

std::unique_ptr<ITexture> VulkanResourceFactory::AllocateFromTexturePool(uint64_t poolId) {
    auto poolIt = m_texturePools.find(poolId);
    if (poolIt == m_texturePools.end()) {
        return nullptr;
    }

    auto& pool = poolIt->second;
    if (!pool.empty()) {
        auto texture = std::move(pool.back());
        pool.pop_back();
        ++m_creationStats.texturesPooled;
        return texture;
    }

    auto descIt = m_texturePoolDescs.find(poolId);
    return descIt != m_texturePoolDescs.end() ? CreateTextureImpl(descIt->second) : nullptr;
}

void VulkanResourceFactory::DeallocateToTexturePool(uint64_t poolId, ITexture* texture) {
    if (!m_resourcePoolingEnabled || !texture) {
        return;
    }

    auto poolIt = m_texturePools.find(poolId);
    if (poolIt == m_texturePools.end()) {
        return;
    }

    LOG_DEBUG("Vulkan", "Texture returned to pool {0} is externally owned; caller should release through a pooled handle", poolId);
}
void VulkanResourceFactory::CleanupResourcePools() {}

bool VulkanResourceFactory::ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) {
    if (desc.width == 0 || desc.height == 0) {
        errorMsg = "Texture dimensions must be greater than zero";
        return false;
    }
    if (desc.arraySize == 0) {
        errorMsg = "Texture array size must be greater than zero";
        return false;
    }
    if (desc.mipLevels == 0) {
        errorMsg = "Texture mip level count must be greater than zero";
        return false;
    }
    errorMsg.clear();
    return true;
}

bool VulkanResourceFactory::ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) {
    if (desc.size == 0) {
        errorMsg = "Buffer size must be greater than zero";
        return false;
    }
    errorMsg.clear();
    return true;
}

bool VulkanResourceFactory::ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) {
    if (desc.entryPoint.empty()) {
        errorMsg = "Shader entry point is required";
        return false;
    }
    errorMsg.clear();
    return true;
}

void VulkanResourceFactory::GetMemoryBudget(uint64_t& budget, uint64_t& usage) const {
    budget = m_memoryLimit;
    usage = m_creationStats.totalMemoryAllocated;
}

void VulkanResourceFactory::SetMemoryLimit(uint64_t limit) { m_memoryLimit = limit; }
bool VulkanResourceFactory::IsMemoryLimitExceeded() const { return m_memoryLimit != 0 && m_creationStats.totalMemoryAllocated > m_memoryLimit; }
void VulkanResourceFactory::ForceGarbageCollection() { CleanupResourcePools(); }

IResourceFactory::ResourceCreationStats VulkanResourceFactory::GetCreationStats() const { return m_creationStats; }
void VulkanResourceFactory::ResetStats() { m_creationStats = {}; }

void VulkanResourceFactory::EnableResourcePooling(bool enable) { m_resourcePoolingEnabled = enable; }
void VulkanResourceFactory::SetPoolingThreshold(uint64_t threshold) { m_poolingThreshold = threshold; }
void VulkanResourceFactory::EnableDeferredDestruction(bool enable, uint32_t delayFrames) {
    m_deferredDestructionEnabled = enable;
    m_deferredDestructionDelayFrames = delayFrames;
}
void VulkanResourceFactory::ProcessDeferredDestructions() {
    if (!m_deferredDestructionEnabled || m_deferredDestructionDelayFrames == 0) {
        return;
    }
}

} // namespace Prisma::Graphic::Vulkan
