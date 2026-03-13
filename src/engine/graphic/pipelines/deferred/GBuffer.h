#pragma once

#include "interfaces/IGBuffer.h"
#include "interfaces/IDeviceContext.h"
#include "math/MathTypes.h"
#include <memory>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace PrismaEngine::Graphic {

// 前置声明
class RenderCommandContext;

// G- Buffer纹理格式定义
struct GBufferFormats {
    static const uint32_t POSITION_FORMAT;      // DXGI_FORMAT_R16G16B16A16_FLOAT
    static const uint32_t NORMAL_FORMAT;        // DXGI_FORMAT_R16G16B16A16_FLOAT
    static const uint32_t ALBEDO_FORMAT;         // DXGI_FORMAT_R8G8B8A8_UNORM
    static const uint32_t EMISSIVE_FORMAT;       // DXGI_FORMAT_R11G11B10_FLOAT
    static const uint32_t DEPTH_FORMAT;          // DXGI_FORMAT_D32_FLOAT
};

// G-Buffer数据结构（用于着色器）
struct GBufferData {
    // World space position
    PrismaMath::vec3 position;
    float padding1;

    // World space normal
    PrismaMath::vec3 normal;
    float roughness;

    // Albedo color
    PrismaMath::vec3 albedo;
    float metallic;

    // Emissive color
    PrismaMath::vec3 emissive;
    float ao;

    // Material properties
    uint32_t materialID;
    float padding2[3];
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

    void* GetRenderTargetView(GBufferTarget target) const;
    void* GetShaderResourceView(GBufferTarget target) const;
    void* GetDepthStencilView() const;

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    bool IsInitialized() const override { return m_created; }

    ITextureRenderTarget* GetTarget(GBufferTarget target) override;
    IDepthStencil* GetDepthStencil() override;
    void GetColorTargets(ITextureRenderTarget** targets, uint32_t count) override;
    uint32_t GetColorTargetCount() const override { return 4; }
    TextureFormat GetTargetFormat(GBufferTarget target) const override;

    bool InitializeVulkanResources(uint32_t width, uint32_t height);
    void DestroyVulkanResources();

private:
    // 渲染目标资源
    struct RenderTarget {
        void* resource = nullptr;
        void* renderTargetView = nullptr;
        void* shaderResourceView = nullptr;
    };

    RenderTarget m_renderTargets[static_cast<uint32_t>(GBufferTarget::Count)];

    void* m_depthBuffer = nullptr;
    void* m_depthStencilView = nullptr;
    void* m_depthShaderResourceView = nullptr;

    struct VulkanRenderTarget {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };
    VulkanRenderTarget m_vulkanTargets[4];
    struct VulkanDepthTarget {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };
    VulkanDepthTarget m_vulkanDepth;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // 是否已创建
    bool m_created = false;
};

} // namespace PrismaEngine::Graphic