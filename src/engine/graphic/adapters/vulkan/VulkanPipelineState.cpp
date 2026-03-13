#include "VulkanPipelineState.h"
#include "VulkanShader.h"

namespace PrismaEngine::Graphic::Vulkan {

VulkanPipelineState::VulkanPipelineState() {
    m_blendState = BlendState::Default;
    m_rasterizerState = RasterizerState::Default;
    m_depthStencilState = DepthStencilState::Default;
    m_renderTargetFormats.push_back(TextureFormat::RGBA8_UNorm);
}

VulkanPipelineState::~VulkanPipelineState() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanPipelineState::SetShader(ShaderType type, std::shared_ptr<IShader> shader) {
    m_shaders[type] = shader;
    m_isValid = false;
}

std::shared_ptr<IShader> VulkanPipelineState::GetShader(ShaderType type) const {
    auto it = m_shaders.find(type);
    if (it != m_shaders.end()) {
        return it->second;
    }
    return nullptr;
}

bool VulkanPipelineState::HasShader(ShaderType type) const {
    return m_shaders.find(type) != m_shaders.end();
}

void VulkanPipelineState::SetPrimitiveTopology(PrimitiveTopology topology) {
    m_topology = topology;
    m_isValid = false;
}

PrimitiveTopology VulkanPipelineState::GetPrimitiveTopology() const {
    return m_topology;
}

void VulkanPipelineState::SetBlendState(const BlendState& state, uint32_t renderTargetIndex) {
    if (renderTargetIndex == 0) {
        m_blendState = state;
    }
    m_isValid = false;
}

const BlendState& VulkanPipelineState::GetBlendState(uint32_t renderTargetIndex) const {
    (void)renderTargetIndex;
    return m_blendState;
}

void VulkanPipelineState::SetRasterizerState(const RasterizerState& state) {
    m_rasterizerState = state;
    m_isValid = false;
}

const RasterizerState& VulkanPipelineState::GetRasterizerState() const {
    return m_rasterizerState;
}

void VulkanPipelineState::SetDepthStencilState(const DepthStencilState& state) {
    m_depthStencilState = state;
    m_isValid = false;
}

const DepthStencilState& VulkanPipelineState::GetDepthStencilState() const {
    return m_depthStencilState;
}

void VulkanPipelineState::SetInputLayout(const std::vector<VertexInputAttribute>& attributes) {
    m_inputAttributes = attributes;
    m_isValid = false;
}

const std::vector<VertexInputAttribute>& VulkanPipelineState::GetInputLayout() const {
    return m_inputAttributes;
}

uint32_t VulkanPipelineState::GetInputAttributeCount() const {
    return static_cast<uint32_t>(m_inputAttributes.size());
}

void VulkanPipelineState::SetRenderTargetFormats(const std::vector<TextureFormat>& formats) {
    m_renderTargetFormats = formats;
    m_isValid = false;
}

void VulkanPipelineState::SetRenderTargetFormat(uint32_t index, TextureFormat format) {
    if (index < m_renderTargetFormats.size()) {
        m_renderTargetFormats[index] = format;
    } else {
        m_renderTargetFormats.push_back(format);
    }
    m_isValid = false;
}

TextureFormat VulkanPipelineState::GetRenderTargetFormat(uint32_t index) const {
    if (index < m_renderTargetFormats.size()) {
        return m_renderTargetFormats[index];
    }
    return TextureFormat::Unknown;
}

uint32_t VulkanPipelineState::GetRenderTargetCount() const {
    return static_cast<uint32_t>(m_renderTargetFormats.size());
}

void VulkanPipelineState::SetDepthStencilFormat(TextureFormat format) {
    m_depthStencilFormat = format;
    m_isValid = false;
}

TextureFormat VulkanPipelineState::GetDepthStencilFormat() const {
    return m_depthStencilFormat;
}

void VulkanPipelineState::SetSampleCount(uint32_t sampleCount, uint32_t sampleQuality) {
    m_sampleCount = sampleCount;
    m_sampleQuality = sampleQuality;
    m_isValid = false;
}

uint32_t VulkanPipelineState::GetSampleCount() const {
    return m_sampleCount;
}

uint32_t VulkanPipelineState::GetSampleQuality() const {
    return m_sampleQuality;
}

bool VulkanPipelineState::Create(IRenderDevice* device) {
    (void)device;
    // TODO: Implement actual Vulkan pipeline creation
    // This requires:
    // 1. Creating VkShaderModules from shaders
    // 2. Creating VkPipelineVertexInputStateCreateInfo from input layout
    // 3. Creating VkPipelineInputAssemblyStateCreateInfo from topology
    // 4. Creating VkPipelineRasterizationStateCreateInfo from rasterizer state
    // 5. Creating VkPipelineColorBlendStateCreateInfo from blend state
    // 6. Creating VkPipelineDepthStencilStateCreateInfo from depth stencil state
    // 7. Creating VkGraphicsPipelineCreateInfo and calling vkCreateGraphicsPipelines
    m_isValid = false;
    m_errors = "Pipeline creation not fully implemented";
    return false;
}

bool VulkanPipelineState::Recreate() {
    return Create(nullptr);
}

bool VulkanPipelineState::Validate(IRenderDevice* device, std::string& errors) const {
    (void)device;
    errors.clear();
    
    if (!HasShader(ShaderType::Vertex)) {
        errors = "Vertex shader is required";
        return false;
    }
    
    if (!HasShader(ShaderType::Pixel)) {
        errors = "Pixel shader is required";
        return false;
    }
    
    return true;
}

uint64_t VulkanPipelineState::GetCacheKey() const {
    // TODO: Implement proper cache key calculation
    return 0;
}

bool VulkanPipelineState::LoadFromCache(IRenderDevice* device, uint64_t cacheKey) {
    (void)device;
    (void)cacheKey;
    return false;
}

bool VulkanPipelineState::SaveToCache() const {
    return false;
}

std::unique_ptr<IPipelineState> VulkanPipelineState::Clone() const {
    auto clone = std::make_unique<VulkanPipelineState>();
    clone->m_type = m_type;
    clone->m_topology = m_topology;
    clone->m_blendState = m_blendState;
    clone->m_rasterizerState = m_rasterizerState;
    clone->m_depthStencilState = m_depthStencilState;
    clone->m_inputAttributes = m_inputAttributes;
    clone->m_renderTargetFormats = m_renderTargetFormats;
    clone->m_depthStencilFormat = m_depthStencilFormat;
    clone->m_sampleCount = m_sampleCount;
    clone->m_sampleQuality = m_sampleQuality;
    clone->m_shaders = m_shaders;
    clone->m_debugName = m_debugName;
    return clone;
}

} // namespace PrismaEngine::Graphic::Vulkan
