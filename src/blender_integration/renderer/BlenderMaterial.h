#pragma once

#include "../ipc/SceneData.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace Prisma {
namespace Renderer {

// 纹理类型
enum class TextureType {
    ALBEDO = 0,
    NORMAL,
    METALLIC,
    ROUGHNESS,
    AMBIENT_OCCLUSION,
    EMISSION,
    HEIGHT,
    OPACITY,
    SPECULAR,
    CUBEMAP,
    ENVIRONMENT
};

// 纹理数据
struct TextureData {
    std::string path;
    TextureType type;
    glm::vec2 tiling = glm::vec2(1.0f, 1.0f);
    glm::vec2 offset = glm::vec2(0.0f, 0.0f);
    bool srgb = true;
    
    // 加载状态
    bool loaded = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
};

// 着色器类型
enum class ShaderType {
    PBR_STANDARD = 0,
    PBR_METALLIC_ROUGHNESS,
    UNLIT,
    TRANSPARENT,
    CUTOUT,
    EMISSIVE,
    SKYBOX
};

// 材质类 - 用于渲染的材质数据
class BlenderMaterial {
public:
    BlenderMaterial();
    BlenderMaterial(const std::string& name, ShaderType shader_type = ShaderType::PBR_STANDARD);
    ~BlenderMaterial();
    
    // 从Blender数据创建材质
    static std::shared_ptr<BlenderMaterial> CreateFromBlenderData(const MaterialData& material_data);
    
    // 材质属性设置
    void SetAlbedoColor(const glm::vec3& color) { albedo_color_ = color; }
    void SetAlbedoColor(const glm::vec4& color) { albedo_color_ = glm::vec3(color); alpha_ = color.a; }
    void SetMetallic(float metallic) { metallic_ = glm::clamp(metallic, 0.0f, 1.0f); }
    void SetRoughness(float roughness) { roughness_ = glm::clamp(roughness, 0.0f, 1.0f); }
    void SetEmissiveColor(const glm::vec3& color) { emissive_color_ = color; }
    void SetEmissiveIntensity(float intensity) { emissive_intensity_ = intensity; }
    void SetAlpha(float alpha) { alpha_ = glm::clamp(alpha, 0.0f, 1.0f); }
    void SetTransparent(bool transparent) { transparent_ = transparent; }
    void SetDoubleSided(bool double_sided) { double_sided_ = double_sided; }
    
    // 纹理设置
    void SetTexture(TextureType type, const std::string& path);
    void SetTexture(TextureType type, const TextureData& texture);
    void RemoveTexture(TextureType type);
    bool HasTexture(TextureType type) const;
    const TextureData* GetTexture(TextureType type) const;
    
    // 属性获取
    const std::string& GetName() const { return name_; }
    ShaderType GetShaderType() const { return shader_type_; }
    glm::vec3 GetAlbedoColor() const { return albedo_color_; }
    float GetMetallic() const { return metallic_; }
    float GetRoughness() const { return roughness_; }
    glm::vec3 GetEmissiveColor() const { return emissive_color_; }
    float GetEmissiveIntensity() const { return emissive_intensity_; }
    float GetAlpha() const { return alpha_; }
    bool IsTransparent() const { return transparent_; }
    bool IsDoubleSided() const { return double_sided_; }
    
    // 着色器参数
    void SetShaderParam(const std::string& name, float value);
    void SetShaderParam(const std::string& name, const glm::vec2& value);
    void SetShaderParam(const std::string& name, const glm::vec3& value);
    void SetShaderParam(const std::string& name, const glm::vec4& value);
    
    float GetShaderParamFloat(const std::string& name, float default_value = 0.0f) const;
    glm::vec2 GetShaderParamVec2(const std::string& name, const glm::vec2& default_value = glm::vec2(0.0f)) const;
    glm::vec3 GetShaderParamVec3(const std::string& name, const glm::vec3& default_value = glm::vec3(0.0f)) const;
    glm::vec4 GetShaderParamVec4(const std::string& name, const glm::vec4& default_value = glm::vec4(0.0f)) const;
    
    // 序列化/反序列化
    std::vector<uint8_t> Serialize() const;
    static std::shared_ptr<BlenderMaterial> Deserialize(const std::vector<uint8_t>& data);
    
    // 验证
    bool IsValid() const { return !name_.empty(); }
    
private:
    std::string name_;
    ShaderType shader_type_;
    
    // PBR材质属性
    glm::vec3 albedo_color_ = glm::vec3(0.8f, 0.8f, 0.8f);
    float metallic_ = 0.0f;
    float roughness_ = 0.5f;
    glm::vec3 emissive_color_ = glm::vec3(0.0f, 0.0f, 0.0f);
    float emissive_intensity_ = 0.0f;
    float alpha_ = 1.0f;
    
    // 渲染属性
    bool transparent_ = false;
    bool double_sided_ = false;
    
    // 纹理映射
    std::unordered_map<TextureType, TextureData> textures_;
    
    // 自定义着色器参数
    std::unordered_map<std::string, float> float_params_;
    std::unordered_map<std::string, glm::vec2> vec2_params_;
    std::unordered_map<std::string, glm::vec3> vec3_params_;
    std::unordered_map<std::string, glm::vec4> vec4_params_;
};

} // namespace Renderer
} // namespace Prisma