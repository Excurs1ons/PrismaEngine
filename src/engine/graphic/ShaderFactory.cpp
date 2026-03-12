#include "ShaderFactory.h"
#include <fstream>
#include <sstream>

namespace PrismaEngine::Graphic {

std::unique_ptr<IShader> ShaderFactory::CreateShader(
    RenderAPIType backendType,
    const std::string& sourceCode,
    const ShaderDesc& desc) {
    return nullptr;
}

std::unique_ptr<IShader> ShaderFactory::CreateShaderFromFile(
    RenderAPIType backendType,
    const std::string& filepath,
    const ShaderDesc& desc) {
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
