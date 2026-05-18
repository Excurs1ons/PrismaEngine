#include "VulkanComputePipeline.h"
#include "RenderDeviceVulkan.h"
#include "VulkanShader.h"
#include "VulkanResources.h"
#include "interfaces/ShaderReflection.h"
#include <vulkan/vulkan.h>
#include <unordered_map>

namespace Prisma::Graphic::Vulkan {

VulkanComputePipeline::VulkanComputePipeline() = default;

VulkanComputePipeline::~VulkanComputePipeline() {
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pipeline, nullptr);
    }
}

void VulkanComputePipeline::SetShader(std::shared_ptr<IShader> shader) {
    m_shader = std::move(shader);
}

void VulkanComputePipeline::SetPushConstantRange(uint32_t size, uint32_t offset) {
    m_pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    m_pushConstantRange.size = size;
    m_pushConstantRange.offset = offset;
}

bool VulkanComputePipeline::Create(IRenderDevice* device) {
    auto* vkDevice = dynamic_cast<RenderDeviceVulkan*>(device);
    if (!vkDevice) return false;
    m_device = vkDevice->GetVkDevice();

    if (!m_shader) return false;

    // Get SPIR-V from shader
    const auto& spirv = m_shader->GetBytecode();

    // Create shader module
    VkShaderModuleCreateInfo shaderCI{};
    shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCI.codeSize = spirv.size();
    shaderCI.pCode = reinterpret_cast<const uint32_t*>(spirv.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &shaderCI, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }

    // Build descriptor set layouts from shader reflection
    const auto& reflection = m_shader->GetReflection();
    std::vector<VkDescriptorSetLayout> vkLayouts;

    // Group resources by set (using the existing ShaderResource::Set field)
    std::unordered_map<uint32_t, std::vector<const ShaderResource*>> setResources;
    for (const auto& res : reflection.Resources) {
        setResources[res.Set].push_back(&res);
    }

    // Create descriptor set layouts
    for (auto& [set, resources] : setResources) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (const auto* res : resources) {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = res->Binding;
            binding.descriptorCount = res->Count > 0 ? res->Count : 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            // Map resource type to VkDescriptorType
            switch (res->ResourceType) {
                case ShaderResource::Type::UniformBuffer:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
                case ShaderResource::Type::StorageBuffer:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                case ShaderResource::Type::Sampler2D:
                case ShaderResource::Type::SamplerCube:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
                case ShaderResource::Type::Image2D:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    break;
                default:
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
            }
            bindings.push_back(binding);
        }

        VkDescriptorSetLayoutCreateInfo layoutCI{};
        layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCI.bindingCount = (uint32_t)bindings.size();
        layoutCI.pBindings = bindings.data();

        VkDescriptorSetLayout vkLayout;
        if (vkCreateDescriptorSetLayout(m_device, &layoutCI, nullptr, &vkLayout) != VK_SUCCESS) {
            vkDestroyShaderModule(m_device, shaderModule, nullptr);
            return false;
        }

        auto layout = std::make_shared<VulkanDescriptorSetLayout>(m_device, vkLayout);
        m_descriptorSetLayouts.push_back(layout);
        vkLayouts.push_back(vkLayout);
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutCI{};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.setLayoutCount = (uint32_t)vkLayouts.size();
    layoutCI.pSetLayouts = vkLayouts.data();
    if (m_pushConstantRange.size > 0) {
        layoutCI.pushConstantRangeCount = 1;
        layoutCI.pPushConstantRanges = &m_pushConstantRange;
    }

    if (vkCreatePipelineLayout(m_device, &layoutCI, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, shaderModule, nullptr);
        return false;
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCI.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineCI.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineCI.stage.module = shaderModule;
    pipelineCI.stage.pName = m_shader->GetEntryPoint().c_str();
    pipelineCI.layout = m_pipelineLayout;

    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, shaderModule, nullptr);
        return false;
    }

    // Shader module no longer needed after pipeline creation
    vkDestroyShaderModule(m_device, shaderModule, nullptr);

    return true;
}

} // namespace Prisma::Graphic::Vulkan
