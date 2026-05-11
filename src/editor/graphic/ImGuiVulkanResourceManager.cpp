#include "ImGuiVulkanResourceManager.h"
#include "../../engine/graphic/adapters/vulkan/VulkanResources.h"
#include <stdexcept>

namespace Prisma {

ImGuiVulkanResourceManager::ImGuiVulkanResourceManager() = default;

ImGuiVulkanResourceManager::~ImGuiVulkanResourceManager() {
    Shutdown();
}

void ImGuiVulkanResourceManager::Initialize(VkDevice device) {
    if (m_device != VK_NULL_HANDLE) return;
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

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ImGui descriptor set layout");
    }
}

void ImGuiVulkanResourceManager::Shutdown() {
    if (m_device == VK_NULL_HANDLE) return;

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    m_textureDescriptorSets.clear();
    m_device = VK_NULL_HANDLE;
}

VkDescriptorSet ImGuiVulkanResourceManager::GetDescriptorSet(Prisma::Graphic::Vulkan::VulkanTexture* texture, VkDescriptorPool pool, VkSampler sampler) {
    if (!texture || m_device == VK_NULL_HANDLE || pool == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. 查找缓存
    auto it = m_textureDescriptorSets.find(texture);
    if (it != m_textureDescriptorSets.end()) {
        return it->second;
    }

    // 2. 创建新的 descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    // 3. 更新 descriptor set
    VkDescriptorImageInfo imageInfo{};
    // -----------------------------------------------------------------------
    // [改动] imageInfo.imageLayout
    //
    // 目的：
    //   解决布局不一致导致的验证层报错。
    //
    // 过程：
    //   必须使用 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL。
    //   原因：ViewportRenderPass 的颜色附件 finalLayout 已声明为此布局。
    //   Vulkan 要求描述符中声明的布局与采样时的真实布局严格匹配。
    // -----------------------------------------------------------------------
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->GetVkImageView();
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

    // 4. 存入缓存
    m_textureDescriptorSets[texture] = descriptorSet;

    return descriptorSet;
}

void ImGuiVulkanResourceManager::ReleaseTextureResources(Prisma::Graphic::Vulkan::VulkanTexture* texture) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_textureDescriptorSets.erase(texture);
}

} // namespace Prisma
