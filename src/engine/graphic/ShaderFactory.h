#pragma once

#include "interfaces/IRenderDevice.h"
#include "interfaces/IShader.h"
#include "RenderDesc.h"
#include <string>
#include <memory>

namespace Prisma::Graphic {

/**
 * @brief 着色器工厂类
 */
class ShaderFactory {
public:
    /**
     * @brief 创建着色器
     */
    static std::shared_ptr<IShader> CreateShader(IRenderDevice* device, const ShaderDesc& desc);

    /**
     * @brief 从文件创建着色器
     */
    static std::shared_ptr<IShader> CreateShaderFromFile(IRenderDevice* device, const std::string& path, ShaderType type);
};

} // namespace Prisma::Graphic
