#pragma once
#include "../resource/ResourceBase.h"
#include "RenderCommandContext.h"
#include <DirectXMath.h>
#include <string>
#include <memory>

namespace Engine {

    class Shader; // 前向声明

    // 材质属性结构
    struct MaterialProperties {
        DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};  // 基础颜色 (RGBA)
        float metallic = 0.0f;     // 金属度 [0, 1]
        float roughness = 0.5f;    // 粗糙度 [0, 1]
        float emissive = 0.0f;     // 自发光强度
        float normalScale = 1.0f;  // 法线贴图强度

        // 纹理路径 (可选)
        std::string albedoTexture;     // 反照率纹理
        std::string normalTexture;     // 法线纹理
        std::string metallicTexture;   // 金属度纹理
        std::string roughnessTexture;  // 粗糙度纹理
        std::string emissiveTexture;   // 自发光纹理
    };

    class Material : public ResourceBase
    {
    public:
        Material();
        Material(const std::string& name);
        ~Material() override;

        // 基础属性设置
        void SetBaseColor(const DirectX::XMFLOAT4& color);
        void SetBaseColor(float r, float g, float b, float a = 1.0f);
        void SetMetallic(float metallic);
        void SetRoughness(float roughness);
        void SetEmissive(float emissive);

        // 纹理设置
        void SetAlbedoTexture(const std::string& texturePath);
        void SetNormalTexture(const std::string& texturePath);

        // 着色器设置
        void SetShader(std::shared_ptr<Shader> shader);
        std::shared_ptr<Shader> GetShader() const { return m_shader; }

        // 应用材质到渲染上下文
        void Apply(RenderCommandContext* context);

        // 创建默认材质
        static std::shared_ptr<Material> CreateDefault();

        // ResourceBase 接口实现
        bool Load(const std::filesystem::path& path) override;
        void Unload() override;
        bool IsLoaded() const override;
        ResourceType GetType() const override {
            return ResourceType::Material;
        }

        // 获取材质属性
        const MaterialProperties& GetProperties() const { return m_properties; }

    private:
        MaterialProperties m_properties;
        std::shared_ptr<Shader> m_shader;
        bool m_isLoaded;
    };
}
