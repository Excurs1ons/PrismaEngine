#pragma once

#include "RenderTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class ITexture;
class IBuffer;
class ISampler;

/**
 * @brief 描述符集抽象接口 (Vulkan VkDescriptorSet 的包装)
 * 这是一个轻量级的对象，由 RHI 内部进行池化管理。
 */
class IDescriptorSet {
public:
    virtual ~IDescriptorSet() = default;

    // 资源绑定接口 (由 Material 调用)
    virtual void BindTexture(uint32_t binding, ITexture* texture, ISampler* sampler) = 0;
    virtual void BindBuffer(uint32_t binding, IBuffer* buffer, uint32_t offset, uint32_t size) = 0;

    // 获取原生句柄 (供后端执行)
    virtual void* GetNativeHandle() const = 0;
    
    // 标记为 Dirty，提醒 RHI 进行 vkUpdateDescriptorSets
    virtual void Update() = 0;
};

/**
 * @brief 描述符集布局抽象 (Vulkan VkDescriptorSetLayout 的包装)
 * 决定了描述符集的结构 (Binding 0 是贴图，Binding 1 是 UBO...)
 */
class IDescriptorSetLayout {
public:
    virtual ~IDescriptorSetLayout() = default;
    virtual void* GetNativeHandle() const = 0;
};

} // namespace Prisma::Graphic
