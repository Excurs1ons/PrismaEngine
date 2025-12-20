#pragma once

#include "interfaces/IMaterial.h"
#include "DX12ResourceFactory.h"
#include <memory>
#include <string>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;
class DX12ResourceFactory;

/// @brief DirectX12材质适配器
/// 实现IMaterial接口，管理DX12特定的材质资源
class DX12Material : public IMaterial {
public:
    /// @brief 构造函数
    /// @param device DX12渲染设备
    /// @param factory DX12资源工厂
    DX12Material(DX12RenderDevice* device, DX12ResourceFactory* factory);

    /// @brief 析构函数
    ~DX12Material() override;

    // IMaterial接口实现
    const MaterialProperties& GetProperties() const override;
    void SetBaseColor(const glm::vec4& color) override;
    void SetMetallic(float metallic) override;
    void SetRoughness(float roughness) override;
    void SetEmissive(float emissive) override;
    void SetTexture(uint32_t slot, std::shared_ptr<ITexture> texture) override;
    std::shared_ptr<ITexture> GetTexture(uint32_t slot) const override;
    void Bind(class ICommandBuffer* commandBuffer) override;
    void Unbind(class ICommandBuffer* commandBuffer) override;
    bool IsTransparent() const override;
    const std::string& GetName() const override;
    void SetName(const std::string& name) override;
    void UpdateConstantBuffer() override;

    // DX12特定方法
    /// @brief 获取常量缓冲区
    /// @return 常量缓冲区
    std::shared_ptr<DX12ConstantBuffer> GetConstantBuffer() const { return m_constantBuffer; }

    /// @brief 获取根签名中的描述符表起始位置
    /// @return 起始位置
    uint32_t GetDescriptorTableStart() const { return m_descriptorTableStart; }

    /// @brief 设置根签名中的描述符表起始位置
    /// @param start 起始位置
    void SetDescriptorTableStart(uint32_t start) { m_descriptorTableStart = start; }


private:
    DX12RenderDevice* m_device;
    DX12ResourceFactory* m_factory;
    MaterialProperties m_properties;
    std::string m_name;
    bool m_transparent;
    uint32_t m_descriptorTableStart;

    // DX12资源
    std::shared_ptr<DX12ConstantBuffer> m_constantBuffer;  // 材质参数缓冲区

    /// @brief 创建常量缓冲区
    void CreateConstantBuffer();

    /// @brief 更新常量缓冲区数据
    void UpdateConstantBufferData();
};

} // namespace PrismaEngine::Graphic::DX12