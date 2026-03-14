#include "VulkanResources.h"
#include <algorithm>

namespace Prisma::Graphic::Vulkan {

VulkanTexture::VulkanTexture(VkImage image, VkImageView imageView, const TextureDesc& desc)
    : m_image(image), m_imageView(imageView), m_desc(desc) {
}

VulkanTexture::~VulkanTexture() {
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

VulkanBuffer::VulkanBuffer(VkBuffer buffer, const BufferDesc& desc)
    : m_buffer(buffer), m_desc(desc) {
}

VulkanBuffer::~VulkanBuffer() {
}

} // namespace Prisma::Graphic::Vulkan
