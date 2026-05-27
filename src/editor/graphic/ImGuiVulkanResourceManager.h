#pragma once

#include "../Export.h"
#include <vulkan/vulkan.h>
#include <mutex>
#include <unordered_map>

namespace Prisma::Graphic::Vulkan {
    class VulkanTexture;
}

namespace Prisma {

/**
 * ImGui Vulkan 资源管理器
 * 
 * [改动] 新增此类。
 * 
 * 目的：
 *   负责管理引擎纹理（VulkanTexture）到 ImGui 描述符集（VkDescriptorSet）的映射。
 *   将 UI 框架相关的资源管理从引擎核心剥离，归属于编辑器模块。
 * 
 * 过程：
 *   1. 维护一个内部缓存 map (VulkanTexture* -> VkDescriptorSet)。
 *   2. 提供 GetDescriptorSet 接口，自动处理描述符集的分配与更新。
 *   3. 生命周期与 Editor 实例绑定，确保在 Vulkan 设备销毁前安全释放。
 */
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
