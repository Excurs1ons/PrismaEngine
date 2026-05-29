#pragma once

#include "Asset.h"
#include "interfaces/RenderTypes.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace Prisma::Graphic {

class ITexture;
class IBuffer;
class IShader;
class IDescriptorSet;
class IDescriptorSetLayout;

// 材质参数值 (统一存储)
using MaterialParamValue = std::variant<float, PrismaMath::vec3, PrismaMath::vec4, std::shared_ptr<ITexture>>;

// 材质类型枚举 (用于 OpaquePass 选择对应 PSO)
enum class MaterialType : uint8_t {
    Unlit = 0,
    PBR = 1,
    NPR = 2
};

/* 材质资产 (Material) */
class ENGINE_API Material : public Prisma::Asset {
public:
    Material(std::shared_ptr<IShader> shader);
    ~Material() override = default;

    // Asset 接口
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override { return m_IsLoaded; }
    Prisma::AssetType GetType() const override { return Prisma::AssetType::Material; }

    // 参数设置
    void SetParam(const std::string& name, const MaterialParamValue& value);
    const MaterialParamValue* GetParam(const std::string& name) const;

    // 快捷方法 (向后兼容)
    static std::shared_ptr<Material> CreateDefault();
    void SetBaseColor(float r, float g, float b, float a);
    void SetBaseColor(const Prisma::Color& color);
    void SetMetallic(float metallic);
    void SetRoughness(float roughness);
    void SetAO(float ao);
    void SetEmissiveIntensity(float intensity);
    void SetEmissiveColor(const PrismaMath::vec3& color);

    // 纹理贴图设置
    void SetAlbedoMap(std::shared_ptr<ITexture> texture);
    void SetNormalMap(std::shared_ptr<ITexture> texture);
    void SetMetallicRoughnessMap(std::shared_ptr<ITexture> texture);
    void SetAOMap(std::shared_ptr<ITexture> texture);
    void SetEmissiveMap(std::shared_ptr<ITexture> texture);

    // 获取 Shader
    std::shared_ptr<IShader> GetShader() const { return m_Shader; }

    // 状态绑定 (由 OpaquePass 调用)
    void Bind(class ICommandBuffer* cmd);

    // 材质类型 (用于 PSO 选择)
    MaterialType GetMaterialType() const { return m_materialType; }
    void SetMaterialType(MaterialType type) { m_materialType = type; }

    // 获取描述符集
    IDescriptorSet* GetDescriptorSet() const { return m_DescriptorSet.get(); }

    // PBR 材质创建
    static std::shared_ptr<Material> CreatePBR();

    // NPR 材质创建
    static std::shared_ptr<Material> CreateNPR();

private:
    void UpdateDescriptorSet();
    void UpdateDescriptorSetPBR();
    void UpdateDescriptorSetNPR();

    struct MaterialData {
        alignas(16) PrismaMath::vec4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        alignas(4)  float metallic = 0.0f;
        alignas(4)  float roughness = 0.5f;
        alignas(4)  float ao = 1.0f;
        alignas(4)  float emissiveIntensity = 0.0f;
        float padding[3]; // 填充到 48 字节 (std140 16字节对齐)
    };

public:
    struct NPRMaterialData {
        alignas(16) PrismaMath::vec4 baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        alignas(16) PrismaMath::vec4 rimColor = { 1.0f, 0.6f, 0.3f, 0.5f };
        alignas(16) PrismaMath::vec4 shadowColor = { 0.3f, 0.2f, 0.15f, 0.8f };
        alignas(16) PrismaMath::vec4 horizonColor = { 1.0f, 0.7f, 0.3f, 0.3f };
        alignas(4)  float roughness = 0.4f;
        alignas(4)  float rimPower = 2.0f;
        alignas(4)  float wrapAmount = 0.2f;
        alignas(4)  float emissiveIntensity = 0.0f;
        alignas(4)  float bloomThreshold = 0.8f;
        float padding[3]; // pad to 96 bytes (std140 16-byte alignment)
    };

private:
    std::shared_ptr<IShader> m_Shader;
    std::unordered_map<std::string, MaterialParamValue> m_Params;
    
    // 底层描述符集缓存
    std::shared_ptr<IDescriptorSet> m_DescriptorSet;
    std::shared_ptr<IDescriptorSetLayout> m_DescriptorSetLayout;
    std::shared_ptr<IBuffer> m_MaterialUBO;

    // PBR 纹理槽位
    std::shared_ptr<ITexture> m_albedoMap;
    std::shared_ptr<ITexture> m_normalMap;
    std::shared_ptr<ITexture> m_metallicRoughnessMap;
    std::shared_ptr<ITexture> m_aoMap;
    std::shared_ptr<ITexture> m_emissiveMap;

    // NPR 材质参数
    NPRMaterialData m_nprData;

    // 材质类型 (默认 Unlit, 由工厂方法设置)
    MaterialType m_materialType = MaterialType::Unlit;
};

} // namespace Prisma::Graphic
