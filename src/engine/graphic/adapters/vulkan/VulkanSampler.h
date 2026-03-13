#pragma once

#include "interfaces/ISampler.h"
#include "RenderDesc.h"
#include <vulkan/vulkan.h>

namespace PrismaEngine::Graphic::Vulkan {

class VulkanSampler : public ISampler {
public:
    VulkanSampler(VkDevice device, const SamplerDesc& desc);
    ~VulkanSampler() override;

    ResourceType GetType() const override { return ResourceType::Sampler; }

    TextureFilter GetFilter() const override { return m_desc.filter; }
    TextureAddressMode GetAddressU() const override { return m_desc.addressU; }
    TextureAddressMode GetAddressV() const override { return m_desc.addressV; }
    TextureAddressMode GetAddressW() const override { return m_desc.addressW; }
    float GetMipLODBias() const override { return m_desc.mipLODBias; }
    uint32_t GetMaxAnisotropy() const override { return m_desc.maxAnisotropy; }
    TextureComparisonFunc GetComparisonFunc() const override { return m_desc.comparisonFunc; }
    void GetBorderColor(float& r, float& g, float& b, float& a) const override {
        r = m_desc.borderColor[0];
        g = m_desc.borderColor[1];
        b = m_desc.borderColor[2];
        a = m_desc.borderColor[3];
    }
    float GetMinLOD() const override { return m_desc.minLOD; }
    float GetMaxLOD() const override { return m_desc.maxLOD; }
    uint64_t GetHandle() const override { return reinterpret_cast<uint64_t>(m_sampler); }

    VkSampler GetVkSampler() const { return m_sampler; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    SamplerDesc m_desc;
};

} // namespace PrismaEngine::Graphic::Vulkan
