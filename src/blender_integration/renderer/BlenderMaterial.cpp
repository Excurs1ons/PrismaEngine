#include "BlenderMaterial.h"
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Prisma {
namespace Renderer {

BlenderMaterial::BlenderMaterial() 
    : shader_type_(ShaderType::PBR_STANDARD) {
}

BlenderMaterial::BlenderMaterial(const std::string& name, ShaderType shader_type)
    : name_(name)
    , shader_type_(shader_type) {
}

BlenderMaterial::~BlenderMaterial() {
    textures_.clear();
}

std::shared_ptr<BlenderMaterial> BlenderMaterial::CreateFromBlenderData(const MaterialData& material_data) {
    auto material = std::make_shared<BlenderMaterial>(material_data.name);
    
    // 设置基本属性
    material->albedo_color_ = material_data.albedo_color;
    material->metallic_ = material_data.metallic;
    material->roughness_ = material_data.roughness;
    
    // 解析Blender纹理映射
    for (const auto& [texture_name, texture_path] : material_data.textures) {
        TextureType type = TextureType::ALBEDO;
        
        // 根据纹理名称猜测类型
        std::string lower_name = texture_name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        
        if (lower_name.find("albedo") != std::string::npos || 
            lower_name.find("diffuse") != std::string::npos ||
            lower_name.find("color") != std::string::npos) {
            type = TextureType::ALBEDO;
        } else if (lower_name.find("normal") != std::string::npos) {
            type = TextureType::NORMAL;
        } else if (lower_name.find("metallic") != std::string::npos) {
            type = TextureType::METALLIC;
        } else if (lower_name.find("roughness") != std::string::npos) {
            type = TextureType::ROUGHNESS;
        } else if (lower_name.find("ao") != std::string::npos || 
                  lower_name.find("ambient") != std::string::npos) {
            type = TextureType::AMBIENT_OCCLUSION;
        } else if (lower_name.find("emission") != std::string::npos ||
                  lower_name.find("emissive") != std::string::npos) {
            type = TextureType::EMISSION;
        } else if (lower_name.find("height") != std::string::npos ||
                  lower_name.find("displacement") != std::string::npos) {
            type = TextureType::HEIGHT;
        } else if (lower_name.find("opacity") != std::string::npos ||
                  lower_name.find("alpha") != std::string::npos) {
            type = TextureType::OPACITY;
        } else if (lower_name.find("specular") != std::string::npos) {
            type = TextureType::SPECULAR;
        }
        
        TextureData texture;
        texture.path = texture_path;
        texture.type = type;
        
        material->textures_[type] = texture;
    }
    
    // 设置透明度
    if (material->alpha_ < 1.0f) {
        material->transparent_ = true;
    }
    
    // 检查是否有透明纹理
    if (material->HasTexture(TextureType::OPACITY)) {
        material->transparent_ = true;
    }
    
    return material;
}

void BlenderMaterial::SetTexture(TextureType type, const std::string& path) {
    TextureData texture;
    texture.path = path;
    texture.type = type;
    textures_[type] = texture;
}

void BlenderMaterial::SetTexture(TextureType type, const TextureData& texture) {
    textures_[type] = texture;
}

void BlenderMaterial::RemoveTexture(TextureType type) {
    textures_.erase(type);
}

bool BlenderMaterial::HasTexture(TextureType type) const {
    return textures_.find(type) != textures_.end();
}

const TextureData* BlenderMaterial::GetTexture(TextureType type) const {
    auto it = textures_.find(type);
    if (it != textures_.end()) {
        return &it->second;
    }
    return nullptr;
}

void BlenderMaterial::SetShaderParam(const std::string& name, float value) {
    float_params_[name] = value;
}

void BlenderMaterial::SetShaderParam(const std::string& name, const glm::vec2& value) {
    vec2_params_[name] = value;
}

void BlenderMaterial::SetShaderParam(const std::string& name, const glm::vec3& value) {
    vec3_params_[name] = value;
}

void BlenderMaterial::SetShaderParam(const std::string& name, const glm::vec4& value) {
    vec4_params_[name] = value;
}

float BlenderMaterial::GetShaderParamFloat(const std::string& name, float default_value) const {
    auto it = float_params_.find(name);
    if (it != float_params_.end()) {
        return it->second;
    }
    return default_value;
}

glm::vec2 BlenderMaterial::GetShaderParamVec2(const std::string& name, const glm::vec2& default_value) const {
    auto it = vec2_params_.find(name);
    if (it != vec2_params_.end()) {
        return it->second;
    }
    return default_value;
}

glm::vec3 BlenderMaterial::GetShaderParamVec3(const std::string& name, const glm::vec3& default_value) const {
    auto it = vec3_params_.find(name);
    if (it != vec3_params_.end()) {
        return it->second;
    }
    return default_value;
}

glm::vec4 BlenderMaterial::GetShaderParamVec4(const std::string& name, const glm::vec4& default_value) const {
    auto it = vec4_params_.find(name);
    if (it != vec4_params_.end()) {
        return it->second;
    }
    return default_value;
}

std::vector<uint8_t> BlenderMaterial::Serialize() const {
    // 简单的二进制序列化
    std::vector<uint8_t> data;
    
    // 头部
    struct Header {
        uint32_t magic = 0x4D41544C;  // "MATL"
        uint32_t version = 1;
        uint32_t name_length;
        uint32_t shader_type;
        uint32_t texture_count;
        uint32_t float_param_count;
        uint32_t vec2_param_count;
        uint32_t vec3_param_count;
        uint32_t vec4_param_count;
        float albedo_color[3];
        float metallic;
        float roughness;
        float emissive_color[3];
        float emissive_intensity;
        float alpha;
        uint8_t transparent;
        uint8_t double_sided;
        uint8_t padding[2];
    } header;
    
    header.name_length = static_cast<uint32_t>(name_.length());
    header.shader_type = static_cast<uint32_t>(shader_type_);
    header.texture_count = static_cast<uint32_t>(textures_.size());
    header.float_param_count = static_cast<uint32_t>(float_params_.size());
    header.vec2_param_count = static_cast<uint32_t>(vec2_params_.size());
    header.vec3_param_count = static_cast<uint32_t>(vec3_params_.size());
    header.vec4_param_count = static_cast<uint32_t>(vec4_params_.size());
    
    memcpy(header.albedo_color, glm::value_ptr(albedo_color_), sizeof(float) * 3);
    header.metallic = metallic_;
    header.roughness = roughness_;
    memcpy(header.emissive_color, glm::value_ptr(emissive_color_), sizeof(float) * 3);
    header.emissive_intensity = emissive_intensity_;
    header.alpha = alpha_;
    header.transparent = transparent_ ? 1 : 0;
    header.double_sided = double_sided_ ? 1 : 0;
    
    // 写入头部
    size_t offset = data.size();
    data.resize(data.size() + sizeof(Header));
    memcpy(data.data() + offset, &header, sizeof(Header));
    
    // 写入名称
    offset = data.size();
    data.resize(data.size() + name_.length());
    memcpy(data.data() + offset, name_.c_str(), name_.length());
    
    // 写入纹理数据（简化版本，只存储类型和路径长度）
    for (const auto& [type, texture] : textures_) {
        // 纹理类型
        uint32_t tex_type = static_cast<uint32_t>(type);
        offset = data.size();
        data.resize(data.size() + sizeof(uint32_t));
        memcpy(data.data() + offset, &tex_type, sizeof(uint32_t));
        
        // 路径长度
        uint32_t path_length = static_cast<uint32_t>(texture.path.length());
        offset = data.size();
        data.resize(data.size() + sizeof(uint32_t));
        memcpy(data.data() + offset, &path_length, sizeof(uint32_t));
        
        // 路径内容
        if (path_length > 0) {
            offset = data.size();
            data.resize(data.size() + path_length);
            memcpy(data.data() + offset, texture.path.c_str(), path_length);
        }
    }
    
    // 写入浮点参数
    for (const auto& [name, value] : float_params_) {
        // 名称长度
        uint32_t name_length = static_cast<uint32_t>(name.length());
        offset = data.size();
        data.resize(data.size() + sizeof(uint32_t));
        memcpy(data.data() + offset, &name_length, sizeof(uint32_t));
        
        // 名称内容
        offset = data.size();
        data.resize(data.size() + name_length);
        memcpy(data.data() + offset, name.c_str(), name_length);
        
        // 值
        offset = data.size();
        data.resize(data.size() + sizeof(float));
        memcpy(data.data() + offset, &value, sizeof(float));
    }
    
    // 写入vec2参数
    for (const auto& [name, value] : vec2_params_) {
        // 名称长度
        uint32_t name_length = static_cast<uint32_t>(name.length());
        offset = data.size();
        data.resize(data.size() + sizeof(uint32_t));
        memcpy(data.data() + offset, &name_length, sizeof(uint32_t));
        
        // 名称内容
        offset = data.size();
        data.resize(data.size() + name_length);
        memcpy(data.data() + offset, name.c_str(), name_length);
        
        // 值
        offset = data.size();
        data.resize(data.size() + sizeof(float) * 2);
        memcpy(data.data() + offset, glm::value_ptr(value), sizeof(float) * 2);
    }
    
    // 写入vec3参数
    for (const auto& [name, value] : vec3_params_) {
        // 名称长度
        uint32_t name_length = static_cast<uint32_t>(name.length());
        offset = data.size();
        data.resize(data.size() + sizeof(uint32_t));
        memcpy(data.data() + offset, &name_length, sizeof(uint32_t));
        
        // 名称内容
        offset = data.size();
        data.resize(data.size() + name_length);
        memcpy(data.data() + offset, name.c_str(), name_length);
        
        // 值
        offset = data.size();
        data.resize(data.size() + sizeof(float) * 3);
        memcpy(data.data() + offset, glm::value_ptr(value), sizeof(float) * 3);
    }
    
    // 写入vec4参数
    for (const auto& [name, value] : vec4_params_) {
        // 名称长度
        uint32_t name_length = static_cast<uint32_t>(name.length());
        offset = data.size();
        data.resize(data.size() + sizeof(uint32_t));
        memcpy(data.data() + offset, &name_length, sizeof(uint32_t));
        
        // 名称内容
        offset = data.size();
        data.resize(data.size() + name_length);
        memcpy(data.data() + offset, name.c_str(), name_length);
        
        // 值
        offset = data.size();
        data.resize(data.size() + sizeof(float) * 4);
        memcpy(data.data() + offset, glm::value_ptr(value), sizeof(float) * 4);
    }
    
    return data;
}

std::shared_ptr<BlenderMaterial> BlenderMaterial::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(Header)) {
        return nullptr;
    }
    
    const auto* header = reinterpret_cast<const Header*>(data.data());
    if (header->magic != 0x4D41544C) {
        return nullptr;
    }
    
    // 读取名称
    const char* name_start = reinterpret_cast<const char*>(data.data() + sizeof(Header));
    std::string name(name_start, header->name_length);
    
    auto material = std::make_shared<BlenderMaterial>(name);
    material->shader_type_ = static_cast<ShaderType>(header->shader_type);
    
    // 读取材质属性
    memcpy(glm::value_ptr(material->albedo_color_), header->albedo_color, sizeof(float) * 3);
    material->metallic_ = header->metallic;
    material->roughness_ = header->roughness;
    memcpy(glm::value_ptr(material->emissive_color_), header->emissive_color, sizeof(float) * 3);
    material->emissive_intensity_ = header->emissive_intensity;
    material->alpha_ = header->alpha;
    material->transparent_ = header->transparent != 0;
    material->double_sided_ = header->double_sided != 0;
    
    // 读取纹理数据（简化版本）
    const uint8_t* texture_data = data.data() + sizeof(Header) + header->name_length;
    for (uint32_t i = 0; i < header->texture_count; ++i) {
        // 读取纹理类型
        uint32_t tex_type = *reinterpret_cast<const uint32_t*>(texture_data);
        texture_data += sizeof(uint32_t);
        
        // 读取路径长度
        uint32_t path_length = *reinterpret_cast<const uint32_t*>(texture_data);
        texture_data += sizeof(uint32_t);
        
        // 读取路径内容
        std::string path(reinterpret_cast<const char*>(texture_data), path_length);
        texture_data += path_length;
        
        TextureData texture;
        texture.type = static_cast<TextureType>(tex_type);
        texture.path = path;
        
        material->textures_[texture.type] = texture;
    }
    
    // 读取浮点参数
    const uint8_t* param_data = texture_data;
    for (uint32_t i = 0; i < header->float_param_count; ++i) {
        uint32_t name_length = *reinterpret_cast<const uint32_t*>(param_data);
        param_data += sizeof(uint32_t);
        
        std::string name(reinterpret_cast<const char*>(param_data), name_length);
        param_data += name_length;
        
        float value = *reinterpret_cast<const float*>(param_data);
        param_data += sizeof(float);
        
        material->float_params_[name] = value;
    }
    
    // 读取vec2参数
    for (uint32_t i = 0; i < header->vec2_param_count; ++i) {
        uint32_t name_length = *reinterpret_cast<const uint32_t*>(param_data);
        param_data += sizeof(uint32_t);
        
        std::string name(reinterpret_cast<const char*>(param_data), name_length);
        param_data += name_length;
        
        glm::vec2 value;
        memcpy(glm::value_ptr(value), param_data, sizeof(float) * 2);
        param_data += sizeof(float) * 2;
        
        material->vec2_params_[name] = value;
    }
    
    // 读取vec3参数
    for (uint32_t i = 0; i < header->vec3_param_count; ++i) {
        uint32_t name_length = *reinterpret_cast<const uint32_t*>(param_data);
        param_data += sizeof(uint32_t);
        
        std::string name(reinterpret_cast<const char*>(param_data), name_length);
        param_data += name_length;
        
        glm::vec3 value;
        memcpy(glm::value_ptr(value), param_data, sizeof(float) * 3);
        param_data += sizeof(float) * 3;
        
        material->vec3_params_[name] = value;
    }
    
    // 读取vec4参数
    for (uint32_t i = 0; i < header->vec4_param_count; ++i) {
        uint32_t name_length = *reinterpret_cast<const uint32_t*>(param_data);
        param_data += sizeof(uint32_t);
        
        std::string name(reinterpret_cast<const char*>(param_data), name_length);
        param_data += name_length;
        
        glm::vec4 value;
        memcpy(glm::value_ptr(value), param_data, sizeof(float) * 4);
        param_data += sizeof(float) * 4;
        
        material->vec4_params_[name] = value;
    }
    
    return material;
}

} // namespace Renderer
} // namespace Prisma