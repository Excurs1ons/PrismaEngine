#pragma once

#include "RenderTypes.h"
#include <string>
#include <vector>
#include <memory>

namespace Prisma::Graphic {

// 材质属性结构
struct MaterialProperties {
    // 基础属性
    alignas(16) glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};  // 基础颜色 (RGBA)
    float metallic = 0.0f;      // 金属度 [0, 1]
    float roughness = 0.5f;     // 粗糙度 [0, 1]
    float emissive = 0.0f;      // 自发光强度
    float normalScale = 1.0f;   // 法线贴图强度

    // 纹理资源句柄（跨平台）
    struct TextureSlot {
        std::shared_ptr<ITexture> texture;
        std::string name;  // 着色器中的变量名
        uint32_t bindPoint = 0;
        uint32_t space = 0;
    };

    std::vector<TextureSlot> textures;
};

// 材质抽象接口
class IMaterial {
public:
    virtual ~IMaterial() = default;

    // 获取材质属性
    virtual const MaterialProperties& GetProperties() const = 0;

    // 设置基础颜色
    virtual void SetBaseColor(const glm::vec4& color) = 0;

    // 设置金属度
    virtual void SetMetallic(float metallic) = 0;

    // 设置粗糙度
    virtual void SetRoughness(float roughness) = 0;

    // 设置自发光强度
    virtual void SetEmissive(float emissive) = 0;

    // 设置纹理
    virtual void SetTexture(uint32_t slot, std::shared_ptr<ITexture> texture) = 0;

    // 获取纹理
    virtual std::shared_ptr<ITexture> GetTexture(uint32_t slot) const = 0;

    // 绑定材质到渲染管线
    virtual void Bind(class ICommandBuffer* commandBuffer) = 0;

    // 解绑材质
    virtual void Unbind(class ICommandBuffer* commandBuffer) = 0;

    // 是否透明
    virtual bool IsTransparent() const = 0;

    // 获取材质名称
    virtual const std::string& GetName() const = 0;

    // 设置材质名称
    virtual void SetName(const std::string& name) = 0;

    // 更新常量缓冲区
    /// 当材质属性改变时调用
    virtual void UpdateConstantBuffer() = 0;
};

} // namespace Prisma::Graphic