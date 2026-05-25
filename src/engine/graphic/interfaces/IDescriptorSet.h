#pragma once

#include "RenderTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ITexture;
class IBuffer;
class ISampler;

// 描述符类型枚举
enum class DescriptorType {
    UniformBuffer,      // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    StorageBuffer,      // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER (SSBO)
    StorageImage,       // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    SampledImage,       // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    Sampler,            // VK_DESCRIPTOR_TYPE_SAMPLER
};

/* 描述符集抽象接口 (Vulkan VkDescriptorSet 的包装) */
class IDescriptorSet {
public:
    virtual ~IDescriptorSet() = default;

    // 资源绑定接口 (由 Material 调用)
    virtual void BindTexture(uint32_t binding, ITexture* texture, ISampler* sampler) = 0;
    virtual void BindBuffer(uint32_t binding, IBuffer* buffer, uint32_t offset, uint32_t size,
                            DescriptorType type = DescriptorType::UniformBuffer) = 0;
    virtual void BindStorageImage(uint32_t binding, ITexture* texture) = 0;

    // 绑定加速结构（光线追踪 TLAS/BLAS）
    virtual void BindAccelerationStructure(uint32_t binding, void* accelerationStructure) = 0;

    // 获取原生句柄 (供后端执行)
    virtual void* GetNativeHandle() const = 0;
    
    // 标记为 Dirty，提醒 RHI 进行 vkUpdateDescriptorSets
    virtual void Update() = 0;
};

/* 描述符集布局抽象 (Vulkan VkDescriptorSetLayout 的包装) */
class IDescriptorSetLayout {
public:
    virtual ~IDescriptorSetLayout() = default;
    virtual void* GetNativeHandle() const = 0;
};

} // namespace Prisma::Graphic
