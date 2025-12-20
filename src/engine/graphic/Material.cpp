#include "Material.h"
#include "Shader.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "DefaultShader.h"

using namespace Engine;

Material::Material()
    : m_isLoaded(false) {
    m_name = "Unnamed Material";
}

Material::Material(const std::string& name)
    : m_isLoaded(false) {
    m_name = name;
}

Material::~Material() {
    Unload();
}

// 基础属性设置
void Material::SetBaseColor(const PrismaMath::vec4& color) {
    m_properties.baseColor = color;
}

void Material::SetBaseColor(float r, float g, float b, float a) {
    m_properties.baseColor = PrismaMath::vec4(r, g, b, a);
}

void Material::SetMetallic(float metallic) {
    m_properties.metallic = std::clamp(metallic, 0.0f, 1.0f);
}

void Material::SetRoughness(float roughness) {
    m_properties.roughness = std::clamp(roughness, 0.0f, 1.0f);
}

void Material::SetEmissive(float emissive) {
    m_properties.emissive = std::max(0.0f, emissive);
}

// 纹理设置
void Material::SetAlbedoTexture(const std::string& texturePath) {
    m_properties.albedoTexture = texturePath;
}

void Material::SetNormalTexture(const std::string& texturePath) {
    m_properties.normalTexture = texturePath;
}

// 着色器设置
void Material::SetShader(std::shared_ptr<Shader> shader) {
    m_shader = shader;
}

void Material::Apply(RenderCommandContext* context) {
    if (!context) {
        LOG_WARNING("Material", "Apply: context is null");
        return;
    }

    LOG_DEBUG("Material", "应用材质 '{0}' 到渲染上下文", m_name);

    // TODO: 设置材质特定的PSO
    // 注意：当前使用的是RenderBackend的默认PSO

    // 1. 设置基础颜色常量缓冲区 (寄存器 b2)
    context->SetConstantBuffer("BaseColor", reinterpret_cast<const float*>(&m_properties.baseColor), 4);

    // 2. 设置其他材质参数 (寄存器 b3)
    float materialParams[4] = {
        m_properties.metallic,      // metallic
        m_properties.roughness,     // roughness
        m_properties.emissive,      // emissive
        m_properties.normalScale    // normalScale
    };
    context->SetConstantBuffer("MaterialParams", materialParams, 4);

    // 3. TODO: 绑定纹理 (纹理系统实现后)
    // if (!m_properties.albedoTexture.empty()) {
    //     // 绑定反照率纹理
    // }

    // 4. TODO: 绑定采样器
    // context->SetSampler("MainSampler", mainSampler);

    LOG_DEBUG("Material", "材质应用完成: 颜色=({0}, {1}, {2}, {3}), 金属度={4}, 粗糙度={5}",
        m_properties.baseColor.x, m_properties.baseColor.y, m_properties.baseColor.z, m_properties.baseColor.w,
        m_properties.metallic, m_properties.roughness);
}

// 创建默认材质
std::shared_ptr<Material> Material::CreateDefault() {
    auto material = std::make_shared<Material>("DefaultMaterial");

    // 设置默认属性
    material->SetBaseColor(1.0f, 1.0f, 1.0f, 1.0f);  // 白色
    material->SetMetallic(0.0f);                        // 非金属
    material->SetRoughness(0.5f);                       // 中等粗糙度
    material->SetEmissive(0.0f);                        // 无自发光

    // 使用硬编码的默认着色器
    auto defaultShader = std::make_shared<Shader>();
    if (defaultShader->CompileFromString(Graphic::DEFAULT_VERTEX_SHADER, Graphic::DEFAULT_PIXEL_SHADER)) {
        defaultShader->SetName("DefaultMaterialShader");
        material->SetShader(defaultShader);
        //LOG_INFO("Material", "默认材质加载了硬编码着色器");
    } else {
        //LOG_ERROR("Material", "默认材质无法编译硬编码着色器");
    }

    material->m_isLoaded = true;
    return material;
}

// ResourceBase 接口实现
bool Material::Load(const std::filesystem::path& path) {
    m_path = path;
    m_name = path.filename().string();

    // TODO: 从文件加载材质属性
    // 目前标记为已加载
    m_isLoaded = true;

    LOG_INFO("Material", "材质 '{0}' 已加载", m_name);
    return true;
}

void Material::Unload() {
    m_isLoaded = false;
    m_shader.reset();
    m_properties = MaterialProperties(); // 重置为默认值
    LOG_INFO("Material", "材质 '{0}' 已卸载", m_name);
}

bool Material::IsLoaded() const {
    return m_isLoaded;
}
