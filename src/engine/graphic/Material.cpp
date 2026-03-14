#include "Material.h"
#include "Shader.h"
#include "interfaces/ICommandBuffer.h"
#include "Logger.h"

namespace Prisma::Graphic {

Material::Material(std::shared_ptr<Shader> shader) : m_Shader(std::move(shader)) {
}

bool Material::Load(const std::filesystem::path& path) {
    (void)path;
    // TODO: 从 .mat 文件加载材质数据
    return true;
}

void Material::Unload() {
    m_Shader = nullptr;
    m_Params.clear();
}

void Material::SetParam(const std::string& name, const MaterialParamValue& value) {
    m_Params[name] = value;
}

const MaterialParamValue* Material::GetParam(const std::string& name) const {
    auto it = m_Params.find(name);
    return it != m_Params.end() ? &it->second : nullptr;
}

std::shared_ptr<Material> Material::CreateDefault() {
    // 默认创建一个不带 Shader 的材质 (或者应该找一个内置的默认 Shader)
    return std::make_shared<Material>(nullptr);
}

void Material::SetBaseColor(float r, float g, float b, float a) {
    SetParam("BaseColor", Prisma::Color(r, g, b, a));
}

void Material::SetBaseColor(const Prisma::Color& color) {
    SetParam("BaseColor", color);
}

void Material::SetMetallic(float metallic) {
    SetParam("Metallic", metallic);
}

void Material::SetRoughness(float roughness) {
    SetParam("Roughness", roughness);
}

void Material::Bind(class ICommandBuffer* cmd) {
    if (!m_Shader || !cmd) return;

    // 1. 核心逻辑：基于反射自动绑定参数
    const auto& reflection = m_Shader->GetReflection();
    
    // 遍历 Shader 需要的所有资源
    for (const auto& resource : reflection.Resources) {
        // 在材质参数映射表中找同名资源
        auto it = m_Params.find(resource.Name);
        if (it == m_Params.end()) {
            // 记录缺失的参数，方便调试
            LOG_WARN("Material", "Missing required parameter: {0} for shader binding.", resource.Name);
            continue;
        }

        // 2. 根据反射信息进行资源的分发
        const auto& value = it->second;
        
        switch (resource.ResourceType) {
            case ShaderResource::Type::Sampler2D:
            case ShaderResource::Type::SamplerCube:
            case ShaderResource::Type::Image2D: {
                // 如果参数是一个贴图
                if (std::holds_alternative<std::shared_ptr<ITexture>>(value)) {
                    // auto texture = std::get<std::shared_ptr<ITexture>>(value);
                    // cmd->BindTexture(resource.Set, resource.Binding, texture.get());
                }
                break;
            }
            case ShaderResource::Type::UniformBuffer: {
                // 如果参数是基础数值 (float, vec3, vec4)
                break;
            }
            default:
                break;
        }
    }

    // 3. 提交底层 Descriptor Set
    // cmd->BindDescriptorSet(1, m_DescriptorSetHandle);
}

} // namespace Prisma::Graphic
