#include "VulkanResourceFactory.h"
#include "RenderDeviceVulkan.h"
#include "VulkanResources.h"
#include "VulkanShader.h"
#include "VulkanPipelineState.h"
#include "VulkanSampler.h"
#include "VulkanSwapChain.h"
#include "VulkanFence.h"
#include <vk_mem_alloc.h>
#include "Logger.h"

namespace Prisma::Graphic::Vulkan {

VulkanResourceFactory::VulkanResourceFactory(RenderDeviceVulkan* device)
    : m_device(device), m_vkDevice(device->GetVkDevice()), m_vmaAllocator(device->GetAllocator()) {
    LOG_INFO("Vulkan", "创建 Vulkan 资源工厂实例");
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

    return std::make_unique<VulkanTexture>(image, imageView, desc);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromFile(const std::string& filename, const TextureDesc* desc) { return nullptr; }
std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) { return nullptr; }

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateBufferImpl(const BufferDesc& desc) {
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

    return std::make_unique<VulkanBuffer>(buffer, desc);
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
    return std::make_unique<VulkanShader>(m_device, desc, spirv, reflection);
}

std::unique_ptr<IPipelineState> VulkanResourceFactory::CreatePipelineStateImpl() {
    return std::make_unique<VulkanPipelineState>();
}

std::unique_ptr<ISampler> VulkanResourceFactory::CreateSamplerImpl(const SamplerDesc& desc) {
    return std::make_unique<VulkanSampler>(m_vkDevice, desc);
}

std::unique_ptr<ISwapChain> VulkanResourceFactory::CreateSwapChainImpl(void* windowHandle, uint32_t width, uint32_t height, TextureFormat format, uint32_t bufferCount, bool vsync) { return nullptr; }
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

uint64_t VulkanResourceFactory::GetOrCreateTexturePool(TextureFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arraySize) { return 0; }
std::unique_ptr<ITexture> VulkanResourceFactory::AllocateFromTexturePool(uint64_t poolId) { return nullptr; }
void VulkanResourceFactory::DeallocateToTexturePool(uint64_t poolId, ITexture* texture) {}
void VulkanResourceFactory::CleanupResourcePools() {}

bool VulkanResourceFactory::ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) { return true; }
bool VulkanResourceFactory::ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) { return true; }
bool VulkanResourceFactory::ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) { return true; }

void VulkanResourceFactory::GetMemoryBudget(uint64_t& budget, uint64_t& usage) const {
    budget = 0; usage = 0;
}

void VulkanResourceFactory::SetMemoryLimit(uint64_t limit) {}
bool VulkanResourceFactory::IsMemoryLimitExceeded() const { return false; }
void VulkanResourceFactory::ForceGarbageCollection() {}

IResourceFactory::ResourceCreationStats VulkanResourceFactory::GetCreationStats() const { return {}; }
void VulkanResourceFactory::ResetStats() {}

void VulkanResourceFactory::EnableResourcePooling(bool enable) {}
void VulkanResourceFactory::SetPoolingThreshold(uint64_t threshold) {}
void VulkanResourceFactory::EnableDeferredDestruction(bool enable, uint32_t delayFrames) {}
void VulkanResourceFactory::ProcessDeferredDestructions() {}

} // namespace Prisma::Graphic::Vulkan
