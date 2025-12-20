#include "ShaderFactory.h"
#include "adapters/dx12/DX12Shader.h"
#include "adapters/dx12/DX12RenderDevice.h"
#include <fstream>
#include <sstream>

namespace PrismaEngine::Graphic {

std::unique_ptr<IShader> ShaderFactory::CreateShader(
    RenderBackendType backendType,
    const std::string& sourceCode,
    const ShaderDesc& desc) {
    
    switch (backendType) {
        case RenderBackendType::DirectX12:
            // 在实际实现中，这里需要编译HLSL源码为DX12字节码
            // 并创建DX12Shader实例
            // 注意：这需要一个有效的DX12RenderDevice实例
            // 示例伪代码：
            // auto bytecode = CompileHLSLToDX12(sourceCode, desc);
            // auto reflection = ReflectDX12Shader(bytecode);
            // return std::make_unique<DX12Shader>(device, desc, bytecode, reflection);
            break;
            
        case RenderBackendType::Vulkan:
            // 为Vulkan着色器预留实现空间
            // 示例伪代码：
            // auto spirv = CompileGLSLToSPIRV(sourceCode, desc);
            // return std::make_unique<VulkanShader>(spirv, desc);
            break;
            
        default:
            break;
    }
    
    return nullptr;
}

std::unique_ptr<IShader> ShaderFactory::CreateShaderFromFile(
    RenderBackendType backendType,
    const std::string& filepath,
    const ShaderDesc& desc) {
    
    // 读取文件内容
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return nullptr;
    }
    
    std::stringstream sourceStream;
    sourceStream << file.rdbuf();
    std::string sourceCode = sourceStream.str();
    file.close();
    
    return CreateShader(backendType, sourceCode, desc);
}

} // namespace PrismaEngine::Graphic