#include "GBuffer.h"
#include "RenderCommandContext.h"
#include "Logger.h"

namespace PrismaEngine::Graphic {

const uint32_t GBufferFormats::POSITION_FORMAT = 10;
const uint32_t GBufferFormats::NORMAL_FORMAT = 10;
const uint32_t GBufferFormats::ALBEDO_FORMAT = 28;
const uint32_t GBufferFormats::EMISSIVE_FORMAT = 26;
const uint32_t GBufferFormats::DEPTH_FORMAT = 40;

GBuffer::GBuffer() {
    LOG_DEBUG("GBuffer", "GBuffer构造函数被调用");
}

GBuffer::~GBuffer() {
    LOG_DEBUG("GBuffer", "GBuffer析构函数被调用");
    Destroy();
}

bool GBuffer::Initialize(uint32_t width, uint32_t height) {
    LOG_INFO("GBuffer", "创建G-Buffer: {0}x{1}", width, height);

    if (m_created) {
        LOG_WARNING("GBuffer", "G-Buffer已经创建，先销毁旧的");
        Destroy();
    }

    m_width = width;
    m_height = height;

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    if (!InitializeVulkanResources(width, height)) {
        LOG_ERROR("GBuffer", "Vulkan资源创建失败");
        return false;
    }
#endif

    m_created = true;
    LOG_INFO("GBuffer", "G-Buffer创建成功");
    return true;
}

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
bool GBuffer::InitializeVulkanResources(uint32_t width, uint32_t height) {
    VkFormat formats[4] = {
        VK_FORMAT_R16G16B16A16_SFLOAT,  // Position
        VK_FORMAT_R16G16B16A16_SFLOAT,  // Normal
        VK_FORMAT_R8G8B8A8_UNORM,       // Albedo
        VK_FORMAT_R11G11B10_SFLOAT      // Emissive
    };

    for (uint32_t i = 0; i < 4; ++i) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = formats[i];
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkResult result = vmaCreateImage(m_vmaAllocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
        if (result != VK_SUCCESS) {
            LOG_ERROR("GBuffer", "Failed to create GBuffer target {}", i);
            return false;
        }

        m_vulkanTargets[i].image = image;
        m_vulkanTargets[i].allocation = allocation;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = formats[i];
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        result = vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &imageView);
        if (result != VK_SUCCESS) {
            LOG_ERROR("GBuffer", "Failed to create GBuffer view {}", i);
            vmaDestroyImage(m_vmaAllocator, image, allocation);
            return false;
        }

        m_vulkanTargets[i].imageView = imageView;
    }

    VkImageCreateInfo depthImageInfo{};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.extent.width = width;
    depthImageInfo.extent.height = height;
    depthImageInfo.extent.depth = 1;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.format = VK_FORMAT_D32_SFLOAT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo depthAllocInfo{};
    depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage depthImage = VK_NULL_HANDLE;
    VmaAllocation depthAllocation = VK_NULL_HANDLE;
    VkResult result = vmaCreateImage(m_vmaAllocator, &depthImageInfo, &depthAllocInfo, &depthImage, &depthAllocation, nullptr);
    if (result != VK_SUCCESS) {
        LOG_ERROR("GBuffer", "Failed to create depth buffer");
        return false;
    }

    m_vulkanDepth.image = depthImage;
    m_vulkanDepth.allocation = depthAllocation;

    VkImageViewCreateInfo depthViewInfo{};
    depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthViewInfo.image = depthImage;
    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format = VK_FORMAT_D32_SFLOAT;
    depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewInfo.subresourceRange.baseMipLevel = 0;
    depthViewInfo.subresourceRange.levelCount = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(m_vkDevice, &depthViewInfo, nullptr, &m_vulkanDepth.imageView);
    if (result != VK_SUCCESS) {
        LOG_ERROR("GBuffer", "Failed to create depth view");
        vmaDestroyImage(m_vmaAllocator, depthImage, depthAllocation);
        return false;
    }

    LOG_INFO("GBuffer", "Vulkan G-Buffer resources created successfully");
    return true;
}

void GBuffer::DestroyVulkanResources() {
    for (uint32_t i = 0; i < 4; ++i) {
        if (m_vulkanTargets[i].imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_vkDevice, m_vulkanTargets[i].imageView, nullptr);
            m_vulkanTargets[i].imageView = VK_NULL_HANDLE;
        }
        if (m_vulkanTargets[i].image != VK_NULL_HANDLE) {
            vmaDestroyImage(m_vmaAllocator, m_vulkanTargets[i].image, m_vulkanTargets[i].allocation);
            m_vulkanTargets[i].image = VK_NULL_HANDLE;
            m_vulkanTargets[i].allocation = VK_NULL_HANDLE;
        }
    }

    if (m_vulkanDepth.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_vkDevice, m_vulkanDepth.imageView, nullptr);
        m_vulkanDepth.imageView = VK_NULL_HANDLE;
    }
    if (m_vulkanDepth.image != VK_NULL_HANDLE) {
        vmaDestroyImage(m_vmaAllocator, m_vulkanDepth.image, m_vulkanDepth.allocation);
        m_vulkanDepth.image = VK_NULL_HANDLE;
        m_vulkanDepth.allocation = VK_NULL_HANDLE;
    }
}
#endif

void GBuffer::Destroy() {
    if (!m_created) {
        return;
    }

    LOG_DEBUG("GBuffer", "销毁G-Buffer");

#if defined(PRISMA_ENABLE_RENDER_VULKAN)
    DestroyVulkanResources();
#endif

    for (uint32_t i = 0; i < static_cast<uint32_t>(GBufferTarget::Count); ++i) {
        if (m_renderTargets[i].resource) {
            m_renderTargets[i].resource = nullptr;
        }
        if (m_renderTargets[i].renderTargetView) {
            m_renderTargets[i].renderTargetView = nullptr;
        }
        if (m_renderTargets[i].shaderResourceView) {
            m_renderTargets[i].shaderResourceView = nullptr;
        }
    }

    if (m_depthBuffer) {
        m_depthBuffer = nullptr;
    }
    if (m_depthStencilView) {
        m_depthStencilView = nullptr;
    }
    if (m_depthShaderResourceView) {
        m_depthShaderResourceView = nullptr;
    }

    m_created = false;
    LOG_DEBUG("GBuffer", "G-Buffer销毁完成");
}

bool GBuffer::Resize(uint32_t width, uint32_t height) {
    if (m_width == width && m_height == height) {
        return true;
    }

    LOG_DEBUG("GBuffer", "调整G-Buffer尺寸: {0}x{1} -> {2}x{3}",
              m_width, m_height, width, height);

    return Create(width, height);
}

void GBuffer::SetAsRenderTarget(IDeviceContext* deviceContext) {
    if (!deviceContext || !m_created) {
        LOG_ERROR("GBuffer", "无效的上下文或G-Buffer未创建");
        return;
    }

    LOG_DEBUG("GBuffer", "设置G-Buffer为渲染目标");
}

void GBuffer::BindAsShaderResources(IDeviceContext* deviceContext, uint32_t startSlot) {
    if (!deviceContext || !m_created) {
        LOG_ERROR("GBuffer", "无效的上下文或G-Buffer未创建");
        return;
    }

    LOG_DEBUG("GBuffer", "设置G-Buffer为着色器资源");
}

void GBuffer::UnbindShaderResources(IDeviceContext* deviceContext, uint32_t startSlot, uint32_t count) {
    (void)deviceContext;
    (void)startSlot;
    (void)count;
}

void GBuffer::Clear(IDeviceContext* deviceContext, const float color[4]) {
    if (!deviceContext || !m_created) {
        LOG_ERROR("GBuffer", "无效的上下文或G-Buffer未创建");
        return;
    }

    LOG_DEBUG("GBuffer", "清除G-Buffer");
    (void)color;
}

void GBuffer::ClearDepth(IDeviceContext* deviceContext, float depth) {
    if (!deviceContext || !m_created) {
        LOG_ERROR("GBuffer", "无效的上下文或G-Buffer未创建");
        return;
    }

    LOG_DEBUG("GBuffer", "清除深度: {0}", depth);
    (void)depth;
}

void* GBuffer::GetRenderTargetView(GBufferTarget target) const {
    uint32_t index = static_cast<uint32_t>(target);
    if (index >= static_cast<uint32_t>(GBufferTarget::Count)) {
        return nullptr;
    }
    return m_renderTargets[index].renderTargetView;
}

void* GBuffer::GetShaderResourceView(GBufferTarget target) const {
    uint32_t index = static_cast<uint32_t>(target);
    if (index >= static_cast<uint32_t>(GBufferTarget::Count)) {
        return nullptr;
    }
    return m_renderTargets[index].shaderResourceView;
}

void* GBuffer::GetDepthStencilView() const {
    return m_depthStencilView;
}

ITextureRenderTarget* GBuffer::GetTarget(GBufferTarget target) {
    return nullptr;
}

IDepthStencil* GBuffer::GetDepthStencil() {
    return nullptr;
}

void GBuffer::GetColorTargets(ITextureRenderTarget** targets, uint32_t count) {
    (void)targets;
    (void)count;
}

TextureFormat GBuffer::GetTargetFormat(GBufferTarget target) const {
    switch (target) {
        case GBufferTarget::Position: return static_cast<TextureFormat>(GBufferFormats::POSITION_FORMAT);
        case GBufferTarget::Normal: return static_cast<TextureFormat>(GBufferFormats::NORMAL_FORMAT);
        case GBufferTarget::Albedo: return static_cast<TextureFormat>(GBufferFormats::ALBEDO_FORMAT);
        case GBufferTarget::Emissive: return static_cast<TextureFormat>(GBufferFormats::EMISSIVE_FORMAT);
        case GBufferTarget::Depth: return static_cast<TextureFormat>(GBufferFormats::DEPTH_FORMAT);
        default: return TextureFormat::Unknown;
    }
}

} // namespace PrismaEngine::Graphic