#include "Shader.h"
#include "Logger.h"

namespace Prisma::Graphic {

bool Shader::Load(const std::filesystem::path& path) {
    // 1. 加载 SPIR-V 字节码 (这里简化处理，实际应读取文件)
    LOG_INFO("Shader", "Loading shader bytecode from: {0}", path.string());
    
    // 2. 模拟反射信息填充 (实际应使用 spirv-cross 等工具自动生成)
    // 假设这是一个基础的 PBR Shader
    ShaderResource albedo;
    albedo.Name = "u_Albedo";
    albedo.ResourceType = ShaderResource::Type::Sampler2D;
    albedo.Set = 1;
    albedo.Binding = 0;
    m_Reflection.Resources.push_back(albedo);
    
    ShaderResource params;
    params.Name = "u_MaterialParams";
    params.ResourceType = ShaderResource::Type::UniformBuffer;
    params.Set = 1;
    params.Binding = 1;
    params.Size = sizeof(float) * 4; // metallic, roughness, etc.
    m_Reflection.Resources.push_back(params);

    // 3. 构建快速查找映射
    for (uint32_t i = 0; i < (uint32_t)m_Reflection.Resources.size(); ++i) {
        m_ResourceMap[m_Reflection.Resources[i].Name] = i;
    }

    return true;
}

void Shader::Unload() {
    m_Bytecode.clear();
    m_Reflection.Resources.clear();
    m_ResourceMap.clear();
}

const ShaderResource* Shader::FindResource(const std::string& name) const {
    auto it = m_ResourceMap.find(name);
    if (it != m_ResourceMap.end()) {
        return &m_Reflection.Resources[it->second];
    }
    return nullptr;
}

} // namespace Prisma::Graphic
