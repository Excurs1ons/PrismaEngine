#pragma once

#include "interfaces/IShader.h"
#include "interfaces/RenderTypes.h"
#include <memory>
#include <string>

namespace PrismaEngine::Graphic {

class ShaderFactory {
public:
    /// @brief 根据指定的渲染后端类型创建着色器
    /// @param backendType 渲染后端类型 (DX12, Vulkan等)
    /// @param sourceCode 着色器源码
    /// @param desc 着色器描述信息
    /// @return 创建的着色器实例
    static std::unique_ptr<IShader> CreateShader(
        RenderBackendType backendType,
        const std::string& sourceCode,
        const ShaderDesc& desc);

    /// @brief 根据文件创建着色器
    /// @param backendType 渲染后端类型 (DX12, Vulkan等)
    /// @param filepath 着色器文件路径
    /// @param desc 着色器描述信息
    /// @return 创建的着色器实例
    static std::unique_ptr<IShader> CreateShaderFromFile(
        RenderBackendType backendType,
        const std::string& filepath,
        const ShaderDesc& desc);
};

} // namespace PrismaEngine::Graphic