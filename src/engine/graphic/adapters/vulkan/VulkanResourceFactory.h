#pragma once

#include "interfaces/IResourceFactory.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "interfaces/IShader.h"
#include "interfaces/IPipelineState.h"
#include "interfaces/ISampler.h"
#include "interfaces/ISwapChain.h"
#include "interfaces/IFence.h"
#include "VulkanResources.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace PrismaEngine::Graphic::Vulkan {

class RenderDeviceVulkan;

class VulkanResourceFactory : public IResourceFactory {
public:
    VulkanResourceFactory(RenderDeviceVulkan* device);
    ~VulkanResourceFactory() override;

    bool Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Reset() override;

    std::unique_ptr<ITexture> CreateTextureImpl(const TextureDesc& desc) override;
    std::unique_ptr<ITexture> CreateTextureFromFile(const std::string& filename, const TextureDesc* desc = nullptr) override;
    std::unique_ptr<ITexture> CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) override;

    std::unique_ptr<IBuffer> CreateBufferImpl(const BufferDesc& desc) override;
    std::unique_ptr<IBuffer> CreateDynamicBuffer(uint64_t size, BufferType type, BufferUsage usage) override;

    std::unique_ptr<IShader> CreateShaderImpl(const ShaderDesc& desc, const std::vector<uint8_t>& bytecode, const ShaderReflection& reflection) override;
    std::unique_ptr<IPipelineState> CreatePipelineStateImpl() override;
    std::unique_ptr<ISampler> CreateSamplerImpl(const SamplerDesc& desc) override;

    std::unique_ptr<ISwapChain> CreateSwapChainImpl(void* windowHandle, uint32_t width, uint32_t height, TextureFormat format, uint32_t bufferCount, bool vsync) override;
    std::unique_ptr<IFence> CreateFenceImpl() override;

    std::vector<std::unique_ptr<ITexture>> CreateTexturesBatch(const TextureDesc* descs, uint32_t count) override;
    std::vector<std::unique_ptr<IBuffer>> CreateBuffersBatch(const BufferDesc* descs, uint32_t count) override;

    uint64_t GetOrCreateTexturePool(TextureFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arraySize) override;
    std::unique_ptr<ITexture> AllocateFromTexturePool(uint64_t poolId) override;
    void DeallocateToTexturePool(uint64_t poolId, ITexture* texture) override;
    void CleanupResourcePools() override;

    bool ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) override;
    bool ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) override;
    bool ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) override;

    void GetMemoryBudget(uint64_t& budget, uint64_t& usage) const override;
    void SetMemoryLimit(uint64_t limit) override;
    bool IsMemoryLimitExceeded() const override;
    void ForceGarbageCollection() override;

    ResourceCreationStats GetCreationStats() const override;
    void ResetStats() override;

    void EnableResourcePooling(bool enable) override;
    void SetPoolingThreshold(uint64_t threshold) override;
    void EnableDeferredDestruction(bool enable, uint32_t delayFrames) override;
    void ProcessDeferredDestructions() override;

private:
    RenderDeviceVulkan* m_device = nullptr;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
};

} // namespace PrismaEngine::Graphic::Vulkan