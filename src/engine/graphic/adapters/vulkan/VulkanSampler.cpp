#include "VulkanSampler.h"

namespace PrismaEngine::Graphic::Vulkan {

VulkanSampler::VulkanSampler(VkDevice device, const SamplerDesc& desc)
    : m_device(device), m_desc(desc) {
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = static_cast<VkFilter>(m_desc.filter);
    samplerInfo.minFilter = static_cast<VkFilter>(m_desc.filter);
    samplerInfo.mipmapMode = static_cast<VkSamplerMipmapMode>(m_desc.mipFilter);
    samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(m_desc.addressU);
    samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(m_desc.addressV);
    samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(m_desc.addressW);
    samplerInfo.mipLodBias = m_desc.mipLODBias;
    samplerInfo.anisotropyEnable = m_desc.maxAnisotropy > 1;
    samplerInfo.maxAnisotropy = static_cast<float>(m_desc.maxAnisotropy);
    samplerInfo.compareEnable = m_desc.comparisonFunc != TextureComparisonFunc::None;
    samplerInfo.compareOp = static_cast<VkCompareOp>(m_desc.comparisonFunc);
    samplerInfo.minLod = m_desc.minLOD;
    samplerInfo.maxLod = m_desc.maxLOD;
    samplerInfo.borderColor = static_cast<VkBorderColor>(m_desc.borderColorMode);
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);
}

VulkanSampler::~VulkanSampler() {
    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
}

} // namespace PrismaEngine::Graphic::Vulkan
