#pragma once

#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "RenderDesc.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

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
        TextureMapDesc mapDesc{};
        if (mipLevel >= m_desc.mipLevels || arraySlice >= m_desc.arraySize) {
            return mapDesc;
        }

        const uint64_t subresourceSize = GetSubresourceSize(mipLevel);
        const uint64_t offset = (static_cast<uint64_t>(arraySlice) * m_desc.mipLevels + mipLevel) * subresourceSize;
        const uint64_t requiredSize = offset + subresourceSize;
        if (m_shadowData.size() < requiredSize) {
            m_shadowData.resize(static_cast<size_t>(requiredSize), 0);
        }

        mapDesc.data = m_shadowData.data() + offset;
        mapDesc.rowPitch = std::max<uint64_t>(1, m_desc.width >> mipLevel) * GetBytesPerPixel();
        mapDesc.depthPitch = subresourceSize;
        mapDesc.size = subresourceSize;
        mapDesc.offset = offset;
        m_lastTextureMapType = mapType;
        return mapDesc;
    }
    void Unmap(uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        if (mipLevel < m_desc.mipLevels && arraySlice < m_desc.arraySize) {
            m_lastTextureMapType = 0;
        }
    }

    void UpdateData(const void* data, uint64_t dataSize, uint32_t mipLevel, uint32_t arraySlice,
                   uint32_t left, uint32_t top, uint32_t front, uint64_t width, uint64_t height, uint64_t depth) override {
        if (!data || mipLevel >= m_desc.mipLevels || arraySlice >= m_desc.arraySize) {
            return;
        }

        TextureMapDesc mapDesc = Map(mipLevel, arraySlice, 1);
        if (!mapDesc.data || mapDesc.size == 0) {
            return;
        }

        const uint64_t mipWidth = std::max<uint64_t>(1, m_desc.width >> mipLevel);
        const uint64_t mipHeight = std::max<uint64_t>(1, m_desc.height >> mipLevel);
        const uint64_t bytesPerPixel = GetBytesPerPixel();
        const uint64_t copyWidth = std::min<uint64_t>(width == 0 ? mipWidth : width, mipWidth);
        const uint64_t copyHeight = std::min<uint64_t>(height == 0 ? mipHeight : height, mipHeight);
        const uint64_t copyDepth = std::min<uint64_t>(depth == 0 ? std::max<uint32_t>(1, m_desc.depth) : depth, std::max<uint32_t>(1, m_desc.depth));
        const uint64_t rowBytes = copyWidth * bytesPerPixel;
        const uint64_t requiredBytes = rowBytes * copyHeight * copyDepth;
        const uint64_t safeCopyBytes = std::min<uint64_t>(dataSize, requiredBytes);
        if (safeCopyBytes == 0) {
            return;
        }

        auto* destinationBytes = static_cast<std::byte*>(mapDesc.data);
        const auto* sourceBytes = static_cast<const std::byte*>(data);
        const uint64_t baseOffset = ((static_cast<uint64_t>(front) * mipHeight + top) * mipWidth + left) * bytesPerPixel;
        if (baseOffset >= mapDesc.size) {
            return;
        }

        std::memcpy(destinationBytes + baseOffset, sourceBytes, std::min<uint64_t>(safeCopyBytes, mapDesc.size - baseOffset));
    }

    void GenerateMips() override {
        if (m_desc.mipLevels <= 1 || m_shadowData.empty()) {
            return;
        }

        for (uint32_t mipLevel = 1; mipLevel < m_desc.mipLevels; ++mipLevel) {
            TextureMapDesc destination = Map(mipLevel, 0, 1);
            TextureMapDesc source = Map(mipLevel - 1, 0, m_lastTextureMapType);
            if (!destination.data || !source.data) {
                continue;
            }

            std::memcpy(destination.data,
                        source.data,
                        static_cast<size_t>(std::min<uint64_t>(destination.size, source.size)));
        }
    }

    void CopyFrom(ITexture* srcTexture, uint32_t srcMipLevel = 0, uint32_t srcArraySlice = 0,
                 uint32_t dstMipLevel = 0, uint32_t dstArraySlice = 0) override {
        if (!srcTexture) {
            return;
        }

        const uint64_t copySize = std::min<uint64_t>(GetSubresourceSize(dstMipLevel), srcTexture->GetSubresourceSize(srcMipLevel));
        if (copySize == 0) {
            return;
        }

        std::vector<std::byte> tempBuffer(static_cast<size_t>(copySize));
        if (srcTexture->ReadData(srcMipLevel, srcArraySlice, tempBuffer.data(), copySize)) {
            UpdateData(tempBuffer.data(), copySize, dstMipLevel, dstArraySlice, 0, 0, 0, 0, 0, 0);
        }
    }

    bool ReadData(uint32_t mipLevel, uint32_t arraySlice, void* dstBuffer, uint64_t bufferSize) override {
        if (!dstBuffer) {
            return false;
        }

        TextureMapDesc mapDesc = Map(mipLevel, arraySlice, 2);
        if (!mapDesc.data || bufferSize < mapDesc.size) {
            return false;
        }

        std::memcpy(dstBuffer, mapDesc.data, static_cast<size_t>(mapDesc.size));
        return true;
    }

    uint64_t CreateDescriptor(TextureDescriptorType descType, TextureFormat format = TextureFormat::Unknown,
                              uint32_t mipLevel = 0, uint32_t arraySize = 0) override {
        if (format != TextureFormat::Unknown) {
            m_lastRequestedTextureFormat = format;
        }
        m_lastDescriptorType = descType;
        m_lastDescriptorMipLevel = mipLevel;
        m_lastDescriptorArraySize = arraySize == 0 ? m_desc.arraySize : arraySize;
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_imageView));
    }

    uint64_t GetDefaultSRV() const override { return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_imageView)); }
    uint64_t GetDefaultRTV() const override { return IsRenderTarget() ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_imageView)) : 0; }
    uint64_t GetDefaultDSV() const override { return IsDepthStencil() ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_imageView)) : 0; }
    uint64_t GetDefaultUAV() const override { return IsUnorderedAccess() ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_imageView)) : 0; }

    void Clear(const Color& color, uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        TextureMapDesc mapDesc = Map(mipLevel, arraySlice, 1);
        if (!mapDesc.data || GetBytesPerPixel() < 4) {
            return;
        }

        const uint8_t rgba[4] = {
            static_cast<uint8_t>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f),
            static_cast<uint8_t>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f)
        };

        auto* bytes = static_cast<uint8_t*>(mapDesc.data);
        for (uint64_t offset = 0; offset + 3 < mapDesc.size; offset += GetBytesPerPixel()) {
            bytes[offset + 0] = rgba[0];
            bytes[offset + 1] = rgba[1];
            bytes[offset + 2] = rgba[2];
            bytes[offset + 3] = rgba[3];
        }
    }

    void ClearDepthStencil(float depth = 1.0f, uint8_t stencil = 0) override {
        if (!IsDepthStencil()) {
            return;
        }

        TextureMapDesc mapDesc = Map(0, 0, 1);
        if (!mapDesc.data || mapDesc.size < sizeof(float)) {
            return;
        }

        auto* bytes = static_cast<std::byte*>(mapDesc.data);
        for (uint64_t offset = 0; offset + sizeof(float) <= mapDesc.size; offset += sizeof(float)) {
            std::memcpy(bytes + offset, &depth, sizeof(float));
            if (offset + sizeof(float) < mapDesc.size) {
                bytes[offset + sizeof(float)] = static_cast<std::byte>(stencil);
            }
        }
    }

    void ResolveMultisampled(ITexture* dstTexture, TextureFormat format = TextureFormat::Unknown) override {
        if (!dstTexture) {
            return;
        }

        const uint64_t copySize = std::min<uint64_t>(GetSubresourceSize(0), dstTexture->GetSubresourceSize(0));
        if (copySize == 0) {
            return;
        }

        std::vector<std::byte> tempBuffer(static_cast<size_t>(copySize));
        if (ReadData(0, 0, tempBuffer.data(), copySize)) {
            dstTexture->UpdateData(tempBuffer.data(), copySize, 0, 0, 0, 0, 0, m_desc.width, m_desc.height, m_desc.depth);
            if (format != TextureFormat::Unknown) {
                m_lastRequestedTextureFormat = format;
            }
        }
    }

    void Discard(uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        if (mipLevel >= m_desc.mipLevels || arraySlice >= m_desc.arraySize) {
            return;
        }

        TextureMapDesc mapDesc = Map(mipLevel, arraySlice, 1);
        if (mapDesc.data) {
            std::memset(mapDesc.data, 0, static_cast<size_t>(mapDesc.size));
        }
    }

    void Compact() override {
        m_shadowData.shrink_to_fit();
    }

    uint64_t GetMemoryUsage() const override {
        return !m_shadowData.empty() ? static_cast<uint64_t>(m_shadowData.size())
                                     : GetSubresourceSize(0) * std::max<uint32_t>(1, m_desc.arraySize);
    }

    bool DebugSaveToFile(const std::string& filename, uint32_t mipLevel = 0, uint32_t arraySlice = 0) override {
        TextureMapDesc mapDesc = Map(mipLevel, arraySlice, 2);
        if (!mapDesc.data) {
            return false;
        }

        std::ofstream stream(filename, std::ios::binary);
        if (!stream.is_open()) {
            return false;
        }

        stream.write(static_cast<const char*>(mapDesc.data), static_cast<std::streamsize>(mapDesc.size));
        return stream.good();
    }

    bool Validate() override { return m_image != VK_NULL_HANDLE || !m_shadowData.empty(); }

    VkImage GetVkImage() const { return m_image; }
    VkImageView GetVkImageView() const { return m_imageView; }

private:
    // 存储分配器和分配信息以实现析构时的自动资源销毁
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    TextureDesc m_desc;
    std::vector<uint8_t> m_shadowData;
    TextureDescriptorType m_lastDescriptorType = TextureDescriptorType::ShaderResourceView;
    TextureFormat m_lastRequestedTextureFormat = TextureFormat::Unknown;
    uint32_t m_lastDescriptorMipLevel = 0;
    uint32_t m_lastDescriptorArraySize = 0;
    uint32_t m_lastTextureMapType = 0;
};

class VulkanBuffer : public IBuffer {
public:
    VulkanBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation, const BufferDesc& desc);
    ~VulkanBuffer() override;

    ResourceType GetType() const override { return ResourceType::Buffer; }
    BufferType GetBufferType() const override { return BufferType::Unknown; }
    uint64_t GetSize() const override { return m_desc.size; }
    BufferUsage GetUsage() const override { return m_desc.usage; }
    bool IsDynamic() const override { return HasFlag(m_desc.usage, BufferUsage::Dynamic); }

    BufferMapDesc Map(uint64_t offset = 0, uint64_t size = 0, uint32_t mapType = 0) override {
        BufferMapDesc mapDesc{};
        const uint64_t requestedSize = size == 0 ? m_desc.size - std::min<uint64_t>(offset, m_desc.size) : size;
        const uint64_t clampedOffset = std::min<uint64_t>(offset, m_desc.size);
        const uint64_t clampedSize = std::min<uint64_t>(requestedSize, m_desc.size - clampedOffset);
        const uint64_t requiredSize = clampedOffset + clampedSize;
        if (m_shadowData.size() < requiredSize) {
            m_shadowData.resize(static_cast<size_t>(requiredSize), 0);
        }

        mapDesc.data = m_shadowData.data() + clampedOffset;
        mapDesc.size = clampedSize;
        mapDesc.offset = clampedOffset;
        m_lastBufferMapType = mapType;
        return mapDesc;
    }
    void Unmap(uint64_t offset = 0, uint64_t size = 0) override {
        const uint64_t touchedSize = size == 0 ? (m_desc.size - std::min<uint64_t>(offset, m_desc.size)) : size;
        if (offset <= m_desc.size && touchedSize <= m_desc.size) {
            m_lastBufferMapType = 0;
        }
    }
    void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) override {
        if (!data || offset >= m_desc.size) {
            return;
        }

        BufferMapDesc mapDesc = Map(offset, size, 1);
        if (mapDesc.data && mapDesc.size > 0) {
            std::memcpy(mapDesc.data, data, static_cast<size_t>(mapDesc.size));
        }
    }
    bool ReadData(void* dstBuffer, uint64_t size, uint64_t offset = 0) override {
        if (!dstBuffer || offset >= m_desc.size) {
            return false;
        }

        BufferMapDesc mapDesc = Map(offset, size, 2);
        if (!mapDesc.data || mapDesc.size < size) {
            return false;
        }

        std::memcpy(dstBuffer, mapDesc.data, static_cast<size_t>(size));
        return true;
    }
    void CopyTo(IBuffer* dstBuffer, uint64_t srcOffset = 0, uint64_t dstOffset = 0, uint64_t size = 0) override {
        if (!dstBuffer || srcOffset >= m_desc.size) {
            return;
        }

        const uint64_t copySize = size == 0 ? (m_desc.size - srcOffset) : size;
        std::vector<std::byte> tempBuffer(static_cast<size_t>(copySize));
        if (ReadData(tempBuffer.data(), copySize, srcOffset)) {
            dstBuffer->UpdateData(tempBuffer.data(), copySize, dstOffset);
        }
    }
    void Fill(uint32_t value, uint64_t offset = 0, uint64_t size = 0) override {
        BufferMapDesc mapDesc = Map(offset, size, 1);
        if (!mapDesc.data || mapDesc.size == 0) {
            return;
        }

        auto* bufferWords = static_cast<uint32_t*>(mapDesc.data);
        const uint64_t wordCount = mapDesc.size / sizeof(uint32_t);
        for (uint64_t wordIndex = 0; wordIndex < wordCount; ++wordIndex) {
            bufferWords[wordIndex] = value;
        }
    }
    void CopyFromTexture(ITexture* srcTexture, uint32_t srcMipLevel = 0, uint32_t srcArraySlice = 0) override {
        if (!srcTexture) {
            return;
        }

        const uint64_t copySize = std::min<uint64_t>(m_desc.size, srcTexture->GetSubresourceSize(srcMipLevel));
        std::vector<std::byte> tempBuffer(static_cast<size_t>(copySize));
        if (srcTexture->ReadData(srcMipLevel, srcArraySlice, tempBuffer.data(), copySize)) {
            UpdateData(tempBuffer.data(), copySize, 0);
        }
    }
    void CopyToTexture(ITexture* dstTexture, uint32_t dstMipLevel = 0, uint32_t dstArraySlice = 0) override {
        if (!dstTexture) {
            return;
        }

        const uint64_t copySize = std::min<uint64_t>(m_desc.size, dstTexture->GetSubresourceSize(dstMipLevel));
        std::vector<std::byte> tempBuffer(static_cast<size_t>(copySize));
        if (ReadData(tempBuffer.data(), copySize, 0)) {
            dstTexture->UpdateData(tempBuffer.data(), copySize, dstMipLevel, dstArraySlice, 0, 0, 0, 0, 0, 0);
        }
    }

    uint32_t GetStride() const override { return 0; }
    uint32_t GetElementCount() const override { return static_cast<uint32_t>(m_desc.size); }
    bool IsReadOnly() const override { return false; }
    bool IsShaderResource() const override { return HasFlag(m_desc.usage, BufferUsage::ShaderResource); }
    bool IsUnorderedAccess() const override { return HasFlag(m_desc.usage, BufferUsage::UnorderedAccess); }

    uint64_t CreateView(BufferDescriptorType descType, const BufferViewDesc& desc = {}) override {
        m_lastBufferDescriptorType = descType;
        m_lastBufferViewDesc = desc;
        return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_buffer));
    }

    uint64_t GetDefaultSRV() const override { return IsShaderResource() ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_buffer)) : 0; }
    uint64_t GetDefaultUAV() const override { return IsUnorderedAccess() ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_buffer)) : 0; }
    uint64_t GetDefaultCBV() const override { return GetBufferType() == BufferType::Constant ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_buffer)) : 0; }
    uint64_t GetDefaultVBV() const override { return GetBufferType() == BufferType::Vertex ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_buffer)) : 0; }
    uint64_t GetDefaultIBV() const override { return GetBufferType() == BufferType::Index ? static_cast<uint64_t>(reinterpret_cast<uintptr_t>(m_buffer)) : 0; }

    void CopyFromBuffer(IBuffer* srcBuffer, uint64_t srcOffset = 0, uint64_t dstOffset = 0, uint64_t size = 0) override {
        if (!srcBuffer || dstOffset >= m_desc.size) {
            return;
        }

        const uint64_t copySize = size == 0 ? (m_desc.size - dstOffset) : size;
        std::vector<std::byte> tempBuffer(static_cast<size_t>(copySize));
        if (srcBuffer->ReadData(tempBuffer.data(), copySize, srcOffset)) {
            UpdateData(tempBuffer.data(), copySize, dstOffset);
        }
    }

    uint64_t AllocateDynamic(uint64_t size, uint64_t alignment = 256) override {
        const uint64_t alignedOffset = alignment == 0
            ? m_dynamicOffset
            : ((m_dynamicOffset + alignment - 1) / alignment) * alignment;
        if (alignedOffset + size > m_desc.size) {
            return m_desc.size;
        }
        m_dynamicOffset = alignedOffset + size;
        return alignedOffset;
    }
    void ResetDynamicAllocation() override { m_dynamicOffset = 0; }
    uint64_t GetCurrentDynamicOffset() const override { return m_dynamicOffset; }
    uint64_t GetAvailableDynamicSpace() const override { return m_desc.size > m_dynamicOffset ? (m_desc.size - m_dynamicOffset) : 0; }

    bool DebugSaveToFile(const std::string& filename, const std::string& format = "hex", uint64_t offset = 0, uint64_t size = 0) override {
        BufferMapDesc mapDesc = Map(offset, size, 2);
        if (!mapDesc.data) {
            return false;
        }

        std::ofstream stream(filename, std::ios::binary);
        if (!stream.is_open()) {
            return false;
        }

        if (format == "hex") {
            static constexpr char digits[] = "0123456789ABCDEF";
            const auto* bytes = static_cast<const uint8_t*>(mapDesc.data);
            for (uint64_t index = 0; index < mapDesc.size; ++index) {
                const char pair[2] = {digits[(bytes[index] >> 4) & 0x0F], digits[bytes[index] & 0x0F]};
                stream.write(pair, 2);
            }
        } else {
            stream.write(static_cast<const char*>(mapDesc.data), static_cast<std::streamsize>(mapDesc.size));
        }

        return stream.good();
    }
    bool DebugValidateContent(const void* expectedData, uint64_t size, uint64_t offset = 0) override {
        if (!expectedData) {
            return false;
        }

        BufferMapDesc mapDesc = Map(offset, size, 2);
        if (!mapDesc.data || mapDesc.size < size) {
            return false;
        }

        return std::memcmp(mapDesc.data, expectedData, static_cast<size_t>(size)) == 0;
    }
    void DebugPrintInfo() const override {}

    void Discard(uint64_t offset = 0, uint64_t size = 0) override {
        BufferMapDesc mapDesc = const_cast<VulkanBuffer*>(this)->Map(offset, size, 1);
        if (mapDesc.data) {
            std::memset(mapDesc.data, 0, static_cast<size_t>(mapDesc.size));
        }
    }
    void Reserve(uint64_t size) override {
        if (size > m_shadowData.size()) {
            m_shadowData.resize(static_cast<size_t>(std::min<uint64_t>(size, m_desc.size)), 0);
        }
    }
    void Compact() override { m_shadowData.shrink_to_fit(); }

    uint64_t GetMemoryUsage() const override { return !m_shadowData.empty() ? static_cast<uint64_t>(m_shadowData.size()) : m_desc.size; }
    uint64_t GetGPUMemoryUsage() const override { return m_desc.size; }

    VkBuffer GetVkBuffer() const { return m_buffer; }

private:
    // 存储分配器和分配信息以实现析构时的自动资源销毁
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    BufferDesc m_desc;
    std::vector<uint8_t> m_shadowData;
    BufferDescriptorType m_lastBufferDescriptorType = BufferDescriptorType::ShaderResourceView;
    BufferViewDesc m_lastBufferViewDesc{};
    uint64_t m_dynamicOffset = 0;
    uint32_t m_lastBufferMapType = 0;
};

} // namespace Prisma::Graphic::Vulkan
