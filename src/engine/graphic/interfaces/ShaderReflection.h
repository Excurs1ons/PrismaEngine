#pragma once

#include "math/MathTypes.h"
#include <string>
#include <vector>

namespace Prisma::Graphic {

/**
 * @brief 着色器资源反射信息 (Descriptor Set Binding)
 */
struct ShaderResource {
    enum class Type { UniformBuffer, StorageBuffer, Sampler2D, SamplerCube, Image2D };
    
    std::string Name;
    Type ResourceType;
    uint32_t Set;
    uint32_t Binding;
    uint32_t Count;
    uint32_t Size; // 仅对 Buffer 有效
};

/**
 * @brief 完整的着色器反射信息
 */
struct ShaderReflection {
    std::vector<ShaderResource> Resources;
    std::vector<uint32_t> PushConstantRanges; // 简单的偏移量列表
};

} // namespace Prisma::Graphic
