#include "Material.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IBuffer.h"
#include "RenderResourceManager.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "logger/Logger.h"
#include <fstream>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>

namespace Prisma::Graphic {

Material::Material(std::shared_ptr<IShader> shader) : m_Shader(std::move(shader)) {
    m_IsLoaded = (m_Shader != nullptr);
}

bool Material::Load(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Material", "无法打开材质文件: {0}", path.string());
        return false;
    }

    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    glz::json_t root;
    auto ec = glz::read_json(root, jsonStr);
    if (ec) {
        LOG_ERROR("Material", "解析材质文件 {0} 失败", path.string());
        return false;
    }

    m_Params.clear();
    SetPath(path);

    auto& rootObj = root.get_object();
    if (rootObj.contains("name") && rootObj.at("name").is_string()) {
        SetName(rootObj.at("name").get_string());
    } else {
        SetName(path.stem().string());
    }

    if (rootObj.contains("shader") && rootObj.at("shader").is_string()) {
        const std::string shaderName = rootObj.at("shader").get_string();
        auto resourceManager = Engine::Get().GetRenderResourceManager();
        if (resourceManager) {
            auto shaderRes = resourceManager->LoadShaderSync(shaderName);
            if (shaderRes) {
                m_Shader = shaderRes;
            }
        }
    }

    if (rootObj.contains("properties") && rootObj.at("properties").is_object()) {
        const auto& properties = rootObj.at("properties").get_object();

        if (properties.contains("basecolor") && properties.at("basecolor").is_array() && properties.at("basecolor").get_array().size() >= 4) {
            auto& albedoArr = properties.at("basecolor").get_array();
            SetBaseColor(
                static_cast<float>(albedoArr[0].get_number()),
                static_cast<float>(albedoArr[1].get_number()),
                static_cast<float>(albedoArr[2].get_number()),
                static_cast<float>(albedoArr[3].get_number()));
            }
            if (properties.contains("metallic")) {
            SetMetallic(static_cast<float>(properties.at("metallic").get_number()));
            }
            if (properties.contains("roughness")) {
            SetRoughness(static_cast<float>(properties.at("roughness").get_number()));
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
    auto resourceManager = Engine::Get().GetRenderResourceManager();
    std::shared_ptr<IShader> defaultShader = nullptr;
    if (resourceManager) {
        defaultShader = resourceManager->LoadShaderSync("Default");
    }
    
    auto material = std::make_shared<Material>(defaultShader);
    material->m_IsLoaded = true;
    material->SetBaseColor(1.0f, 1.0f, 1.0f, 1.0f);
    return material;
}

void Material::SetBaseColor(float r, float g, float b, float a) {
    SetParam("BaseColor", PrismaMath::vec4(r, g, b, a));
}

void Material::SetBaseColor(const Prisma::Color& color) {
    SetParam("BaseColor", PrismaMath::vec4(color.r, color.g, color.b, color.a));
}

void Material::SetMetallic(float metallic) {
    SetParam("Metallic", metallic);
}

void Material::SetRoughness(float roughness) {
    SetParam("Roughness", roughness);
}

void Material::Bind(ICommandBuffer* cmd) {
    if (!cmd || !m_Shader) return;

    UpdateDescriptorSet();

    // 每次 Bind 都更新一次数据，确保动态修改生效
    if (m_MaterialUBO) {
        MaterialData data{};
        if (auto* val = GetParam("BaseColor")) {
            if (std::holds_alternative<PrismaMath::vec4>(*val))
                data.baseColor = std::get<PrismaMath::vec4>(*val);
        }
        if (auto* val = GetParam("Metallic")) {
            if (std::holds_alternative<float>(*val))
                data.metallic = std::get<float>(*val);
        }
        if (auto* val = GetParam("Roughness")) {
            if (std::holds_alternative<float>(*val))
                data.roughness = std::get<float>(*val);
        }
        m_MaterialUBO->UpdateData(&data, sizeof(data), 0);
    }

    if (m_DescriptorSet) {
        cmd->BindDescriptorSet(0, m_DescriptorSet.get());
    }
}

void Material::UpdateDescriptorSet() {
    if (m_DescriptorSet) return;

    auto* rf = Engine::Get().GetRenderSystem()->GetDevice()->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();

    // 1. 创建材质 UBO
    BufferDesc uboDesc;
    uboDesc.type = BufferType::Constant;
    uboDesc.size = sizeof(MaterialData);
    uboDesc.usage = BufferUsage::Dynamic;
    m_MaterialUBO = rf->CreateBufferImpl(uboDesc);

    // 2. 收集资源并创建 Layout
    // 强制匹配 clustered_forward.frag 的预期布局：
    // Binding 0: MaterialData (UBO)
    // Binding 1: AlbedoMap (Sampler2D)
    std::vector<ShaderResource> shaderResources;
    
    ShaderResource uboRes;
    uboRes.Name = "MaterialData";
    uboRes.ResourceType = ShaderResource::Type::UniformBuffer;
    uboRes.Set = 0;
    uboRes.Binding = 0;
    shaderResources.push_back(uboRes);

    ShaderResource texRes;
    texRes.Name = "AlbedoMap";
    texRes.ResourceType = ShaderResource::Type::Sampler2D;
    texRes.Set = 0;
    texRes.Binding = 1;
    shaderResources.push_back(texRes);

    m_DescriptorSetLayout = rf->CreateDescriptorSetLayout(shaderResources);
    m_DescriptorSet = rf->CreateDescriptorSet(m_DescriptorSetLayout.get());

    if (m_DescriptorSet) {
        m_DescriptorSet->BindBuffer(0, m_MaterialUBO.get(), 0, sizeof(MaterialData), DescriptorType::UniformBuffer);

        auto defaultSampler = rm->GetDefaultSampler();
        std::shared_ptr<ITexture> texture = nullptr;

        // 尝试获取用户设置的纹理
        if (auto* val = GetParam("AlbedoMap")) {
            if (std::holds_alternative<std::shared_ptr<ITexture>>(*val))
                texture = std::get<std::shared_ptr<ITexture>>(*val);
        }

        // [核心修复] 如果没有贴图，使用一个默认的 1x1 白色贴图作为兜底
        if (!texture) {
             uint32_t white = 0xFFFFFFFF;
             TextureDesc desc;
             desc.width = 1;
             desc.height = 1;
             desc.format = TextureFormat::RGBA8_UNorm;
             texture = rm->CreateTextureFromMemory(&white, sizeof(white), desc);
        }

        if (texture) {
            m_DescriptorSet->BindTexture(1, texture.get(), defaultSampler.get());
        }
        
        m_DescriptorSet->Update();
    }
}

} // namespace Prisma::Graphic
