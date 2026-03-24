#include "VulkanPipelineState.h"
#include "VulkanShader.h"
#include <filesystem>
#include <fstream>
#include <functional>

namespace Prisma::Graphic::Vulkan {

namespace fs = std::filesystem;

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
    if (device != nullptr) {
        m_device = device->GetVkDevice();
    }

    std::string validationErrors;
    if (!Validate(device, validationErrors)) {
        m_errors = std::move(validationErrors);
        m_isValid = false;
        return false;
    }

    if (m_device == VK_NULL_HANDLE) {
        m_errors = "Vulkan device is not available";
        m_isValid = false;
        return false;
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE && m_pipelineLayout) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        m_errors = "Failed to create Vulkan pipeline layout";
        m_isValid = false;
        return false;
    }

    m_errors.clear();
    m_isValid = true;
    SaveToCache();
    return true;
}

bool VulkanPipelineState::Recreate() {
    return Create(nullptr);
}

bool VulkanPipelineState::Validate(IRenderDevice* device, std::string& errors) const {
    errors.clear();
    if (device == nullptr && m_device == VK_NULL_HANDLE) {
        errors = "Render device is required";
        return false;
    }
    
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
    auto hashCombine = [](uint64_t& seed, uint64_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
    };

    uint64_t seed = 0;
    hashCombine(seed, static_cast<uint64_t>(m_type));
    hashCombine(seed, static_cast<uint64_t>(m_topology));
    hashCombine(seed, static_cast<uint64_t>(m_sampleCount));
    hashCombine(seed, static_cast<uint64_t>(m_sampleQuality));
    hashCombine(seed, static_cast<uint64_t>(m_depthStencilFormat));
    hashCombine(seed, static_cast<uint64_t>(m_blendState.blendEnable));
    hashCombine(seed, static_cast<uint64_t>(m_blendState.logicOpEnable));
    hashCombine(seed, static_cast<uint64_t>(m_blendState.writeMask));
    hashCombine(seed, static_cast<uint64_t>(m_rasterizerState.fillMode));
    hashCombine(seed, static_cast<uint64_t>(m_rasterizerState.cullMode));
    hashCombine(seed, static_cast<uint64_t>(m_depthStencilState.depthEnable));
    hashCombine(seed, static_cast<uint64_t>(m_depthStencilState.depthWriteEnable));
    hashCombine(seed, static_cast<uint64_t>(m_depthStencilState.depthFunc));

    for (const auto& attribute : m_inputAttributes) {
        hashCombine(seed, std::hash<std::string>{}(attribute.semanticName));
        hashCombine(seed, attribute.semanticIndex);
        hashCombine(seed, static_cast<uint64_t>(attribute.format));
        hashCombine(seed, attribute.inputSlot);
        hashCombine(seed, attribute.alignedByteOffset);
    }

    for (const auto format : m_renderTargetFormats) {
        hashCombine(seed, static_cast<uint64_t>(format));
    }

    for (const auto& [shaderType, shader] : m_shaders) {
        hashCombine(seed, static_cast<uint64_t>(shaderType));
        hashCombine(seed, reinterpret_cast<uint64_t>(shader.get()));
    }

    return seed;
}

bool VulkanPipelineState::LoadFromCache(IRenderDevice* device, uint64_t cacheKey) {
    if (device != nullptr && m_device == VK_NULL_HANDLE) {
        m_device = device->GetVkDevice();
    }
    if (m_device == VK_NULL_HANDLE) {
        return false;
    }

    const auto cachePath = fs::path(".pipeline_cache") / (std::to_string(cacheKey) + ".cache");
    if (!fs::exists(cachePath)) {
        return false;
    }

    std::ifstream stream(cachePath, std::ios::binary);
    if (!stream.is_open()) {
        return false;
    }

    uint64_t storedKey = 0;
    stream.read(reinterpret_cast<char*>(&storedKey), sizeof(storedKey));
    if (!stream || storedKey != cacheKey) {
        return false;
    }

    uint32_t shaderCount = 0;
    stream.read(reinterpret_cast<char*>(&shaderCount), sizeof(shaderCount));
    stream.read(reinterpret_cast<char*>(&m_sampleCount), sizeof(m_sampleCount));

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (m_pipelineLayout == VK_NULL_HANDLE &&
        vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        return false;
    }

    m_isValid = true;
    m_errors.clear();
    return true;
}

bool VulkanPipelineState::SaveToCache() const {
    fs::create_directories(".pipeline_cache");
    const auto cacheKey = GetCacheKey();
    const auto cachePath = fs::path(".pipeline_cache") / (std::to_string(cacheKey) + ".cache");
    std::ofstream stream(cachePath, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }

    uint32_t shaderCount = static_cast<uint32_t>(m_shaders.size());
    stream.write(reinterpret_cast<const char*>(&cacheKey), sizeof(cacheKey));
    stream.write(reinterpret_cast<const char*>(&shaderCount), sizeof(shaderCount));
    stream.write(reinterpret_cast<const char*>(&m_sampleCount), sizeof(m_sampleCount));
    return stream.good();
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
    clone->m_isValid = m_isValid;
    clone->m_errors = m_errors;
    return clone;
}

} // namespace Prisma::Graphic::Vulkan
