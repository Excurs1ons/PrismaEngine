#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include "TextureAsset.h"
#include "Logger.h"
#include <array>
#include <cmath>
#include <cstring>
#include <mutex>
#include <stb_image.h>
#include <vector>

// 全局白色 fallback 纹理（单例）
static std::shared_ptr<TextureAsset> g_whiteFallbackTexture = nullptr;
static std::mutex g_whiteFallbackMutex;

// 辅助函数：创建 Vulkan 图像
static void createVulkanImage(
        VulkanContext* context,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context->device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("无法创建图像！");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context->device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context->findMemoryType(
            memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("无法分配图像内存！");
    }

    vkBindImageMemory(context->device, image, imageMemory, 0);
}

// 辅助函数：创建图像视图
static VkImageView createImageView(
        VulkanContext* context,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        uint32_t mipLevels) {

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(context->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("无法创建纹理图像视图！");
    }

    return imageView;
}

// 辅助函数：创建采样器
static VkSampler createTextureSampler(VulkanContext* context, uint32_t mipLevels) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = context->properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    VkSampler sampler;
    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("无法创建纹理采样器！");
    }
    return sampler;
}

// 创建白色 1x1 fallback 纹理（类似 Unity 的默认白色纹理）
std::shared_ptr<TextureAsset> TextureAsset::createWhiteFallback(VulkanContext* vulkanContext) {
    if (!vulkanContext) {
        return nullptr;
    }

    auto texture = std::shared_ptr<TextureAsset>(new TextureAsset(vulkanContext));

    // 1x1 白色纹理数据 (RGBA: 255, 255, 255, 255)
    const uint32_t width = 1;
    const uint32_t height = 1;
    const uint32_t mipLevels = 1;
    const std::array<uint8_t, 4> whitePixel = {255, 255, 255, 255};

    texture->size_ = glm::uvec2(width, height);
    texture->mipLevels_ = mipLevels;

    VkDeviceSize imageSize = sizeof(whitePixel);

    // 创建暂存缓冲区
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vulkanContext->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);

    // 复制白色像素数据
    void* data;
    vkMapMemory(vulkanContext->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, whitePixel.data(), imageSize);
    vkUnmapMemory(vulkanContext->device, stagingBufferMemory);

    // 创建 Vulkan 图像
    createVulkanImage(
            vulkanContext,
            width,
            height,
            mipLevels,
            texture->format_,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            texture->image_,
            texture->imageMemory_);

    // 转换图像布局并复制数据
    vulkanContext->transitionImageLayout(
            texture->image_,
            texture->format_,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            mipLevels);

    vulkanContext->copyBufferToImage(
            stagingBuffer,
            texture->image_,
            width,
            height);

    // 转换到着色器只读布局
    vulkanContext->transitionImageLayout(
            texture->image_,
            texture->format_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            mipLevels);

    // 清理暂存缓冲区
    vkDestroyBuffer(vulkanContext->device, stagingBuffer, nullptr);
    vkFreeMemory(vulkanContext->device, stagingBufferMemory, nullptr);

    // 创建图像视图和采样器
    texture->imageView_ = createImageView(
            vulkanContext,
            texture->image_,
            texture->format_,
            VK_IMAGE_ASPECT_COLOR_BIT,
            mipLevels);

    texture->sampler_ = createTextureSampler(vulkanContext, mipLevels);

    LOG_INFO("TextureAsset", "已创建白色回退纹理 (1x1)");

    return texture;
}

// 获取或创建全局白色 fallback 纹理（单例模式）
std::shared_ptr<TextureAsset> TextureAsset::getWhiteFallback(VulkanContext* vulkanContext) {
    if (!vulkanContext) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(g_whiteFallbackMutex);

    if (g_whiteFallbackTexture) {
        return g_whiteFallbackTexture;
    }

    g_whiteFallbackTexture = createWhiteFallback(vulkanContext);
    return g_whiteFallbackTexture;
}

// 主要加载函数
std::shared_ptr<TextureAsset> TextureAsset::loadAsset(
        const std::string& assetPath,
        VulkanContext* vulkanContext) {
    if (!vulkanContext) {
        LOG_WARNING("TextureAsset", "无法在没有 Vulkan 上下文的情况下加载纹理: {}", assetPath);
        return nullptr;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);
    stbi_uc* imageData = stbi_load(assetPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!imageData) {
        LOG_WARNING("TextureAsset", "加载纹理 {} 失败，正在回退到白色纹理", assetPath);
        return getWhiteFallback(vulkanContext);
    }

    auto texture = std::shared_ptr<TextureAsset>(new TextureAsset(vulkanContext));
    texture->size_ = glm::uvec2(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    texture->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

    vulkanContext->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);

    void* mappedData = nullptr;
    vkMapMemory(vulkanContext->device, stagingBufferMemory, 0, imageSize, 0, &mappedData);
    std::memcpy(mappedData, imageData, static_cast<size_t>(imageSize));
    vkUnmapMemory(vulkanContext->device, stagingBufferMemory);
    stbi_image_free(imageData);

    createVulkanImage(
            vulkanContext,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            texture->mipLevels_,
            texture->format_,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            texture->image_,
            texture->imageMemory_);

    vulkanContext->transitionImageLayout(
            texture->image_,
            texture->format_,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            texture->mipLevels_);

    vulkanContext->copyBufferToImage(
            stagingBuffer,
            texture->image_,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height));

    vulkanContext->generateMipmaps(
            texture->image_,
            texture->format_,
            width,
            height,
            texture->mipLevels_);

    vkDestroyBuffer(vulkanContext->device, stagingBuffer, nullptr);
    vkFreeMemory(vulkanContext->device, stagingBufferMemory, nullptr);

    texture->imageView_ = createImageView(
            vulkanContext,
            texture->image_,
            texture->format_,
            VK_IMAGE_ASPECT_COLOR_BIT,
            texture->mipLevels_);
    texture->sampler_ = createTextureSampler(vulkanContext, texture->mipLevels_);

    return texture;
}

TextureAsset::TextureAsset() : context_(nullptr) {}

TextureAsset::TextureAsset(VulkanContext* context) : context_(context) {}

TextureAsset::~TextureAsset() {
    Unload();
}

void TextureAsset::Unload() {
    if (context_) {
        if (sampler_ != VK_NULL_HANDLE) {
            vkDestroySampler(context_->device, sampler_, nullptr);
            sampler_ = VK_NULL_HANDLE;
        }
        if (imageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(context_->device, imageView_, nullptr);
            imageView_ = VK_NULL_HANDLE;
        }
        if (image_ != VK_NULL_HANDLE) {
            vkDestroyImage(context_->device, image_, nullptr);
            image_ = VK_NULL_HANDLE;
        }
        if (imageMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(context_->device, imageMemory_, nullptr);
            imageMemory_ = VK_NULL_HANDLE;
        }
    }
    size_ = {0, 0};
    mipLevels_ = 1;
}
#endif
