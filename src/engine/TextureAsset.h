#pragma once

#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <android/asset_manager.h>

class VulkanContext; // 前向声明

class TextureAsset {
public:
    // 使用 Vulkan 上下文创建
    static std::shared_ptr<TextureAsset> loadAsset(
            AAssetManager* assetManager,
            const std::string& assetPath,
            VulkanContext* vulkanContext = nullptr);

    // 创建白色 1x1 fallback 纹理（类似 Unity 的默认白色纹理）
    // 当材质没有纹理时使用此纹理作为 fallback
    static std::shared_ptr<TextureAsset> createWhiteFallback(VulkanContext* vulkanContext);

    // 获取或创建全局白色 fallback 纹理（单例模式）
    // 确保整个引擎只创建一个白色纹理，节省资源
    static std::shared_ptr<TextureAsset> getOrCreateWhiteFallback(VulkanContext* vulkanContext);

    virtual ~TextureAsset();

    // 获取 Vulkan 资源句柄
    VkImageView getImageView() const { return imageView_; }
    VkSampler getSampler() const { return sampler_; }
    glm::uvec2 getSize() const { return size_; }
    uint32_t getWidth() const { return size_.x; }
    uint32_t getHeight() const { return size_.y; }
    VkFormat getFormat() const { return format_; }

    // 获取 OpenGL 资源句柄
    GLuint getTextureID() const { return textureID_; }

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

    // OpenGL 资源
    GLuint textureID_ = 0;
};
#endif