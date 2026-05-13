#include "VulkanPipelineState.h"
#include "VulkanShader.h"
#include "RenderDeviceVulkan.h"
#include "VulkanSwapChain.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <array>

namespace Prisma::Graphic::Vulkan {

namespace fs = std::filesystem;

// ============================================================
// 辅助函数：BlendFactorType → VkBlendFactor
// ============================================================
static VkBlendFactor ToVkBlendFactor(BlendFactorType factor) {
    switch (factor) {
        case BlendFactorType::Zero:         return VK_BLEND_FACTOR_ZERO;
        case BlendFactorType::One:          return VK_BLEND_FACTOR_ONE;
        case BlendFactorType::SrcColor:     return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactorType::InvSrcColor:  return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactorType::SrcAlpha:     return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactorType::InvSrcAlpha:  return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactorType::DstAlpha:     return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactorType::InvDstAlpha:  return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactorType::DstColor:     return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactorType::InvDstColor:  return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactorType::SrcAlphaSat:  return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case BlendFactorType::BlendFactor:  return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case BlendFactorType::InvBlendFactor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactorType::Src1Color:    return VK_BLEND_FACTOR_SRC1_COLOR;
        case BlendFactorType::InvSrc1Color: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case BlendFactorType::Src1Alpha:    return VK_BLEND_FACTOR_SRC1_ALPHA;
        case BlendFactorType::InvSrc1Alpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:                            return VK_BLEND_FACTOR_ONE;
    }
}

// ============================================================
// 辅助函数：BlendOp → VkBlendOp
// ============================================================
static VkBlendOp ToVkBlendOp(BlendOp op) {
    switch (op) {
        case BlendOp::Add:         return VK_BLEND_OP_ADD;
        case BlendOp::Subtract:    return VK_BLEND_OP_SUBTRACT;
        case BlendOp::RevSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min:         return VK_BLEND_OP_MIN;
        case BlendOp::Max:         return VK_BLEND_OP_MAX;
        default:                   return VK_BLEND_OP_ADD;
    }
}

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

    auto deviceVulkan = dynamic_cast<RenderDeviceVulkan*>(Engine::Get().GetRenderSystem()->GetDevice());

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

    // [修复] 配置 Push Constant Range 供 2D 变换使用
    std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandles;
    std::vector<VkPushConstantRange> pushConstantRanges;

    // 默认推流常量 (mat4 MVP + vec4 Color)
    VkPushConstantRange defaultPushConstant{};
    defaultPushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    defaultPushConstant.offset = 0;
    defaultPushConstant.size = 128; 
    pushConstantRanges.push_back(defaultPushConstant);

    m_descriptorSetLayouts.clear();
    for (const auto& [type, shaderRes] : m_shaders) {
        auto vkShader = std::dynamic_pointer_cast<VulkanShader>(shaderRes);
        if (!vkShader) continue;

        const auto& reflection = vkShader->GetReflection();
        
        // 处理 Descriptor Sets (目前简化，仅处理 Set 0)
        if (!reflection.Resources.empty()) {
            auto layout = deviceVulkan->GetResourceFactory()->CreateDescriptorSetLayout(reflection.Resources);
            if (layout) {
                m_descriptorSetLayouts.push_back(layout);
                descriptorSetLayoutHandles.push_back((VkDescriptorSetLayout)layout->GetNativeHandle());
            }
        }
    }

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges.data();
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayoutHandles.size());
    layoutInfo.pSetLayouts = descriptorSetLayoutHandles.data();
    
    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        m_errors = "Failed to create Vulkan pipeline layout";
        m_isValid = false;
        return false;
    }

    // [补全] 创建图形管线核心逻辑
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const auto& [type, shaderRes] : m_shaders) {
        auto vkShader = std::dynamic_pointer_cast<VulkanShader>(shaderRes);
        if (vkShader) {
            VkPipelineShaderStageCreateInfo stageInfo{};
            stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageInfo.stage = (type == ShaderType::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
            stageInfo.module = vkShader->GetShaderModule();
            stageInfo.pName = "main";
            shaderStages.push_back(stageInfo);
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    // 默认使用 Prisma::Vertex 布局
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Prisma::Graphic::Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Prisma::Graphic::Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Prisma::Graphic::Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Prisma::Graphic::Vertex, uv);

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // [修复] 从 m_blendState 读取混合状态，不再硬编码 alpha blending
    // 此前硬编码为 srcAlpha / oneMinusSrcAlpha，导致 additive blending（如 2D 光照叠加）失效
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = m_blendState.blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.colorWriteMask = m_blendState.writeMask; // 0xF = RGBA，与 VK 常量值一致
    colorBlendAttachment.srcColorBlendFactor = ToVkBlendFactor(m_blendState.srcBlend);
    colorBlendAttachment.dstColorBlendFactor = ToVkBlendFactor(m_blendState.destBlend);
    colorBlendAttachment.colorBlendOp = ToVkBlendOp(m_blendState.blendOp);
    colorBlendAttachment.srcAlphaBlendFactor = ToVkBlendFactor(m_blendState.srcBlendAlpha);
    colorBlendAttachment.dstAlphaBlendFactor = ToVkBlendFactor(m_blendState.destBlendAlpha);
    colorBlendAttachment.alphaBlendOp = ToVkBlendOp(m_blendState.blendOpAlpha);

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 需要一个有效的 RenderPass
    if (!deviceVulkan || !deviceVulkan->GetSwapChain()) {
        m_errors = "RenderPass required for pipeline creation";
        return false;
    }

    auto vkSwapChain = dynamic_cast<VulkanSwapChain*>(deviceVulkan->GetSwapChain());
    if (!vkSwapChain) {
        m_errors = "VulkanSwapChain required for pipeline creation";
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = vkSwapChain->GetRenderPass();
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        m_errors = "Failed to create graphics pipeline";
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
