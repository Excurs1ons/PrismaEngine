#include "VulkanResources.h"
#include <algorithm>

namespace Prisma::Graphic::Vulkan {

VulkanTexture::VulkanTexture(VkDevice device, VmaAllocator allocator, VkImage image, VmaAllocation allocation, VkImageView imageView, const TextureDesc& desc)
    : m_device(device), m_allocator(allocator), m_image(image), m_allocation(allocation), m_imageView(imageView), m_desc(desc) {
    // 构造函数现在接收并存储 VMA 相关对象，以便在析构时能够正确释放资源。
}

VulkanTexture::~VulkanTexture() {
    if (m_allocator) {
        // 在销毁 Texture 资源时，通过 VMA 释放关联的 VkImage 和内存。
        if (m_image != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
            vmaDestroyImage(m_allocator, m_image, m_allocation);
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

// === ImGuiVulkanResourceManager 实现 ===

void ImGuiVulkanResourceManager::Initialize(VkDevice device) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_device != VK_NULL_HANDLE) {
        return; // 已经初始化
    }

    m_device = device;

    // 创建 descriptor set layout（ImGui 需要的单一采样器纹理绑定）
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout);
}

void ImGuiVulkanResourceManager::Shutdown(VkDevice device) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
    m_device = VK_NULL_HANDLE;
}

VkDescriptorSet ImGuiVulkanResourceManager::CreateDescriptorSet(VkDevice device, VkDescriptorPool pool,
                                                                VkImageView imageView, VkSampler sampler) {
    if (pool == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    // 分配 descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    // 更新 descriptor set
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    return descriptorSet;
}

// === VulkanTexture ImGui 支持 ===

VkDescriptorSet VulkanTexture::GetOrCreateImGuiDescriptorSet(VkDescriptorPool pool, VkSampler sampler) {
    std::lock_guard<std::mutex> lock(m_descriptorSetMutex);

    // 如果已经有缓存的 descriptor set，直接返回
    if (m_imguiDescriptorSet != VK_NULL_HANDLE) {
        return m_imguiDescriptorSet;
    }

    // 确保资源管理器已初始化
    auto& resourceManager = ImGuiVulkanResourceManager::Get();
    if (resourceManager.GetDescriptorSetLayout() == VK_NULL_HANDLE) {
        // 需要外部调用 Initialize，这里暂时返回 null
        return VK_NULL_HANDLE;
    }

    // 创建新的 descriptor set
    m_imguiDescriptorSet = resourceManager.CreateDescriptorSet(
        m_device,
        pool,
        m_imageView,
        sampler
    );

    return m_imguiDescriptorSet;
}

} // namespace Prisma::Graphic::Vulkan
