#pragma once

#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class VulkanContext;

class TextureAsset {
public:
    static std::shared_ptr<TextureAsset> loadAsset(
            const std::string& assetPath,
            VulkanContext* vulkanContext = nullptr);

    // 创建白色 1x1 fallback 纹理（类似 Unity 的默认白色纹理）
    static std::shared_ptr<TextureAsset> getWhiteFallback(VulkanContext* vulkanContext);

    // 获取白色 fallback
    static std::shared_ptr<TextureAsset> White() { return getWhiteFallback(nullptr); }

    // 构造函数
    TextureAsset();
    ~TextureAsset();

    // 加载和卸载
    bool load(const std::string& path);
    void Unload();

    // 属性
    VkImageView getImageView() const { return imageView_; }
    VkSampler getSampler() const { return sampler_; }
    glm::uvec2 getSize() const { return size_; }
    uint32_t getWidth() const { return size_.x; }
    uint32_t getHeight() const { return size_.y; }
    VkFormat getFormat() const { return format_; }

    void Unload();
protected:

    TextureAsset(VulkanContext* context);

    VulkanContext* context_;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory_ = VK_NULL_HANDLE;
    VkImageView imageView_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    glm::uvec2 size_ = {0, 0};
    VkFormat format_ = VK_FORMAT_R8G8B8A8_SRGB;
    uint32_t mipLevels_ = 1;
};
#endif
