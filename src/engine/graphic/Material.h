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
class Shader;

/**
 * @brief 材质参数值 (统一存储)
 */
using MaterialParamValue = std::variant<float, PrismaMath::vec3, PrismaMath::vec4, std::shared_ptr<ITexture>>;

/**
 * @brief 材质资产 (Material)
 * 一个材质由一个 Shader 和一组参数组成。
 */
class ENGINE_API Material : public Prisma::Asset {
public:
    Material(std::shared_ptr<Shader> shader);
    ~Material() override = default;

    // Asset 接口
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override { return m_Shader != nullptr; }
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

    // 获取 Shader
    std::shared_ptr<Shader> GetShader() const { return m_Shader; }

    // 状态绑定 (由 OpaquePass 调用)
    void Bind(class ICommandBuffer* cmd);

private:
    std::shared_ptr<Shader> m_Shader;
    std::unordered_map<std::string, MaterialParamValue> m_Params;
    
    // 底层 Vulkan Descriptor Set 缓存 (由 RHI 管理)
    void* m_DescriptorSetHandle = nullptr; 
};

} // namespace Prisma::Graphic
