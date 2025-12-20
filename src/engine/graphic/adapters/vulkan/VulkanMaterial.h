#pragma once

#include "interfaces/IMaterial.h"
#include <memory>
#include <string>
#include <vulkan/vulkan.h>

namespace PrismaEngine::Graphic::Vulkan {

// 前置声明
class VulkanRenderDevice;
class VulkanResourceFactory;

/// @brief Vulkan材质适配器
/// 实现IMaterial接口，管理Vulkan特定的材质资源
class VulkanMaterial : public IMaterial {
public:
    /// @brief 构造函数
    /// @param device Vulkan渲染设备
    /// @param factory Vulkan资源工厂
    VulkanMaterial(VulkanRenderDevice* device, VulkanResourceFactory* factory);

    /// @brief 析构函数
    ~VulkanMaterial() override;

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

    // Vulkan特定方法
    /// @brief 获取描述符集
    /// @return 描述符集
    VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }

    /// @brief 获取描述符集布局
    /// @return 描述符集布局
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

    /// @brief 更新描述符集
    /// 当纹理改变时调用
    void UpdateDescriptorSet();


private:
    VulkanRenderDevice* m_device;
    VulkanResourceFactory* m_factory;
    MaterialProperties m_properties;
    std::string m_name;
    bool m_transparent;

    // Vulkan资源
    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    void* m_mappedData = nullptr;

    /// @brief 创建统一缓冲区
    void CreateUniformBuffer();

    /// @brief 创建描述符集
    void CreateDescriptorSet();

    /// @brief 更新统一缓冲区数据
    void UpdateUniformBufferData();

    /// @brief 清理资源
    void Cleanup();
};

} // namespace PrismaEngine::Graphic::Vulkan