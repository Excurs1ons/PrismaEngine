#pragma once

#include "interfaces/IGBuffer.h"
#include "interfaces/IDeviceContext.h"
#include "math/MathTypes.h"
#include <memory>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Prisma::Graphic {

// 前置声明
class RenderCommandContext;

// G- Buffer纹理格式定义
struct GBufferFormats {
    static const uint32_t POSITION_FORMAT;
    static const uint32_t NORMAL_FORMAT;
    static const uint32_t ALBEDO_FORMAT;
    static const uint32_t EMISSIVE_FORMAT;
    static const uint32_t DEPTH_FORMAT;
};

// G-Buffer数据结构（用于着色器）
struct GBufferData {
    PrismaMath::vec3 position;
    float roughness;
    PrismaMath::vec3 normal;
    float metallic;
    PrismaMath::vec3 albedo;
    float ao;
    PrismaMath::vec3 emissive;
    uint32_t materialID;
};

// G-Buffer资源管理器
class GBuffer : public IGBuffer
{
public:
    GBuffer();
    ~GBuffer() override;

    bool Initialize(uint32_t width, uint32_t height) override;
    bool Resize(uint32_t width, uint32_t height) override;

    void SetAsRenderTarget(IDeviceContext* deviceContext) override;
    void BindAsShaderResources(IDeviceContext* deviceContext, uint32_t startSlot = 0) override;
    void UnbindShaderResources(IDeviceContext* deviceContext, uint32_t startSlot = 0, uint32_t count = 4) override;

    void Clear(IDeviceContext* deviceContext, const float color[4]) override;
    void ClearDepth(IDeviceContext* deviceContext, float depth = 1.0f) override;

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    bool IsInitialized() const override { return m_created; }

    ITextureRenderTarget* GetTarget(GBufferTarget target) override { return nullptr; }
    IDepthStencil* GetDepthStencil() override { return nullptr; }
    void GetColorTargets(ITextureRenderTarget** targets, uint32_t count) override {}
    uint32_t GetColorTargetCount() const override { return 4; }
    TextureFormat GetTargetFormat(GBufferTarget target) const override { return TextureFormat::Unknown; }

    bool InitializeVulkanResources(uint32_t width, uint32_t height);
    void DestroyVulkanResources();

private:
    struct VulkanResource {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    VulkanResource m_renderTargets[static_cast<uint32_t>(GBufferTarget::Count)];
    VulkanResource m_depthBuffer;

    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_created = false;
};

} // namespace Prisma::Graphic
