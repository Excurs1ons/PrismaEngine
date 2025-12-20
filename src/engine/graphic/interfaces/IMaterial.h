#pragma once

#include "RenderTypes.h"
#include <string>
#include <vector>
#include <memory>

namespace PrismaEngine::Graphic {

/// @brief 材质属性结构
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

/// @brief 材质抽象接口
class IMaterial {
public:
    virtual ~IMaterial() = default;

    /// @brief 获取材质属性
    /// @return 材质属性
    virtual const MaterialProperties& GetProperties() const = 0;

    /// @brief 设置基础颜色
    /// @param color RGBA颜色
    virtual void SetBaseColor(const glm::vec4& color) = 0;

    /// @brief 设置金属度
    /// @param metallic 金属度值 [0, 1]
    virtual void SetMetallic(float metallic) = 0;

    /// @brief 设置粗糙度
    /// @param roughness 粗糙度值 [0, 1]
    virtual void SetRoughness(float roughness) = 0;

    /// @brief 设置自发光强度
    /// @param emissive 自发光强度
    virtual void SetEmissive(float emissive) = 0;

    /// @brief 设置纹理
    /// @param slot 纹理槽位
    /// @param texture 纹理资源
    virtual void SetTexture(uint32_t slot, std::shared_ptr<ITexture> texture) = 0;

    /// @brief 获取纹理
    /// @param slot 纹理槽位
    /// @return 纹理资源
    virtual std::shared_ptr<ITexture> GetTexture(uint32_t slot) const = 0;

    /// @brief 绑定材质到渲染管线
    /// @param commandBuffer 命令缓冲区
    virtual void Bind(class ICommandBuffer* commandBuffer) = 0;

    /// @brief 解绑材质
    /// @param commandBuffer 命令缓冲区
    virtual void Unbind(class ICommandBuffer* commandBuffer) = 0;

    /// @brief 是否透明
    /// @return 是否为透明材质
    virtual bool IsTransparent() const = 0;

    /// @brief 获取材质名称
    /// @return 材质名称
    virtual const std::string& GetName() const = 0;

    /// @brief 设置材质名称
    /// @param name 材质名称
    virtual void SetName(const std::string& name) = 0;

    /// @brief 更新常量缓冲区
    /// 当材质属性改变时调用
    virtual void UpdateConstantBuffer() = 0;
};

} // namespace PrismaEngine::Graphic