#include "SpirvReflector.h"
#include <spirv_reflect.h>
#include <cstring>

namespace Prisma::Graphic {

static ShaderResource::Type MapDescriptorType(SpvReflectDescriptorType descType, const SpvReflectImageTraits* imageTraits) {
    switch (descType) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return ShaderResource::Type::UniformBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return ShaderResource::Type::StorageBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            if (imageTraits && imageTraits->dim == SpvDimCube) {
                return ShaderResource::Type::SamplerCube;
            }
            return ShaderResource::Type::Sampler2D;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return ShaderResource::Type::Image2D;
        default:
            return ShaderResource::Type::Sampler2D; // fallback
    }
}

bool SpirvReflector::Reflect(const std::vector<uint8_t>& bytecode, ShaderReflection& outReflection) {
    if (bytecode.empty()) return false;
    return Reflect(bytecode.data(), bytecode.size(), outReflection);
}

bool SpirvReflector::Reflect(const void* data, size_t size, ShaderReflection& outReflection) {
    if (!data || size == 0 || size % 4 != 0) return false;

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(size, data, &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) return false;

    // ── 枚举 Descriptor Bindings ──
    uint32_t bindingCount = 0;
    spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
    if (bindingCount > 0) {
        std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);
        spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());

        for (uint32_t i = 0; i < bindingCount; ++i) {
            const auto* b = bindings[i];
            ShaderResource res;
            res.Name = b->name ? b->name : "";
            res.Set = b->set;
            res.Binding = b->binding;
            res.Count = b->count;
            bool isImage = (b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                            b->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
            res.ResourceType = MapDescriptorType(b->descriptor_type, isImage ? &b->image : nullptr);
            res.Size = b->block.size;
            outReflection.Resources.push_back(res);
        }
    }

    // ── 枚举 Push Constant Blocks ──
    uint32_t pcCount = 0;
    spvReflectEnumeratePushConstantBlocks(&module, &pcCount, nullptr);
    if (pcCount > 0) {
        std::vector<SpvReflectBlockVariable*> pushConstants(pcCount);
        spvReflectEnumeratePushConstantBlocks(&module, &pcCount, pushConstants.data());

        for (uint32_t i = 0; i < pcCount; ++i) {
            outReflection.PushConstantRanges.push_back(pushConstants[i]->size);
        }
    }

    spvReflectDestroyShaderModule(&module);
    return true;
}

} // namespace Prisma::Graphic
