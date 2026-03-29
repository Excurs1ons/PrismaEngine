#pragma once
#include "VulkanResources.h"
#include "RenderDeviceVulkan.h"
#include <algorithm>
#include "Export.h"

namespace Prisma::Graphic::Vulkan {

// -----------------------------------------------------------------------
// [改动] VulkanTexture 构造函数
//
// 目的：
//   接收并存储纹理的原始 VkFormat 以及绘图队列信息。
// -----------------------------------------------------------------------
VulkanTexture::VulkanTexture(VkDevice device, VmaAllocator allocator, VkQueue graphicsQueue, uint32_t graphicsQueueIndex, 
                             VkImage image, VmaAllocation allocation, VkImageView imageView, VkFormat vkFormat, const TextureDesc& desc)
    : m_device(device), m_allocator(allocator), m_graphicsQueue(graphicsQueue), m_graphicsQueueIndex(graphicsQueueIndex),
      m_image(image), m_allocation(allocation), m_imageView(imageView), m_vkFormat(vkFormat), m_desc(desc) {
}

VulkanTexture::~VulkanTexture() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }
    }

    if (m_allocator) {
        if (m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
            vmaDestroyImage(m_allocator, m_image, m_allocation);
        }
    }
}

void VulkanTexture::SetDebugName(const std::string& name) {
    if (m_device == VK_NULL_HANDLE || m_image == VK_NULL_HANDLE) return;

    auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_device, "vkSetDebugUtilsObjectNameEXT");
    if (func) {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
        nameInfo.objectHandle = (uint64_t)m_image;
        nameInfo.pObjectName = name.c_str();
        func(m_device, &nameInfo);

        if (m_imageView != VK_NULL_HANDLE) {
            nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
            nameInfo.objectHandle = (uint64_t)m_imageView;
            func(m_device, &nameInfo);
        }
    }
}

void VulkanTexture::DownloadFromGPU() {
    if (m_device == VK_NULL_HANDLE || m_image == VK_NULL_HANDLE || m_allocator == VK_NULL_HANDLE || m_graphicsQueue == VK_NULL_HANDLE) return;

    uint64_t size = GetSubresourceSize(0);
    if (m_shadowData.size() < size) {
        m_shadowData.resize(static_cast<size_t>(size));
    }

    // 1. 创建 Staging Buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;

    if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) {
        return;
    }

    // 2. 创建临时指令缓冲
    VkCommandPool tempPool;
    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = m_graphicsQueueIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    vkCreateCommandPool(m_device, &poolInfo, nullptr, &tempPool);

    VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdAllocInfo.commandPool = tempPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // 3. 布局转换并拷贝
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.image = m_image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region = {};
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageExtent = { (uint32_t)m_desc.width, (uint32_t)m_desc.height, 1 };
    vkCmdCopyImageToBuffer(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

    // 还原布局
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    // 4. 提交
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    // 5. 读回数据
    void* mappedData;
    vmaMapMemory(m_allocator, stagingAllocation, &mappedData);
    memcpy(m_shadowData.data(), mappedData, (size_t)size);
    vmaUnmapMemory(m_allocator, stagingAllocation);

    // 6. 清理
    vkFreeCommandBuffers(m_device, tempPool, 1, &cmd);
    vkDestroyCommandPool(m_device, tempPool, nullptr);
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
}

uint64_t VulkanTexture::GetBytesPerPixel() const {
    switch (m_desc.format) {
        case TextureFormat::RGBA8_UNorm: return 4;
        case TextureFormat::RGB8_UNorm: return 3;
        case TextureFormat::RG8_UNorm: return 2;
        case TextureFormat::R8_UNorm: return 1;
        case TextureFormat::RGBA16_Float: return 8;
        case TextureFormat::RGBA32_Float: return 16;
        case TextureFormat::D32_Float: return 4;
        case TextureFormat::D24_UNorm_S8_UInt: return 4;
        default: return 4;
    }
}

uint64_t VulkanTexture::GetSubresourceSize(uint32_t mipLevel) const {
    uint64_t mipWidth = std::max<uint64_t>(1, m_desc.width >> mipLevel);
    uint64_t mipHeight = std::max<uint64_t>(1, m_desc.height >> mipLevel);
    return mipWidth * mipHeight * GetBytesPerPixel();
}

VulkanBuffer::VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, const BufferDesc& desc)
    : m_allocator(allocator), m_buffer(buffer), m_allocation(allocation), m_desc(desc) {
}

VulkanBuffer::~VulkanBuffer() {
    if (m_allocator != VK_NULL_HANDLE && m_buffer != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
}

} // namespace Prisma::Graphic::Vulkan
