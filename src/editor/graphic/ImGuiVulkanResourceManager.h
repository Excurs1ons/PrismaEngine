#pragma once

#include "../Export.h"
#include <vulkan/vulkan.h>
#include <mutex>
#include <unordered_map>

namespace Prisma::Graphic::Vulkan {
    class VulkanTexture;
}

namespace Prisma {

class EDITOR_API ImGuiVulkanResourceManager {
public:
    ImGuiVulkanResourceManager();
    ~ImGuiVulkanResourceManager();

    void Initialize(VkDevice device);
    void Shutdown();

    // 为 VulkanTexture 获取或创建 ImGui DescriptorSet
    VkDescriptorSet GetDescriptorSet(Prisma::Graphic::Vulkan::VulkanTexture* texture, VkDescriptorPool pool, VkSampler sampler);

    // 清理特定纹理的资源
    void ReleaseTextureResources(Prisma::Graphic::Vulkan::VulkanTexture* texture);

    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    
    // 缓存：纹理 -> DescriptorSet
    std::unordered_map<Prisma::Graphic::Vulkan::VulkanTexture*, VkDescriptorSet> m_textureDescriptorSets;
    std::mutex m_mutex;
};

} // namespace Prisma
