#include "VulkanResources.h"
#include "VulkanCommandBuffer.h"
#include <algorithm>
#include "Export.h"
#include "logger/Logger.h"

namespace Prisma::Graphic::Vulkan {

// -----------------------------------------------------------------------
// [改动] VulkanTexture 构造函数
//
// 目的：
//   接收并存储纹理的原始 VkFormat。
//
// 过程：
//   初始化列表中新增 m_vkFormat(vkFormat)。
// -----------------------------------------------------------------------
VulkanTexture::VulkanTexture(VkDevice device, VmaAllocator allocator, VkImage image, VmaAllocation allocation, VkImageView imageView, VkFormat vkFormat, const TextureDesc& desc, VkImageView defaultUAV)
    : m_device(device), m_allocator(allocator), m_image(image), m_allocation(allocation), m_imageView(imageView), m_defaultUAV(defaultUAV), m_vkFormat(vkFormat), m_desc(desc) {
    // 构造函数现在接收并存储 VMA 相关对象，以便在析构时能够正确释放资源。
}

VulkanTexture::~VulkanTexture() {
    if (m_device != VK_NULL_HANDLE) {
        // [修复] 必须先释放离屏缓存中的 RenderPass/Framebuffer
        // 原因：缓存的 Framebuffer 引用了此 ImageView，必须在 ImageView
        //       销毁前从缓存中移除并释放，否则驱动会因野句柄而崩溃。
        VulkanCommandBuffer::ReleaseOffscreenResources(m_imageView);

        // [修复] 必须显式销毁 ImageView
        // 原因：ImageView 是由 vkCreateImageView 创建的独立句柄，
        //       VMA 只管理 Image 和 Memory，不会自动销毁 View。
        //       漏掉此步骤会导致 vkDestroyDevice 时报资源泄露错误。
        if (m_imageView != VK_NULL_HANDLE) {
            // 如果 UAV 视图与主视图是同一句柄，避免后续重复销毁
            if (m_defaultUAV == m_imageView) {
                m_defaultUAV = VK_NULL_HANDLE;
            }
            vkDestroyImageView(m_device, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }

        // 销毁独立的 UAV ImageView
        if (m_defaultUAV != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, m_defaultUAV, nullptr);
            m_defaultUAV = VK_NULL_HANDLE;
        }
    }

    if (m_allocator) {
        // 在销毁 Texture 资源时，通过 VMA 释放关联的 VkImage 和内存。
        if (m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
            vmaDestroyImage(m_allocator, m_image, m_allocation);
        }
    }
}

// -----------------------------------------------------------------------
// [改动] SetDebugName
//
// 目的：
//   利用 Vulkan 调试扩展为资源命名，辅助定位 Validation Layer 报错。
//
// 过程：
//   1. 动态加载 vkSetDebugUtilsObjectNameEXT。
//   2. 填充 VkDebugUtilsObjectNameInfoEXT 结构体。
//   3. 分别为底层 VkImage 和绑定的 VkImageView 设置名称。
// -----------------------------------------------------------------------
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

        if (m_defaultUAV != VK_NULL_HANDLE && m_defaultUAV != m_imageView) {
            nameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
            nameInfo.objectHandle = (uint64_t)m_defaultUAV;
            nameInfo.pObjectName = (name + "_UAV").c_str();
            func(m_device, &nameInfo);
        }
    }
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
    // 构造函数现在接收并存储 VMA 相关对象，以便在析构时能够正确释放资源。
}

VulkanBuffer::~VulkanBuffer() {
    // 在销毁 Buffer 资源时，通过 VMA 释放关联的 VkBuffer 和内存。
    if (m_allocator != VK_NULL_HANDLE && m_buffer != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
}

void VulkanDescriptorSet::BindTexture(uint32_t binding, ITexture* texture, ISampler* sampler) {
    auto vkTex = dynamic_cast<VulkanTexture*>(texture);
    if (!vkTex) return;

    // [修复] 检查采样器是否有效
    // 原因：Vulkan 规范要求 COMBINED_IMAGE_SAMPLER 必须包含有效的采样器。
    //       若 sampler 为 nullptr，将导致驱动在更新描述符集时崩溃。
    if (!sampler) {
        LOG_ERROR("Vulkan", "BindTexture called with null sampler at binding {0}. This will likely cause a crash in vkUpdateDescriptorSets.", binding);
    }

    WriteInfo write{};
    write.binding = binding;
    write.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    write.imageInfo.imageView = vkTex->GetVkImageView();
    if (sampler) {
        write.imageInfo.sampler = (VkSampler)sampler->GetHandle();
    } else {
        write.imageInfo.sampler = VK_NULL_HANDLE;
    }
    write.isImage = true;
    m_writes.push_back(write);
}

void VulkanDescriptorSet::BindBuffer(uint32_t binding, IBuffer* buffer, uint32_t offset, uint32_t size,
                                     DescriptorType type) {
    auto vkBuf = dynamic_cast<VulkanBuffer*>(buffer);
    if (!vkBuf) return;

    VkDescriptorType vkType;
    switch (type) {
        case DescriptorType::UniformBuffer:
            vkType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case DescriptorType::StorageBuffer:
            vkType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        default:
            vkType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
    }

    WriteInfo write{};
    write.binding = binding;
    write.type = vkType;
    write.bufferInfo.buffer = vkBuf->GetVkBuffer();
    write.bufferInfo.offset = offset;
    write.bufferInfo.range = size;
    write.isImage = false;
    m_writes.push_back(write);
}

void VulkanDescriptorSet::BindStorageImage(uint32_t binding, ITexture* texture) {
    auto vkTex = dynamic_cast<VulkanTexture*>(texture);
    if (!vkTex) return;

    WriteInfo write{};
    write.binding = binding;
    write.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    write.imageInfo.imageView = vkTex->GetUAVImageView();
    write.imageInfo.sampler = VK_NULL_HANDLE;
    write.isImage = true;
    write.isStorageImage = true;
    m_writes.push_back(write);
}

void VulkanDescriptorSet::BindAccelerationStructure(uint32_t binding, void* accelerationStructure) {
    WriteInfo write{};
    write.binding = binding;
    write.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.accelerationStructure = static_cast<VkAccelerationStructureKHR>(reinterpret_cast<uintptr_t>(accelerationStructure));
    write.isAccelerationStructure = true;
    m_writes.push_back(write);
}

void VulkanDescriptorSet::Update() {
    if (m_writes.empty()) return;

    // [修复] 检查设备是否有效
    if (m_device == VK_NULL_HANDLE) {
        LOG_ERROR("Vulkan", "VulkanDescriptorSet::Update called with null device.");
        return;
    }

    std::vector<VkWriteDescriptorSet> vkWrites;
    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> asWrites;
    asWrites.reserve(m_writes.size());

    for (auto& write : m_writes) {
        VkWriteDescriptorSet vkWrite{};
        vkWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkWrite.dstSet = m_set;
        vkWrite.dstBinding = write.binding;
        vkWrite.descriptorCount = 1;
        vkWrite.descriptorType = write.type;

        if (write.isAccelerationStructure) {
            VkWriteDescriptorSetAccelerationStructureKHR asWrite{};
            asWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            asWrite.accelerationStructureCount = 1;
            asWrite.pAccelerationStructures = &write.accelerationStructure;
            asWrites.push_back(asWrite);
            vkWrite.pNext = &asWrites.back();
        } else if (write.isImage) {
            vkWrite.pImageInfo = &write.imageInfo;
        } else {
            vkWrite.pBufferInfo = &write.bufferInfo;
        }
        vkWrites.push_back(vkWrite);
    }

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(vkWrites.size()), vkWrites.data(), 0, nullptr);
    m_writes.clear();
}

} // namespace Prisma::Graphic::Vulkan
