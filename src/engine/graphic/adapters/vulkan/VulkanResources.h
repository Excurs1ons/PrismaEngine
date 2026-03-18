#pragma once

#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "RenderDesc.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <algorithm>

namespace Prisma::Graphic::Vulkan {

class VulkanTexture : public ITexture {
public:
    VulkanTexture(VmaAllocator allocator, VkImage image, VmaAllocation allocation, VkImageView imageView, const TextureDesc& desc);
    ~VulkanTexture() override;

    ResourceType GetType() const override { return ResourceType::Texture; }
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

    void UpdateData(const void* data, uint64_t dataSize, uint32_t mipLevel, uint32_t arraySlice,
                   uint32_t left, uint32_t top, uint32_t front, uint64_t width, uint64_t height, uint64_t depth) override {
        (void)data; (void)dataSize; (void)mipLevel; (void)arraySlice;
        (void)left; (void)top; (void)front; (void)width; (void)height; (void)depth;
    }

    void GenerateMips() override {}

    void CopyFrom(ITexture* srcTexture, uint32_t srcMipLevel = 0, uint32_t srcArraySlice = 0,
                 uint32_t dstMipLevel = 0, uint32_t dstArraySlice = 0) override {
        (void)srcTexture; (void)srcMipLevel; (void)srcArraySlice;
        (void)dstMipLevel; (void)dstArraySlice;
    }

    bool ReadData(uint32_t mipLevel, uint32_t arraySlice, void* dstBuffer, uint64_t bufferSize) override {
        (void)mipLevel; (void)arraySlice; (void)dstBuffer; (void)bufferSize;
        return false;
    }

    uint64_t CreateDescriptor(TextureDescriptorType descType, TextureFormat format = TextureFormat::Unknown,
                              uint32_t mipLevel = 0, uint32_t arraySize = 0) override {
        (void)descType; (void)format; (void)mipLevel; (void)arraySize;
        return 0;
    }

    uint64_t GetDefaultSRV() const override { return 0; }
    uint64_t GetDefaultRTV() const override { return 0; }
    uint64_t GetDefaultDSV() const override { return 0; }
    uint64_t GetDefaultUAV() const override { return 0; }

    void Clear(const Color& color, uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        (void)color; (void)mipLevel; (void)arraySlice;
    }

    void ClearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) override {
        (void)depth; (void)stencil;
    }

    void ResolveMultisampled(ITexture* dstTexture, TextureFormat format = TextureFormat::Unknown) override {
        (void)dstTexture; (void)format;
    }

    void Discard(uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        (void)mipLevel; (void)arraySlice;
    }

    void Compact() override {}

    uint64_t GetMemoryUsage() const override { return 0; }

    bool DebugSaveToFile(const std::string& filename, uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        (void)filename; (void)mipLevel; (void)arraySlice;
        return false;
    }

    bool Validate() override { return true; }

    VkImage GetVkImage() const { return m_image; }
    VkImageView GetVkImageView() const { return m_imageView; }

private:
    // 存储分配器和分配信息以实现析构时的自动资源销毁
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    TextureDesc m_desc;
};

class VulkanBuffer : public IBuffer {
public:
    VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, const BufferDesc& desc);
    ~VulkanBuffer() override;

    ResourceType GetType() const override { return ResourceType::Buffer; }
    BufferType GetBufferType() const override { return BufferType::Unknown; }
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

    uint32_t GetStride() const override { return 0; }
    uint32_t GetElementCount() const override { return static_cast<uint32_t>(m_desc.size); }
    bool IsReadOnly() const override { return false; }
    bool IsShaderResource() const override { return false; }
    bool IsUnorderedAccess() const override { return false; }

    uint64_t CreateView(BufferDescriptorType descType, const BufferViewDesc& desc = {}) override {
        (void)descType; (void)desc;
        return 0;
    }

    uint64_t GetDefaultSRV() const override { return 0; }
    uint64_t GetDefaultUAV() const override { return 0; }
    uint64_t GetDefaultCBV() const override { return 0; }
    uint64_t GetDefaultVBV() const override { return 0; }
    uint64_t GetDefaultIBV() const override { return 0; }

    void CopyFromBuffer(IBuffer* srcBuffer, uint64_t srcOffset = 0, uint64_t dstOffset = 0, uint64_t size = 0) override {
        (void)srcBuffer; (void)srcOffset; (void)dstOffset; (void)size;
    }

    uint64_t AllocateDynamic(uint64_t size, uint64_t alignment = 256) override { (void)size; (void)alignment; return 0; }
    void ResetDynamicAllocation() override {}
    uint64_t GetCurrentDynamicOffset() const override { return 0; }
    uint64_t GetAvailableDynamicSpace() const override { return 0; }

    bool DebugSaveToFile(const std::string& filename, const std::string& format = "hex", uint64_t offset = 0, uint64_t size = 0) override {
        (void)filename; (void)format; (void)offset; (void)size;
        return false;
    }
    bool DebugValidateContent(const void* expectedData, uint64_t size, uint64_t offset = 0) override {
        (void)expectedData; (void)size; (void)offset;
        return false;
    }
    void DebugPrintInfo() const override {}

    void Discard(uint64_t offset = 0, uint64_t size = 0) override { (void)offset; (void)size; }
    void Reserve(uint64_t size) override { (void)size; }
    void Compact() override {}

    uint64_t GetMemoryUsage() const override { return 0; }
    uint64_t GetGPUMemoryUsage() const override { return 0; }

    VkBuffer GetVkBuffer() const { return m_buffer; }

private:
    // 存储分配器和分配信息以实现析构时的自动资源销毁
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    BufferDesc m_desc;
};

} // namespace Prisma::Graphic::Vulkan
