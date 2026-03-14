#include "ShaderFactory.h"
#include "interfaces/IResourceFactory.h"
#include "Logger.h"

namespace Prisma::Graphic {

std::shared_ptr<IShader> ShaderFactory::CreateShader(IRenderDevice* device, const ShaderDesc& desc) {
    if (!device) return nullptr;
    
    auto* factory = device->GetResourceFactory();
    if (!factory) return nullptr;

    // Use dummy bytecode and reflection for now
    std::vector<uint8_t> bytecode;
    ShaderReflection reflection;
    
    return factory->CreateShaderImpl(desc, bytecode, reflection);
}

std::shared_ptr<IShader> ShaderFactory::CreateShaderFromFile(IRenderDevice* device, const std::string& path, ShaderType type) {
    ShaderDesc desc;
    desc.filename = path;
    desc.type = type;
    return CreateShader(device, desc);
}

} // namespace Prisma::Graphic
