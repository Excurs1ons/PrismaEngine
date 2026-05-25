#pragma once

#include "interfaces/ShaderReflection.h"
#include <vector>
#include <cstdint>

namespace Prisma::Graphic {

/**
 * @brief SPIR-V 反射解析器
 *
 * 封装 SPIRV-Reflect 库，自动从 SPIR-V 字节码中提取
 * descriptor set binding 信息，取代手动维护的 if/else 反射链。
 *
 * 用法:
 *   ShaderReflection reflection;
 *   if (SpirvReflector::Reflect(bytecode, reflection)) {
 *       // reflection.Resources 已包含所有 binding 信息
 *   }
 */
class SpirvReflector {
public:
    /**
     * @brief 从 SPIR-V 字节码提取反射信息
     */
    static bool Reflect(const std::vector<uint8_t>& bytecode, ShaderReflection& outReflection);

    /**
     * @brief 从原始指针提取反射信息
     */
    static bool Reflect(const void* data, size_t size, ShaderReflection& outReflection);
};

} // namespace Prisma::Graphic
