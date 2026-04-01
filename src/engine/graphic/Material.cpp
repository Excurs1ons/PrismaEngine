#include "Material.h"
#include "Shader.h"
#include "interfaces/ICommandBuffer.h"
#include "Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace Prisma::Graphic {

Material::Material(std::shared_ptr<Shader> shader) : m_Shader(std::move(shader)) {
    m_IsLoaded = (m_Shader != nullptr);
}

bool Material::Load(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Material", "无法打开材质文件: {0}", path.string());
        return false;
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const std::exception& ex) {
        LOG_ERROR("Material", "解析材质文件 {0} 失败: {1}", path.string(), ex.what());
        return false;
    }

    m_Params.clear();
    SetPath(path);

    if (root.contains("name") && root["name"].is_string()) {
        SetName(root["name"].get<std::string>());
    } else {
        SetName(path.stem().string());
    }

    if (root.contains("shader") && root["shader"].is_string()) {
        auto shader = std::make_shared<Shader>();
        const std::string shaderName = root["shader"].get<std::string>();
        shader->SetName(shaderName);
        shader->SetPath(shaderName);
        if (shader->Load(shaderName)) {
            m_Shader = std::move(shader);
        }
    }

    if (root.contains("properties") && root["properties"].is_object()) {
        const auto& properties = root["properties"];

        if (properties.contains("albedo") && properties["albedo"].is_array() && properties["albedo"].size() >= 4) {
            SetBaseColor(
                properties["albedo"][0].get<float>(),
                properties["albedo"][1].get<float>(),
                properties["albedo"][2].get<float>(),
                properties["albedo"][3].get<float>());
        }
        if (properties.contains("metallic")) {
            SetMetallic(properties["metallic"].get<float>());
        }
        if (properties.contains("roughness")) {
            SetRoughness(properties["roughness"].get<float>());
        }
        if (properties.contains("emissive") && properties["emissive"].is_array() && properties["emissive"].size() >= 3) {
            SetParam("Emissive", PrismaMath::vec3(
                properties["emissive"][0].get<float>(),
                properties["emissive"][1].get<float>(),
                properties["emissive"][2].get<float>()));
        }
        if (properties.contains("emissiveIntensity")) {
            SetParam("EmissiveIntensity", properties["emissiveIntensity"].get<float>());
        }
        if (properties.contains("tiling") && properties["tiling"].is_array() && properties["tiling"].size() >= 2) {
            SetParam("Tiling", PrismaMath::vec4(
                properties["tiling"][0].get<float>(),
                properties["tiling"][1].get<float>(),
                0.0f,
                0.0f));
        }
        if (properties.contains("offset") && properties["offset"].is_array() && properties["offset"].size() >= 2) {
            SetParam("Offset", PrismaMath::vec4(
                properties["offset"][0].get<float>(),
                properties["offset"][1].get<float>(),
                0.0f,
                0.0f));
        }
    }

    m_IsLoaded = true;
    return true;
}

void Material::Unload() {
    m_Shader = nullptr;
    m_Params.clear();
    m_IsLoaded = false;
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
    auto material = std::make_shared<Material>(nullptr);
    material->m_IsLoaded = true;
    return material;
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
            LOG_WARN("Material", "着色器绑定缺失所需参数: {0}。", resource.Name);
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
