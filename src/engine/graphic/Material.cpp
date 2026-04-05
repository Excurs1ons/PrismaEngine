#include "Material.h"
#include "graphic/interfaces/IShader.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IResourceManager.h"
#include "RenderResourceManager.h"
#include "app/Engine.h"
#include "graphic/RenderSystem.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "logger/Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>

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
        const std::string shaderName = root["shader"].get<std::string>();
        auto resourceManager = Engine::Get().GetRenderResourceManager();
        if (resourceManager) {
            auto shaderRes = resourceManager->LoadShaderSync(shaderName);
            if (shaderRes) {
                m_Shader = shaderRes;
            }
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

    // 绑定参数逻辑 (目前简化：仅处理 Set 0, Binding 0 为贴图的情况)
    auto it = m_Params.find("AlbedoMap");
    if (it != m_Params.end() && std::holds_alternative<std::shared_ptr<ITexture>>(it->second)) {
        auto texture = std::get<std::shared_ptr<ITexture>>(it->second);
        if (texture) {
            // 获取或创建描述符集
            if (!m_DescriptorSetHandle) {
                auto* engine = &Engine::Get();
                auto* rf = engine->GetRenderSystem()->GetDevice()->GetResourceFactory();
                
                // 找到 Shader 中的第一个采样器资源信息
                const ShaderResource* resInfo = m_Shader->FindResourceByBindPoint(0, 0);
                if (resInfo) {
                    std::vector<ShaderResource> resources = { *resInfo };
                    auto layout = rf->CreateDescriptorSetLayout(resources);
                    auto ds = rf->CreateDescriptorSet(layout.get());
                    
                    // 这里由于接口限制，暂时通过强转缓存句柄
                    // 生产环境下应该有更好的 DescriptorSet 管理器
                    m_DescriptorSetHandle = ds.get();
                    // 增加引用计数以防销毁 (这是一个 HACK，为了演示)
                    static std::vector<std::shared_ptr<IDescriptorSet>> s_KeepAlive;
                    s_KeepAlive.push_back(ds);
                }
            }

            if (m_DescriptorSetHandle) {
                auto* ds = static_cast<IDescriptorSet*>(m_DescriptorSetHandle);
                ds->BindTexture(0, texture.get(), nullptr);
                ds->Update();
                cmd->BindDescriptorSet(0, ds);
            }
        }
    }
}

} // namespace Prisma::Graphic
