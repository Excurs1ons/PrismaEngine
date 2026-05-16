#include "Material.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "RenderResourceManager.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "logger/Logger.h"
#include <fstream>
#include <glaze/glaze.hpp>
#include <glaze/json/json_t.hpp>

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

        if (properties.contains("albedo") && properties.at("albedo").is_array() && properties.at("albedo").get_array().size() >= 4) {
            auto& albedoArr = properties.at("albedo").get_array();
            SetBaseColor(
                static_cast<float>(albedoArr[0].get_double()),
                static_cast<float>(albedoArr[1].get_double()),
                static_cast<float>(albedoArr[2].get_double()),
                static_cast<float>(albedoArr[3].get_double()));
        }
        if (properties.contains("metallic")) {
            SetMetallic(static_cast<float>(properties.at("metallic").get_double()));
        }
        if (properties.contains("roughness")) {
            SetRoughness(static_cast<float>(properties.at("roughness").get_double()));
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

void Material::Bind(ICommandBuffer* cmd) {
    if (!cmd || !m_Shader) return;

    // 获取或创建描述符集（仅一次）
    if (!m_DescriptorSet) {
        auto* engine = &Engine::Get();
        auto* rf = engine->GetRenderSystem()->GetDevice()->GetResourceFactory();

        // 收集着色器中所有与材质参数匹配的采样器资源信息
        std::vector<ShaderResource> shaderResources;
        for (const auto& [name, value] : m_Params) {
            if (!std::holds_alternative<std::shared_ptr<ITexture>>(value))
                continue;
            const ShaderResource* resInfo = m_Shader->FindResource(name);
            if (resInfo) {
                shaderResources.push_back(*resInfo);
            }
        }

        if (!shaderResources.empty()) {
            m_DescriptorSetLayout = rf->CreateDescriptorSetLayout(shaderResources);
            m_DescriptorSet = rf->CreateDescriptorSet(m_DescriptorSetLayout.get());

            if (m_DescriptorSet) {
                auto defaultSampler = Engine::Get().GetRenderResourceManager()->GetDefaultSampler();

                // 遍历所有纹理参数，按资源名找到对应 binding 并绑定
                for (const auto& [name, value] : m_Params) {
                    if (!std::holds_alternative<std::shared_ptr<ITexture>>(value))
                        continue;
                    auto texture = std::get<std::shared_ptr<ITexture>>(value);
                    if (!texture) continue;

                    const ShaderResource* resInfo = m_Shader->FindResource(name);
                    if (resInfo) {
                        m_DescriptorSet->BindTexture(resInfo->Binding, texture.get(), defaultSampler.get());
                    }
                }
                m_DescriptorSet->Update();
            }
        }
    }

    if (m_DescriptorSet) {
        cmd->BindDescriptorSet(0, m_DescriptorSet.get());
    }
}

} // namespace Prisma::Graphic
