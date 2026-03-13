#pragma once

#include "interfaces/IPipelineState.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>

namespace PrismaEngine::Graphic::Vulkan {

class VulkanPipelineState : public IPipelineState {
public:
    VulkanPipelineState();
    ~VulkanPipelineState() override;

    [[nodiscard]] PipelineType GetType() const override { return m_type; }
    [[nodiscard]] bool IsValid() const override { return m_isValid; }

    void SetShader(ShaderType type, std::shared_ptr<IShader> shader) override;
    [[nodiscard]] std::shared_ptr<IShader> GetShader(ShaderType type) const override;
    [[nodiscard]] bool HasShader(ShaderType type) const override;

    void SetPrimitiveTopology(PrimitiveTopology topology) override;
    [[nodiscard]] PrimitiveTopology GetPrimitiveTopology() const override;

    void SetBlendState(const BlendState& state, uint32_t renderTargetIndex = 0) override;
    [[nodiscard]] const BlendState& GetBlendState(uint32_t renderTargetIndex = 0) const override;

    void SetRasterizerState(const RasterizerState& state) override;
    [[nodiscard]] const RasterizerState& GetRasterizerState() const override;

    void SetDepthStencilState(const DepthStencilState& state) override;
    [[nodiscard]] const DepthStencilState& GetDepthStencilState() const override;

    void SetInputLayout(const std::vector<VertexInputAttribute>& attributes) override;
    [[nodiscard]] const std::vector<VertexInputAttribute>& GetInputLayout() const override;
    [[nodiscard]] uint32_t GetInputAttributeCount() const override;

    void SetRenderTargetFormats(const std::vector<TextureFormat>& formats) override;
    void SetRenderTargetFormat(uint32_t index, TextureFormat format) override;
    [[nodiscard]] TextureFormat GetRenderTargetFormat(uint32_t index) const override;
    [[nodiscard]] uint32_t GetRenderTargetCount() const override;

    void SetDepthStencilFormat(TextureFormat format) override;
    [[nodiscard]] TextureFormat GetDepthStencilFormat() const override;

    void SetSampleCount(uint32_t sampleCount, uint32_t sampleQuality = 0) override;
    [[nodiscard]] uint32_t GetSampleCount() const override;
    [[nodiscard]] uint32_t GetSampleQuality() const override;

    bool Create(IRenderDevice* device) override;
    bool Recreate() override;
    bool Validate(IRenderDevice* device, std::string& errors) const override;

    [[nodiscard]] uint64_t GetCacheKey() const override;
    bool LoadFromCache(IRenderDevice* device, uint64_t cacheKey) override;
    bool SaveToCache() const override;

    const std::string& GetErrors() const override { return m_errors; }
    void SetDebugName(const std::string& name) override { m_debugName = name; }
    const std::string& GetDebugName() const override { return m_debugName; }

    std::unique_ptr<IPipelineState> Clone() const override;

    // Vulkan-specific
    VkPipeline GetVkPipeline() const { return m_pipeline; }
    VkPipelineLayout GetVkPipelineLayout() const { return m_pipelineLayout; }

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    std::unordered_map<ShaderType, std::shared_ptr<IShader>> m_shaders;
};

} // namespace PrismaEngine::Graphic::Vulkan
