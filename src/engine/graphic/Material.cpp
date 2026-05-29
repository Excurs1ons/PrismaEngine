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
            if (properties.contains("ao")) {
            SetAO(static_cast<float>(properties.at("ao").get_number()));
            }
            if (properties.contains("emissive") && properties.at("emissive").is_array() && properties.at("emissive").get_array().size() >= 3) {
            auto& emissiveArr = properties.at("emissive").get_array();
            SetEmissiveColor(PrismaMath::vec3(
                static_cast<float>(emissiveArr[0].get_number()),
                static_cast<float>(emissiveArr[1].get_number()),
                static_cast<float>(emissiveArr[2].get_number())));
            }
            if (properties.contains("emissiveIntensity")) {
            SetEmissiveIntensity(static_cast<float>(properties.at("emissiveIntensity").get_number()));
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
    material->m_materialType = MaterialType::Unlit;
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

void Material::SetAO(float ao) {
    SetParam("AO", ao);
}

void Material::SetEmissiveIntensity(float intensity) {
    SetParam("EmissiveIntensity", intensity);
}

void Material::SetEmissiveColor(const PrismaMath::vec3& color) {
    SetParam("EmissiveColor", color);
}

void Material::SetAlbedoMap(std::shared_ptr<ITexture> texture) {
    m_albedoMap = texture;
    SetParam("AlbedoMap", texture);
}

void Material::SetNormalMap(std::shared_ptr<ITexture> texture) {
    m_normalMap = texture;
    SetParam("NormalMap", texture);
}

void Material::SetMetallicRoughnessMap(std::shared_ptr<ITexture> texture) {
    m_metallicRoughnessMap = texture;
    SetParam("MetallicRoughnessMap", texture);
}

void Material::SetAOMap(std::shared_ptr<ITexture> texture) {
    m_aoMap = texture;
    SetParam("AOMap", texture);
}

void Material::SetEmissiveMap(std::shared_ptr<ITexture> texture) {
    m_emissiveMap = texture;
    SetParam("EmissiveMap", texture);
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
        if (auto* val = GetParam("AO")) {
            if (std::holds_alternative<float>(*val))
                data.ao = std::get<float>(*val);
        }
        if (auto* val = GetParam("EmissiveIntensity")) {
            if (std::holds_alternative<float>(*val))
                data.emissiveIntensity = std::get<float>(*val);
        }
        m_MaterialUBO->UpdateData(&data, sizeof(data), 0);
    }

    if (m_DescriptorSet) {
        cmd->BindDescriptorSet(0, m_DescriptorSet.get());
    }
}

std::shared_ptr<Material> Material::CreatePBR() {
    auto resourceManager = Engine::Get().GetRenderResourceManager();
    std::shared_ptr<IShader> pbrShader = nullptr;
    if (resourceManager) {
        pbrShader = resourceManager->LoadShaderSync("assets/shaders/pbr_lit.frag.spv");
        if (!pbrShader) {
            pbrShader = resourceManager->LoadShaderSync("Default");
        }
    }

    auto material = std::make_shared<Material>(pbrShader);
    material->m_IsLoaded = true;
    material->SetBaseColor(1.0f, 1.0f, 1.0f, 1.0f);
    material->SetMetallic(0.0f);
    material->SetRoughness(0.5f);
    material->SetAO(1.0f);
    material->SetEmissiveIntensity(0.0f);
    material->m_materialType = MaterialType::PBR;
    return material;
}

std::shared_ptr<Material> Material::CreateNPR() {
    auto resourceManager = Engine::Get().GetRenderResourceManager();
    std::shared_ptr<IShader> nprShader = nullptr;
    if (resourceManager) {
        nprShader = resourceManager->LoadShaderSync("assets/shaders/npr_lit.vert.spv", "main");
        if (!nprShader) {
            nprShader = resourceManager->LoadShaderSync("Default");
        }
    }

    auto material = std::make_shared<Material>(nprShader);
    material->m_IsLoaded = true;
    material->SetBaseColor(1.0f, 1.0f, 1.0f, 1.0f);
    material->SetRoughness(0.4f);
    material->SetEmissiveIntensity(0.0f);
    // NPR 默认值
    material->SetParam("RimColor", PrismaMath::vec4(1.0f, 0.6f, 0.3f, 0.5f));
    material->SetParam("RimPower", 2.0f);
    material->SetParam("WrapAmount", 0.2f);
    material->m_materialType = MaterialType::NPR;
    return material;
}

void Material::UpdateDescriptorSet() {
    if (m_DescriptorSet) return;

    auto* rf = Engine::Get().GetRenderSystem()->GetDevice()->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();

    // 创建材质 UBO
    BufferDesc uboDesc;
    uboDesc.type = BufferType::Constant;
    uboDesc.size = sizeof(MaterialData);
    uboDesc.usage = BufferUsage::Dynamic;
    m_MaterialUBO = rf->CreateBufferImpl(uboDesc);

    // 检测是否有 PBR 纹理贴图设置，选择对应的描述符布局
    auto* albedoVal = GetParam("AlbedoMap");
    auto* normalVal = GetParam("NormalMap");
    auto* mrVal = GetParam("MetallicRoughnessMap");
    bool hasPBRTextures = (albedoVal && std::holds_alternative<std::shared_ptr<ITexture>>(*albedoVal)) ||
                          (normalVal && std::holds_alternative<std::shared_ptr<ITexture>>(*normalVal));

    if (hasPBRTextures) {
        UpdateDescriptorSetPBR();
        return;
    }

    // 基本布局: Binding 0 = MaterialData (UBO), Binding 1 = AlbedoMap (Sampler2D)
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

        if (albedoVal && std::holds_alternative<std::shared_ptr<ITexture>>(*albedoVal))
            texture = std::get<std::shared_ptr<ITexture>>(*albedoVal);

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

void Material::UpdateDescriptorSetPBR() {
    auto* rf = Engine::Get().GetRenderSystem()->GetDevice()->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    auto defaultSampler = rm->GetDefaultSampler();

    // PBR 布局: Set 0, Binding 0-5
    // Binding 0: MaterialData (UBO)
    // Binding 1: AlbedoMap (Sampler2D)
    // Binding 2: NormalMap (Sampler2D)
    // Binding 3: MetallicRoughnessMap (Sampler2D)
    // Binding 4: AOMap (Sampler2D)
    // Binding 5: EmissiveMap (Sampler2D)
    std::vector<ShaderResource> shaderResources;

    auto addResource = [&](const std::string& name, uint32_t binding, ShaderResource::Type type) {
        ShaderResource res;
        res.Name = name;
        res.ResourceType = type;
        res.Set = 0;
        res.Binding = binding;
        shaderResources.push_back(res);
    };

    addResource("MaterialData", 0, ShaderResource::Type::UniformBuffer);
    addResource("AlbedoMap", 1, ShaderResource::Type::Sampler2D);
    addResource("NormalMap", 2, ShaderResource::Type::Sampler2D);
    addResource("MetallicRoughnessMap", 3, ShaderResource::Type::Sampler2D);
    addResource("AOMap", 4, ShaderResource::Type::Sampler2D);
    addResource("EmissiveMap", 5, ShaderResource::Type::Sampler2D);

    m_DescriptorSetLayout = rf->CreateDescriptorSetLayout(shaderResources);
    m_DescriptorSet = rf->CreateDescriptorSet(m_DescriptorSetLayout.get());

    if (!m_DescriptorSet) return;

    m_DescriptorSet->BindBuffer(0, m_MaterialUBO.get(), 0, sizeof(MaterialData), DescriptorType::UniformBuffer);

    // 创建默认白色纹理作为兜底
    auto getOrCreateDefaultTexture = [&]() -> std::shared_ptr<ITexture> {
        static std::shared_ptr<ITexture> s_defaultWhite;
        if (!s_defaultWhite) {
            uint32_t white = 0xFFFFFFFF;
            TextureDesc desc;
            desc.width = 1;
            desc.height = 1;
            desc.format = TextureFormat::RGBA8_UNorm;
            s_defaultWhite = rm->CreateTextureFromMemory(&white, sizeof(white), desc);
        }
        return s_defaultWhite;
    };
    auto defaultTex = getOrCreateDefaultTexture();

    auto bindTexture = [&](uint32_t binding, std::shared_ptr<ITexture>& tex, const std::string& paramName) {
        auto texToBind = defaultTex;
        if (auto* val = GetParam(paramName)) {
            if (std::holds_alternative<std::shared_ptr<ITexture>>(*val))
                texToBind = std::get<std::shared_ptr<ITexture>>(*val);
        }
        if (texToBind) {
            m_DescriptorSet->BindTexture(binding, texToBind.get(), defaultSampler.get());
        }
    };

    bindTexture(1, m_albedoMap, "AlbedoMap");
    bindTexture(2, m_normalMap, "NormalMap");
    bindTexture(3, m_metallicRoughnessMap, "MetallicRoughnessMap");
    bindTexture(4, m_aoMap, "AOMap");
    bindTexture(5, m_emissiveMap, "EmissiveMap");

    m_DescriptorSet->Update();
}

void Material::UpdateDescriptorSetNPR() {
    auto* rf = Engine::Get().GetRenderSystem()->GetDevice()->GetResourceFactory();
    auto* rm = Engine::Get().GetRenderResourceManager();
    auto defaultSampler = rm->GetDefaultSampler();

    // NPR 布局: Set 0, Binding 0-3
    // Binding 0: NPRMaterialData (UBO)
    // Binding 1: AlbedoMap (Sampler2D)
    // Binding 2: NormalMap (Sampler2D)
    // Binding 3: MetallicRoughnessMap (Sampler2D)
    std::vector<ShaderResource> shaderResources;

    auto addResource = [&](const std::string& name, uint32_t binding, ShaderResource::Type type) {
        ShaderResource res;
        res.Name = name;
        res.ResourceType = type;
        res.Set = 0;
        res.Binding = binding;
        shaderResources.push_back(res);
    };

    addResource("NPRMaterialData", 0, ShaderResource::Type::UniformBuffer);
    addResource("AlbedoMap", 1, ShaderResource::Type::Sampler2D);
    addResource("NormalMap", 2, ShaderResource::Type::Sampler2D);
    addResource("MetallicRoughnessMap", 3, ShaderResource::Type::Sampler2D);

    m_DescriptorSetLayout = rf->CreateDescriptorSetLayout(shaderResources);
    m_DescriptorSet = rf->CreateDescriptorSet(m_DescriptorSetLayout.get());

    if (!m_DescriptorSet) return;

    // 创建 NPR 材质 UBO
    BufferDesc uboDesc;
    uboDesc.type = BufferType::Constant;
    uboDesc.size = sizeof(NPRMaterialData);
    uboDesc.usage = BufferUsage::Dynamic;
    if (!m_MaterialUBO) {
        m_MaterialUBO = rf->CreateBufferImpl(uboDesc);
    }

    m_DescriptorSet->BindBuffer(0, m_MaterialUBO.get(), 0, sizeof(NPRMaterialData), DescriptorType::UniformBuffer);

    // 绑定纹理，缺失时用默认白色
    auto getDefaultWhite = [&]() -> std::shared_ptr<ITexture> {
        static std::shared_ptr<ITexture> s_defaultWhite;
        if (!s_defaultWhite) {
            uint32_t white = 0xFFFFFFFF;
            TextureDesc desc;
            desc.width = 1;
            desc.height = 1;
            desc.format = TextureFormat::RGBA8_UNorm;
            s_defaultWhite = rm->CreateTextureFromMemory(&white, sizeof(white), desc);
        }
        return s_defaultWhite;
    };
    auto defaultTex = getDefaultWhite();

    auto bindTex = [&](uint32_t binding, const std::string& paramName) {
        auto texToBind = defaultTex;
        if (auto* val = GetParam(paramName)) {
            if (std::holds_alternative<std::shared_ptr<ITexture>>(*val))
                texToBind = std::get<std::shared_ptr<ITexture>>(*val);
        }
        m_DescriptorSet->BindTexture(binding, texToBind.get(), defaultSampler.get());
    };

    bindTex(1, "AlbedoMap");
    bindTex(2, "NormalMap");
    bindTex(3, "MetallicRoughnessMap");

    m_DescriptorSet->Update();
}

} // namespace Prisma::Graphic
