#include "VulkanSampler.h"

namespace Prisma::Graphic::Vulkan {

VulkanSampler::VulkanSampler(VkDevice device, const SamplerDesc& desc)
    : m_device(device), m_desc(desc) {
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    
    // Basic mapping for now, should be expanded for all filter types
    if (desc.filter == TextureFilter::Point) {
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    } else {
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }

    samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(m_desc.addressU);
    samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(m_desc.addressV);
    samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(m_desc.addressW);
    samplerInfo.mipLodBias = m_desc.mipLODBias;
    samplerInfo.anisotropyEnable = m_desc.maxAnisotropy > 1;
    samplerInfo.maxAnisotropy = static_cast<float>(m_desc.maxAnisotropy);
    
    samplerInfo.compareEnable = (m_desc.comparisonFunc != TextureComparisonFunc::Always);
    samplerInfo.compareOp = static_cast<VkCompareOp>(m_desc.comparisonFunc);
    
    samplerInfo.minLod = m_desc.minLOD;
    samplerInfo.maxLod = m_desc.maxLOD;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);
}

VulkanSampler::~VulkanSampler() {
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
}

} // namespace Prisma::Graphic::Vulkan
