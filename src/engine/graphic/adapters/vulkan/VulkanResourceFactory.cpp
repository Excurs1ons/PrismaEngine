#include "VulkanResourceFactory.h"
#include "RenderDeviceVulkan.h"
#include "VulkanResources.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanPipelineState.h"
#include "VulkanSampler.h"
#include "VulkanSwapChain.h"
#include "VulkanFence.h"
#include <vma/vk_mem_alloc.h>

namespace Prisma::Graphic::Vulkan {

VulkanResourceFactory::VulkanResourceFactory(RenderDeviceVulkan* device)
    : m_device(device), m_vkDevice(device->GetDevice()), m_vmaAllocator(device->GetAllocator()) {
}

VulkanResourceFactory::~VulkanResourceFactory() {
    Shutdown();
}

bool VulkanResourceFactory::Initialize(IRenderDevice* device) {
    (void)device;
    return true;
}

void VulkanResourceFactory::Shutdown() {
    m_device = nullptr;
    m_vkDevice = VK_NULL_HANDLE;
    m_vmaAllocator = VK_NULL_HANDLE;
}

void VulkanResourceFactory::Reset() {
    // Reset factory state
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureImpl(const TextureDesc& desc) {
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

    if (desc.allowRenderTarget) {
        imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (desc.allowDepthStencil) {
        imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (desc.allowShaderResource) {
        imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.allowUnorderedAccess) {
        imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.allowRenderTarget || desc.allowDepthStencil) {
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    imageInfo.samples = static_cast<VkSampleCountFlagBits>(desc.sampleCount);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    VkResult result = vmaCreateImage(m_vmaAllocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
    if (result != VK_SUCCESS) {
        return nullptr;
    }

    // Create image view
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
    result = vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) {
        vmaDestroyImage(m_vmaAllocator, image, allocation);
        return nullptr;
    }

    // Store allocation for cleanup - we need to extend the VulkanTexture class
    // For now return a basic texture (the allocation will be leaked without proper wrapper)
    (void)allocation;

    return std::make_unique<VulkanTexture>(image, imageView, desc);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromFile(const std::string& filename, const TextureDesc* desc) {
    (void)filename;
    (void)desc;
    // TODO: Implement file loading with stb_image
    return nullptr;
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    (void)data;
    (void)dataSize;
    // TODO: Implement memory texture loading
    return nullptr;
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateBufferImpl(const BufferDesc& desc) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = 0;

    if (desc.usage & BufferUsage::Vertex) {
        bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::Index) {
        bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::Uniform) {
        bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::Storage) {
        bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::Indirect) {
        bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::ShaderResource) {
        bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    VkResult result = vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
    if (result != VK_SUCCESS) {
        return nullptr;
    }

    (void)allocation;

    return std::make_unique<VulkanBuffer>(buffer, desc);
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateDynamicBuffer(uint64_t size, BufferType type, BufferUsage usage) {
    BufferDesc desc{};
    desc.size = size;
    desc.type = type;
    desc.usage = usage;

    // Dynamic buffers use HOST_VISIBLE memory for CPU access
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;

    if (usage & BufferUsage::Vertex) {
        bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (usage & BufferUsage::Index) {
        bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (usage & BufferUsage::Uniform) {
        bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfoOut{};

    VkResult result = vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocInfoOut);
    if (result != VK_SUCCESS) {
        return nullptr;
    }

    (void)allocation;

    return std::make_unique<VulkanBuffer>(buffer, desc);
}

std::unique_ptr<IShader> VulkanResourceFactory::CreateShaderImpl(const ShaderDesc& desc, const std::vector<uint8_t>& bytecode, const ShaderReflection& reflection) {
    return std::make_unique<VulkanShader>(desc, bytecode, reflection);
}

std::unique_ptr<IPipelineState> VulkanResourceFactory::CreatePipelineStateImpl() {
    return std::make_unique<VulkanPipelineState>();
}

std::unique_ptr<ISampler> VulkanResourceFactory::CreateSamplerImpl(const SamplerDesc& desc) {
    return std::make_unique<VulkanSampler>(m_vkDevice, desc);
}

std::unique_ptr<ISwapChain> VulkanResourceFactory::CreateSwapChainImpl(void* windowHandle, uint32_t width, uint32_t height, TextureFormat format, uint32_t bufferCount, bool vsync) {
    (void)windowHandle;
    (void)width;
    (void)height;
    (void)format;
    (void)bufferCount;
    (void)vsync;
    // SwapChain is created by RenderDeviceVulkan directly
    // This method is for interface compliance
    return nullptr;
}

std::unique_ptr<IFence> VulkanResourceFactory::CreateFenceImpl() {
    return std::make_unique<VulkanFence>(m_vkDevice);
}

std::vector<std::unique_ptr<ITexture>> VulkanResourceFactory::CreateTexturesBatch(const TextureDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<ITexture>> results;
    results.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        auto texture = CreateTextureImpl(descs[i]);
        if (texture) {
            results.push_back(std::move(texture));
        }
    }

    return results;
}

std::vector<std::unique_ptr<IBuffer>> VulkanResourceFactory::CreateBuffersBatch(const BufferDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<IBuffer>> results;
    results.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        auto buffer = CreateBufferImpl(descs[i]);
        if (buffer) {
            results.push_back(std::move(buffer));
        }
    }

    return results;
}

uint64_t VulkanResourceFactory::GetOrCreateTexturePool(TextureFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arraySize) {
    (void)format;
    (void)width;
    (void)height;
    (void)mipLevels;
    (void)arraySize;
    // TODO: Implement texture pooling
    return 0;
}

std::unique_ptr<ITexture> VulkanResourceFactory::AllocateFromTexturePool(uint64_t poolId) {
    (void)poolId;
    return nullptr;
}

void VulkanResourceFactory::DeallocateToTexturePool(uint64_t poolId, ITexture* texture) {
    (void)poolId;
    (void)texture;
}

void VulkanResourceFactory::CleanupResourcePools() {
    // Cleanup unused pools
}

bool VulkanResourceFactory::ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) {
    if (desc.width == 0 || desc.height == 0) {
        errorMsg = "Texture dimensions must be non-zero";
        return false;
    }

    if (desc.width > 16384 || desc.height > 16384) {
        errorMsg = "Texture dimensions exceed maximum (16384)";
        return false;
    }

    return true;
}

bool VulkanResourceFactory::ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) {
    if (desc.size == 0) {
        errorMsg = "Buffer size must be non-zero";
        return false;
    }

    return true;
}

bool VulkanResourceFactory::ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) {
    if (desc.bytecode.empty()) {
        errorMsg = "Shader bytecode is empty";
        return false;
    }

    return true;
}

void VulkanResourceFactory::GetMemoryBudget(uint64_t& budget, uint64_t& usage) const {
    VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
    vmaGetMemoryBudget(m_vmaAllocator, budgets);

    budget = 0;
    usage = 0;
    for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; ++i) {
        budget += budgets[i].budget;
        usage += budgets[i].usage;
    }
}

void VulkanResourceFactory::SetMemoryLimit(uint64_t limit) {
    (void)limit;
    // TODO: Implement memory limit
}

bool VulkanResourceFactory::IsMemoryLimitExceeded() const {
    uint64_t budget = 0;
    uint64_t usage = 0;
    const_cast<VulkanResourceFactory*>(this)->GetMemoryBudget(budget, usage);
    return false; // TODO: Implement proper check
}

void VulkanResourceFactory::ForceGarbageCollection() {
    vmaGarbageCollect(m_vmaAllocator);
}

IResourceFactory::ResourceCreationStats VulkanResourceFactory::GetCreationStats() const {
    return {};
}

void VulkanResourceFactory::ResetStats() {
    // Reset creation stats
}

void VulkanResourceFactory::EnableResourcePooling(bool enable) {
    (void)enable;
}

void VulkanResourceFactory::SetPoolingThreshold(uint64_t threshold) {
    (void)threshold;
}

void VulkanResourceFactory::EnableDeferredDestruction(bool enable, uint32_t delayFrames) {
    (void)enable;
    (void)delayFrames;
}

void VulkanResourceFactory::ProcessDeferredDestructions() {
    // Process deferred destruction queue
}

} // namespace Prisma::Graphic::Vulkan
