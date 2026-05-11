#include "VulkanResourceFactory.h"
#include "RenderDeviceVulkan.h"
#include "VulkanResources.h"
#include "VulkanShader.h"
#include "VulkanPipelineState.h"
#include "VulkanSampler.h"
#include "VulkanSwapChain.h"
#include "VulkanFence.h"
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
#pragma warning(disable: 4505) // unreferenced local function has been removed
#endif
#include <vk_mem_alloc.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#include "logger/Logger.h"
#include <fstream>
#include <cstring>

namespace Prisma::Graphic::Vulkan {

static VkFormat TextureFormatToVkFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8_UNorm:            return VK_FORMAT_R8_UNORM;
        case TextureFormat::R8_SNorm:            return VK_FORMAT_R8_SNORM;
        case TextureFormat::R8_UInt:             return VK_FORMAT_R8_UINT;
        case TextureFormat::R8_SInt:             return VK_FORMAT_R8_SINT;
        case TextureFormat::RG8_UNorm:           return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::RG8_SNorm:           return VK_FORMAT_R8G8_SNORM;
        case TextureFormat::R16_UNorm:           return VK_FORMAT_D16_UNORM;
        case TextureFormat::R16_SNorm:           return VK_FORMAT_R16_SNORM;
        case TextureFormat::R16_Float:           return VK_FORMAT_R16_SFLOAT;
        case TextureFormat::R16_UInt:            return VK_FORMAT_R16_UINT;
        case TextureFormat::R16_SInt:            return VK_FORMAT_R16_SINT;
        case TextureFormat::RG16_UNorm:          return VK_FORMAT_R16G16_UNORM;
        case TextureFormat::RG16_SNorm:          return VK_FORMAT_R16G16_SNORM;
        case TextureFormat::RG16_Float:          return VK_FORMAT_R16G16_SFLOAT;
        case TextureFormat::RG16_UInt:           return VK_FORMAT_R16G16_UINT;
        case TextureFormat::RG16_SInt:           return VK_FORMAT_R16G16_SINT;
        case TextureFormat::RGBA16_UNorm:        return VK_FORMAT_R16G16B16A16_UNORM;
        case TextureFormat::RGBA16_SNorm:        return VK_FORMAT_R16G16B16A16_SNORM;
        case TextureFormat::RGBA16_Float:        return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::RGBA16_UInt:         return VK_FORMAT_R16G16B16A16_UINT;
        case TextureFormat::RGBA16_SInt:         return VK_FORMAT_R16G16B16A16_SINT;
        case TextureFormat::R32_Float:           return VK_FORMAT_R32_SFLOAT;
        case TextureFormat::R32_UInt:            return VK_FORMAT_R32_UINT;
        case TextureFormat::R32_SInt:            return VK_FORMAT_R32_SINT;
        case TextureFormat::RG32_Float:          return VK_FORMAT_R32G32_SFLOAT;
        case TextureFormat::RG32_UInt:           return VK_FORMAT_R32G32_UINT;
        case TextureFormat::RG32_SInt:           return VK_FORMAT_R32G32_SINT;
        case TextureFormat::RGB32_Float:         return VK_FORMAT_R32G32B32_SFLOAT;
        case TextureFormat::RGB32_UInt:          return VK_FORMAT_R32G32B32_UINT;
        case TextureFormat::RGB32_SInt:          return VK_FORMAT_R32G32B32_SINT;
        case TextureFormat::RGBA32_Float:        return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::RGBA32_UInt:         return VK_FORMAT_R32G32B32A32_UINT;
        case TextureFormat::RGBA32_SInt:         return VK_FORMAT_R32G32B32A32_SINT;
        case TextureFormat::RGB8_UNorm:          return VK_FORMAT_R8G8B8_UNORM;
        case TextureFormat::RGBA8_UNorm:         return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_UNorm_sRGB:    return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::RGBA8_SNorm:         return VK_FORMAT_R8G8B8A8_SNORM;
        case TextureFormat::RGBA8_UInt:          return VK_FORMAT_R8G8B8A8_UINT;
        case TextureFormat::RGBA8_SInt:          return VK_FORMAT_R8G8B8A8_SINT;
        case TextureFormat::BGRA8_UNorm:         return VK_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::BGRA8_UNorm_sRGB:    return VK_FORMAT_B8G8R8A8_SRGB;
        case TextureFormat::D16_UNorm:           return VK_FORMAT_D16_UNORM;
        case TextureFormat::D24_UNorm_S8_UInt:   return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_Float:           return VK_FORMAT_D32_SFLOAT;
        case TextureFormat::D32_Float_S8_UInt:   return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case TextureFormat::BC1_UNorm:           return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case TextureFormat::BC1_SRGB:            return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case TextureFormat::BC2_UNorm:           return VK_FORMAT_BC2_UNORM_BLOCK;
        case TextureFormat::BC2_SRGB:            return VK_FORMAT_BC2_SRGB_BLOCK;
        case TextureFormat::BC3_UNorm:           return VK_FORMAT_BC3_UNORM_BLOCK;
        case TextureFormat::BC3_SRGB:            return VK_FORMAT_BC3_SRGB_BLOCK;
        case TextureFormat::BC4_UNorm:           return VK_FORMAT_BC4_UNORM_BLOCK;
        case TextureFormat::BC4_SNorm:           return VK_FORMAT_BC4_SNORM_BLOCK;
        case TextureFormat::BC5_UNorm:           return VK_FORMAT_BC5_UNORM_BLOCK;
        case TextureFormat::BC5_SNorm:           return VK_FORMAT_BC5_SNORM_BLOCK;
        case TextureFormat::BC7_UNorm:           return VK_FORMAT_BC7_UNORM_BLOCK;
        case TextureFormat::BC7_SRGB:            return VK_FORMAT_BC7_SRGB_BLOCK;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

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
        if (m_device && m_device->GetVkDevice() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_vkDevice);
        }
    }
    m_device = nullptr;
    m_vkDevice = VK_NULL_HANDLE;
    m_vmaAllocator = VK_NULL_HANDLE;
}

void VulkanResourceFactory::Reset() {}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureImpl(const TextureDesc& desc) {
    std::string errorMsg;
    if (!ValidateTextureDesc(desc, errorMsg)) {
        LOG_ERROR("Vulkan", "Invalid texture description: {0}", errorMsg);
        return nullptr;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(desc.width);
    imageInfo.extent.height = static_cast<uint32_t>(desc.height);
    imageInfo.extent.depth = desc.depth > 0 ? desc.depth : 1;
    imageInfo.mipLevels = desc.mipLevels > 0 ? desc.mipLevels : 1;
    imageInfo.arrayLayers = desc.arraySize;
    imageInfo.format = TextureFormatToVkFormat(desc.format);
    if (imageInfo.format == VK_FORMAT_UNDEFINED) {
        LOG_ERROR("Vulkan", "Unsupported or unknown TextureFormat: {0}", static_cast<int>(desc.format));
        return nullptr;
    }

    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = 0;
    if (desc.allowRenderTarget)    imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (desc.allowDepthStencil)    imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (desc.allowShaderResource)  imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (desc.allowUnorderedAccess) imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (desc.allowRenderTarget || desc.allowDepthStencil) imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (imageInfo.usage == VK_IMAGE_USAGE_TRANSFER_DST_BIT) imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

    const uint32_t sc = desc.sampleCount > 0 ? desc.sampleCount : 1;
    imageInfo.samples = static_cast<VkSampleCountFlagBits>(sc);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    if (vmaCreateImage(m_vmaAllocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
        LOG_ERROR("Vulkan", "vmaCreateImage failed for texture '{0}' ({1}x{2} fmt={3})", desc.name, desc.width, desc.height, static_cast<int>(desc.format));
        return nullptr;
    }

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

    auto texture = std::make_unique<VulkanTexture>(
        m_vkDevice, 
        m_vmaAllocator, 
        m_device->GetGraphicsQueue(), 
        m_device->GetGraphicsQueueFamily(),
        image, 
        allocation, 
        imageView, 
        imageInfo.format, 
        desc
    );

    if (desc.allowShaderResource) {
        VkCommandPool tempPool;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = m_device->GetGraphicsQueueFamily();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if (vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &tempPool) == VK_SUCCESS) {
            VkCommandBufferAllocateInfo cmdAllocInfo{};
            cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdAllocInfo.commandPool = tempPool;
            cmdAllocInfo.commandBufferCount = 1;
            VkCommandBuffer commandBuffer;
            if (vkAllocateCommandBuffers(m_vkDevice, &cmdAllocInfo, &commandBuffer) == VK_SUCCESS) {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                vkBeginCommandBuffer(commandBuffer, &beginInfo);
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // 统一使用只读优化布局
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = image;
                barrier.subresourceRange.aspectMask = desc.allowDepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = imageInfo.mipLevels;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = desc.arraySize;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
                vkEndCommandBuffer(commandBuffer);
                VkSubmitInfo submitInfo{}; submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; submitInfo.commandBufferCount = 1; submitInfo.pCommandBuffers = &commandBuffer;
                vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
                vkQueueWaitIdle(m_device->GetGraphicsQueue());
                vkFreeCommandBuffers(m_vkDevice, tempPool, 1, &commandBuffer);
            }
            vkDestroyCommandPool(m_vkDevice, tempPool, nullptr);
        }
    }

    ++m_creationStats.texturesCreated;
    m_creationStats.totalMemoryAllocated += static_cast<uint64_t>(desc.width) * desc.height * 4;
    return texture;
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromFile(const std::string& filename, const TextureDesc* desc) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return nullptr;
    const auto fileSize = static_cast<uint64_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
    if (fileSize > 0) file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(fileSize));
    TextureDesc resolvedDesc = desc ? *desc : TextureDesc{};
    if (resolvedDesc.width == 0) resolvedDesc.width = 1;
    if (resolvedDesc.height == 0) resolvedDesc.height = 1;
    resolvedDesc.allowShaderResource = true;
    return CreateTextureFromMemory(bytes.data(), bytes.size(), resolvedDesc);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    auto texture = CreateTextureImpl(desc);
    if (!texture) return nullptr;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = VK_NULL_HANDLE;
    if (vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) return texture;
    void* mappedData = nullptr;
    if (vmaMapMemory(m_vmaAllocator, stagingAllocation, &mappedData) == VK_SUCCESS) {
        std::memcpy(mappedData, data, static_cast<size_t>(dataSize));
        vmaUnmapMemory(m_vmaAllocator, stagingAllocation);
    }
    auto* vkTexture = static_cast<VulkanTexture*>(texture.get());
    VkImage image = vkTexture->GetVkImage();
    VkCommandPool tempPool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_device->GetGraphicsQueueFamily();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    if (vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &tempPool) == VK_SUCCESS) {
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandPool = tempPool;
        cmdAllocInfo.commandBufferCount = 1;
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(m_vkDevice, &cmdAllocInfo, &cmdBuffer) == VK_SUCCESS) {
            VkCommandBufferBeginInfo beginInfo{}; beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cmdBuffer, &beginInfo);
            VkImageMemoryBarrier preBarrier{};
            preBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            preBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            preBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            preBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            preBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            preBarrier.image = image;
            preBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            preBarrier.subresourceRange.baseMipLevel = 0;
            preBarrier.subresourceRange.levelCount = 1;
            preBarrier.subresourceRange.baseArrayLayer = 0;
            preBarrier.subresourceRange.layerCount = desc.arraySize;
            preBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            preBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &preBarrier);
            VkBufferImageCopy copyRegion{};
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.layerCount = desc.arraySize;
            copyRegion.imageExtent = { static_cast<uint32_t>(desc.width), static_cast<uint32_t>(desc.height), 1u };
            vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
            VkImageMemoryBarrier postBarrier{};
            postBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            postBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            postBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            postBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            postBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            postBarrier.image = image;
            postBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            postBarrier.subresourceRange.baseMipLevel = 0;
            postBarrier.subresourceRange.levelCount = 1;
            postBarrier.subresourceRange.baseArrayLayer = 0;
            postBarrier.subresourceRange.layerCount = desc.arraySize;
            postBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            postBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &postBarrier);
            vkEndCommandBuffer(cmdBuffer);
            VkSubmitInfo submitInfo{}; submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; submitInfo.commandBufferCount = 1; submitInfo.pCommandBuffers = &cmdBuffer;
            vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(m_device->GetGraphicsQueue());
            vkFreeCommandBuffers(m_vkDevice, tempPool, 1, &cmdBuffer);
        }
        vkDestroyCommandPool(m_vkDevice, tempPool, nullptr);
    }
    vmaDestroyBuffer(m_vmaAllocator, stagingBuffer, stagingAllocation);
    return texture;
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateBufferImpl(const BufferDesc& desc) {
    std::string errorMsg;
    if (!ValidateBufferDesc(desc, errorMsg)) return nullptr;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = 0;
    switch (desc.type) {
        case BufferType::Vertex: bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
        case BufferType::Index:  bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
        case BufferType::Constant: bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
        case BufferType::Structured: bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; break;
        case BufferType::IndirectArgument: bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT; break;
        default: break;
    }
    if (static_cast<uint32_t>(desc.usage) & static_cast<uint32_t>(BufferUsage::ShaderResource))
        bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    VkBuffer buffer = VK_NULL_HANDLE; VmaAllocation allocation = VK_NULL_HANDLE;
    if (vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) return nullptr;
    if (desc.initialData && desc.size > 0) {
        void* mappedData = nullptr;
        if (vmaMapMemory(m_vmaAllocator, allocation, &mappedData) == VK_SUCCESS) {
            memcpy(mappedData, desc.initialData, desc.size);
            vmaUnmapMemory(m_vmaAllocator, allocation);
        }
    }
    auto createdBuffer = std::make_unique<VulkanBuffer>(m_vmaAllocator, buffer, allocation, desc);
    ++m_creationStats.buffersCreated;
    m_creationStats.totalMemoryAllocated += desc.size;
    return createdBuffer;
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateDynamicBuffer(uint64_t size, BufferType type, BufferUsage usage) {
    BufferDesc desc{}; desc.size = size; desc.type = type; desc.usage = usage;
    return CreateBufferImpl(desc);
}

std::unique_ptr<IShader> VulkanResourceFactory::CreateShaderImpl(const ShaderDesc& desc, const std::vector<uint8_t>& bytecode, const ShaderReflection& reflection) {
    std::vector<uint32_t> spirv;
    if (bytecode.size() % 4 == 0) { spirv.resize(bytecode.size() / 4); memcpy(spirv.data(), bytecode.data(), bytecode.size()); }
    ++m_creationStats.shadersCreated;
    return std::make_unique<VulkanShader>(m_device, desc, spirv, reflection);
}

std::unique_ptr<IPipelineState> VulkanResourceFactory::CreatePipelineStateImpl() { ++m_creationStats.pipelinesCreated; return std::make_unique<VulkanPipelineState>(); }
std::unique_ptr<ISampler> VulkanResourceFactory::CreateSamplerImpl(const SamplerDesc& desc) { ++m_creationStats.samplersCreated; return std::make_unique<VulkanSampler>(m_vkDevice, desc); }
std::unique_ptr<ISwapChain> VulkanResourceFactory::CreateSwapChainImpl(void* windowHandle, uint32_t width, uint32_t height, TextureFormat format, uint32_t bufferCount, PresentMode presentMode) {
    if (!m_device || !windowHandle || width == 0 || height == 0) return nullptr;
    auto swapChain = std::make_unique<VulkanSwapChain>(m_device);
    if (swapChain->Initialize(windowHandle, width, height, presentMode) != 0) return nullptr;
    return swapChain;
}
std::unique_ptr<IFence> VulkanResourceFactory::CreateFenceImpl() { return std::make_unique<VulkanFence>(m_vkDevice); }

std::vector<std::unique_ptr<ITexture>> VulkanResourceFactory::CreateTexturesBatch(const TextureDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<ITexture>> results;
    for (uint32_t i = 0; i < count; ++i) { auto texture = CreateTextureImpl(descs[i]); if (texture) results.push_back(std::move(texture)); }
    return results;
}

std::vector<std::unique_ptr<IBuffer>> VulkanResourceFactory::CreateBuffersBatch(const BufferDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<IBuffer>> results;
    for (uint32_t i = 0; i < count; ++i) { auto buffer = CreateBufferImpl(descs[i]); if (buffer) results.push_back(std::move(buffer)); }
    return results;
}

std::shared_ptr<Prisma::Graphic::IDescriptorSet> VulkanResourceFactory::CreateDescriptorSet(Prisma::Graphic::IDescriptorSetLayout* layout) {
    auto* vkLayout = dynamic_cast<VulkanDescriptorSetLayout*>(layout);
    if (!vkLayout || !m_device) return nullptr;
    VkDescriptorSetLayout layouts[] = { vkLayout->GetVkLayout() };
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_device->GetVkDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;
    VkDescriptorSet set;
    if (vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &set) != VK_SUCCESS) return nullptr;
    return std::make_shared<VulkanDescriptorSet>(m_vkDevice, set, std::static_pointer_cast<VulkanDescriptorSetLayout>(vkLayout->shared_from_this()));
}

std::shared_ptr<Prisma::Graphic::IDescriptorSetLayout> VulkanResourceFactory::CreateDescriptorSetLayout(const std::vector<Prisma::Graphic::ShaderResource>& resources) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& res : resources) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = res.Binding;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_ALL;
        switch (res.ResourceType) {
            case Prisma::Graphic::ShaderResource::Type::Sampler2D:
            case Prisma::Graphic::ShaderResource::Type::SamplerCube:
            case Prisma::Graphic::ShaderResource::Type::Image2D: binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
            case Prisma::Graphic::ShaderResource::Type::UniformBuffer: binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
            default: continue;
        }
        bindings.push_back(binding);
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &layout) != VK_SUCCESS) return nullptr;
    return std::make_shared<VulkanDescriptorSetLayout>(m_vkDevice, layout);
}

uint64_t VulkanResourceFactory::GetOrCreateTexturePool(TextureFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arraySize) {
    for (const auto& [poolId, desc] : m_texturePoolDescs) {
        if (desc.format == format && desc.width == width && desc.height == height && desc.mipLevels == mipLevels && desc.arraySize == arraySize) return poolId;
    }
    TextureDesc desc{}; desc.width = width; desc.height = height; desc.format = format; desc.allowShaderResource = true;
    const uint64_t poolId = m_nextTexturePoolId++;
    m_texturePoolDescs.emplace(poolId, desc); m_texturePools.emplace(poolId, std::vector<std::unique_ptr<ITexture>>{});
    return poolId;
}

std::unique_ptr<ITexture> VulkanResourceFactory::AllocateFromTexturePool(uint64_t poolId) {
    auto poolIt = m_texturePools.find(poolId); if (poolIt == m_texturePools.end()) return nullptr;
    auto& pool = poolIt->second;
    if (!pool.empty()) { auto texture = std::move(pool.back()); pool.pop_back(); ++m_creationStats.texturesPooled; return texture; }
    auto descIt = m_texturePoolDescs.find(poolId); return descIt != m_texturePoolDescs.end() ? CreateTextureImpl(descIt->second) : nullptr;
}

void VulkanResourceFactory::DeallocateToTexturePool(uint64_t poolId, ITexture* texture) {}
void VulkanResourceFactory::CleanupResourcePools() {}

bool VulkanResourceFactory::ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) {
    if (desc.width == 0 || desc.height == 0) { errorMsg = "Texture dimensions must be greater than zero"; return false; }
    return true;
}

bool VulkanResourceFactory::ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) {
    if (desc.size == 0) { errorMsg = "Buffer size must be greater than zero"; return false; }
    return true;
}

bool VulkanResourceFactory::ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) {
    if (desc.entryPoint.empty()) { errorMsg = "Shader entry point is required"; return false; }
    return true;
}

void VulkanResourceFactory::GetMemoryBudget(uint64_t& budget, uint64_t& usage) const { budget = m_memoryLimit; usage = m_creationStats.totalMemoryAllocated; }
void VulkanResourceFactory::SetMemoryLimit(uint64_t limit) { m_memoryLimit = limit; }
bool VulkanResourceFactory::IsMemoryLimitExceeded() const { return m_memoryLimit != 0 && m_creationStats.totalMemoryAllocated > m_memoryLimit; }
void VulkanResourceFactory::ForceGarbageCollection() { CleanupResourcePools(); }

IResourceFactory::ResourceCreationStats VulkanResourceFactory::GetCreationStats() const { return m_creationStats; }
void VulkanResourceFactory::ResetStats() { m_creationStats = {}; }

void VulkanResourceFactory::EnableResourcePooling(bool enable) { m_resourcePoolingEnabled = enable; }
void VulkanResourceFactory::SetPoolingThreshold(uint64_t threshold) { m_poolingThreshold = threshold; }
void VulkanResourceFactory::EnableDeferredDestruction(bool enable, uint32_t delayFrames) { m_deferredDestructionEnabled = enable; m_deferredDestructionDelayFrames = delayFrames; }
void VulkanResourceFactory::ProcessDeferredDestructions() {}

} // namespace Prisma::Graphic::Vulkan
