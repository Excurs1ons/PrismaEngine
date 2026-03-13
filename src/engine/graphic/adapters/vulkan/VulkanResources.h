#pragma once

#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "RenderDesc.h"
#include <vulkan/vulkan.h>
#include <algorithm>

namespace PrismaEngine::Graphic::Vulkan {

class VulkanTexture : public ITexture {
public:
    VulkanTexture(VkImage image, VkImageView imageView, const TextureDesc& desc);
    ~VulkanTexture() override;

    TextureType GetTextureType() const override { return m_desc.type; }
    TextureFormat GetFormat() const override { return m_desc.format; }
    float GetWidth() const override { return static_cast<float>(m_desc.width); }
    float GetHeight() const override { return static_cast<float>(m_desc.height); }
    uint32_t GetDepth() const override { return m_desc.depth; }
    uint32_t GetMipLevels() const override { return m_desc.mipLevels; }
    uint32_t GetArraySize() const override { return m_desc.arraySize; }
    uint32_t GetSampleCount() const override { return m_desc.sampleCount; }
    uint32_t GetSampleQuality() const override { return m_desc.sampleQuality; }
    bool IsRenderTarget() const override { return m_desc.allowRenderTarget; }
    bool IsDepthStencil() const override { return m_desc.allowDepthStencil; }
    bool IsShaderResource() const override { return m_desc.allowShaderResource; }
    bool IsUnorderedAccess() const override { return m_desc.allowUnorderedAccess; }
    uint64_t GetBytesPerPixel() const override;
    uint64_t GetSubresourceSize(uint32_t mipLevel) const override;

    TextureMapDesc Map(uint32_t mipLevel = 0, uint32_t arraySlice = 0, uint32_t mapType = 0) override {
        (void)mipLevel; (void)arraySlice; (void)mapType;
        return {};
    }
    void Unmap(uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        (void)mipLevel; (void)arraySlice;
    }

    VkImage GetVkImage() const { return m_image; }
    VkImageView GetVkImageView() const { return m_imageView; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    TextureDesc m_desc;
};

class VulkanBuffer : public IBuffer {
public:
    VulkanBuffer(VkBuffer buffer, const BufferDesc& desc);
    ~VulkanBuffer() override;

    ResourceType GetType() const override { return ResourceType::Buffer; }
    uint64_t GetSize() const override { return m_desc.size; }
    BufferUsage GetUsage() const override { return m_desc.usage; }
    bool IsDynamic() const override { return false; }

    BufferMapDesc Map(uint64_t offset = 0, uint64_t size = 0, uint32_t mapType = 0) override {
        (void)offset; (void)size; (void)mapType;
        return {};
    }
    void Unmap(uint64_t offset = 0, uint64_t size = 0) override { (void)offset; (void)size; }
    void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) override { (void)data; (void)size; (void)offset; }
    bool ReadData(void* dstBuffer, uint64_t size, uint64_t offset = 0) override { (void)dstBuffer; (void)size; (void)offset; return false; }
    void CopyTo(IBuffer* dstBuffer, uint64_t srcOffset = 0, uint64_t dstOffset = 0, uint64_t size = 0) override { (void)dstBuffer; (void)srcOffset; (void)dstOffset; (void)size; }
    void Fill(uint32_t value, uint64_t offset = 0, uint64_t size = 0) override { (void)value; (void)offset; (void)size; }
    void CopyFromTexture(ITexture* srcTexture, uint32_t srcMipLevel = 0, uint32_t srcArraySlice = 0) override { (void)srcTexture; (void)srcMipLevel; (void)srcArraySlice; }
    void CopyToTexture(ITexture* dstTexture, uint32_t dstMipLevel = 0, uint32_t dstArraySlice = 0) override { (void)dstTexture; (void)dstMipLevel; (void)dstArraySlice; }

    VkBuffer GetVkBuffer() const { return m_buffer; }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    BufferDesc m_desc;
};

} // namespace PrismaEngine::Graphic::Vulkan
