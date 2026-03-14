#pragma once

#include "interfaces/IFence.h"
#include <vulkan/vulkan.h>

namespace Prisma::Graphic::Vulkan {

class VulkanFence : public IFence {
public:
    VulkanFence(VkDevice device);
    ~VulkanFence() override;

    FenceState GetState() const override;
    uint64_t GetCompletedValue() const override;
    void Signal(uint64_t value) override;
    bool Wait(uint64_t value, uint64_t timeout = 0) override;
    void Reset() override;
    void SetEventOnCompletion(uint64_t value, void* event) override;

    VkFence GetVkFence() const { return m_fence; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;
};

} // namespace Prisma::Graphic::Vulkan
