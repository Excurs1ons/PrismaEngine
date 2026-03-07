#pragma once

#include "interfaces/IResourceFactory.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "interfaces/IShader.h"
#include "interfaces/IPipelineState.h"
#include "interfaces/ISampler.h"
#include "interfaces/ISwapChain.h"
#include "interfaces/IFence.h"
#include <vulkan/vulkan.h>

namespace PrismaEngine::Graphic::Vulkan {

class RenderDeviceVulkan;

class VulkanResourceFactory : public IResourceFactory {
public:
    VulkanResourceFactory(RenderDeviceVulkan* device) : m_device(device) {}
    ~VulkanResourceFactory() override = default;

    bool Initialize(IRenderDevice* /*device*/) override { return true; }
    void Shutdown() override {}
    void Reset() override {}

    std::unique_ptr<ITexture> CreateTextureImpl(const TextureDesc& /*desc*/) override { return nullptr; }
    std::unique_ptr<ITexture> CreateTextureFromFile(const std::string& /*filename*/, const TextureDesc* /*desc*/ = nullptr) override { return nullptr; }
    std::unique_ptr<ITexture> CreateTextureFromMemory(const void* /*data*/, uint64_t /*dataSize*/, const TextureDesc& /*desc*/) override { return nullptr; }

    std::unique_ptr<IBuffer> CreateBufferImpl(const BufferDesc& /*desc*/) override { return nullptr; }
    std::unique_ptr<IBuffer> CreateDynamicBuffer(uint64_t /*size*/, BufferType /*type*/, BufferUsage /*usage*/) override { return nullptr; }

    std::unique_ptr<IShader> CreateShaderImpl(const ShaderDesc& /*desc*/, const std::vector<uint8_t>& /*bytecode*/, const ShaderReflection& /*reflection*/) override { return nullptr; }
    std::unique_ptr<IPipelineState> CreatePipelineStateImpl() override { return nullptr; }
    std::unique_ptr<ISampler> CreateSamplerImpl(const SamplerDesc& /*desc*/) override { return nullptr; }

    std::unique_ptr<ISwapChain> CreateSwapChainImpl(void* /*windowHandle*/, uint32_t /*width*/, uint32_t /*height*/, TextureFormat /*format*/, uint32_t /*bufferCount*/, bool /*vsync*/) override { return nullptr; }
    std::unique_ptr<IFence> CreateFenceImpl() override { return nullptr; }

    std::vector<std::unique_ptr<ITexture>> CreateTexturesBatch(const TextureDesc* /*descs*/, uint32_t /*count*/) override { return {}; }
    std::vector<std::unique_ptr<IBuffer>> CreateBuffersBatch(const BufferDesc* /*descs*/, uint32_t /*count*/) override { return {}; }

    uint64_t GetOrCreateTexturePool(TextureFormat /*format*/, uint32_t /*width*/, uint32_t /*height*/, uint32_t /*mipLevels*/, uint32_t /*arraySize*/) override { return 0; }
    std::unique_ptr<ITexture> AllocateFromTexturePool(uint64_t /*poolId*/) override { return nullptr; }
    void DeallocateToTexturePool(uint64_t /*poolId*/, ITexture* /*texture*/) override {}
    void CleanupResourcePools() override {}

    bool ValidateTextureDesc(const TextureDesc& /*desc*/, std::string& /*errorMsg*/) override { return true; }
    bool ValidateBufferDesc(const BufferDesc& /*desc*/, std::string& /*errorMsg*/) override { return true; }
    bool ValidateShaderDesc(const ShaderDesc& /*desc*/, std::string& /*errorMsg*/) override { return true; }

    void GetMemoryBudget(uint64_t& budget, uint64_t& usage) const override { budget = usage = 0; }
    void SetMemoryLimit(uint64_t /*limit*/) override {}
    bool IsMemoryLimitExceeded() const override { return false; }
    void ForceGarbageCollection() override {}

    ResourceCreationStats GetCreationStats() const override { return {}; }
    void ResetStats() override {}

    void EnableResourcePooling(bool /*enable*/) override {}
    void SetPoolingThreshold(uint64_t /*threshold*/) override {}
    void EnableDeferredDestruction(bool /*enable*/, uint32_t /*delayFrames*/) override {}
    void ProcessDeferredDestructions() override {}

private:
    RenderDeviceVulkan* m_device;
};

} // namespace PrismaEngine::Graphic::Vulkan