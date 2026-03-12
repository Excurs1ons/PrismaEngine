#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include "CubemapTextureAsset.h"
#include <stb_image.h>
#include <fstream>

CubemapTextureAsset::CubemapTextureAsset() : TextureAsset() {}

CubemapTextureAsset::~CubemapTextureAsset() {
}

std::shared_ptr<CubemapTextureAsset> CubemapTextureAsset::loadCubemap(
        const std::vector<std::string>& facePaths,
        VulkanContext* vulkanContext) {
    return loadFromFiles(facePaths, vulkanContext) 
        ? std::shared_ptr<CubemapTextureAsset>(new CubemapTextureAsset())
        : nullptr;
}

bool CubemapTextureAsset::loadFromFiles(const std::vector<std::string>& facePaths, VulkanContext* context) {
    if (facePaths.size() != 6) {
        return false;
    }

    int width, height, channels;
    std::vector<uint8_t> pixels[6];
    
    for (size_t i = 0; i < 6; ++i) {
        stbi_set_flip_vertically_on_load(false);
        int w, h, c;
        uint8_t* data = stbi_load(facePaths[i].c_str(), &w, &h, &c, STBI_rgb_alpha);
        if (!data) {
            for (size_t j = 0; j < i; ++j) {
                stbi_image_free(pixels[j].data());
            }
            return false;
        }
        width = w;
        height = h;
        pixels[i].assign(data, data + w * h * 4);
        stbi_image_free(data);
    }

    for (size_t i = 1; i < 6; ++i) {
        if (pixels[i].size() != pixels[0].size()) {
            return false;
        }
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(context->device, &imageInfo, nullptr, &image_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context->device, image_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context->findMemoryType(memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(context->device, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(context->device, image_, memory_, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(context->device, &viewInfo, nullptr, &imageView_) != VK_SUCCESS) {
        return false;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
        return false;
    }

    m_isLoaded = true;
    return true;
}
#endif // PRISMA_ENABLE_RENDER_VULKAN
