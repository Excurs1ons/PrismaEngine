#pragma once

// Vulkan适配器集合
// 包含所有Vulkan特定的实现类

#include "../../../Export.h"
#include "RenderDeviceVulkan.h"
#include "interfaces/IRenderDevice.h"

#include <memory>

namespace Prisma::Graphic::Vulkan {

class RenderDeviceVulkan;

/// @brief 创建Vulkan渲染设备
/// @param deviceDesc 设备描述
/// @return Vulkan渲染设备实例
ENGINE_API std::unique_ptr<RenderDeviceVulkan> CreateRenderDeviceVulkan(const DeviceDesc& deviceDesc);

/// @brief 创建Vulkan渲染设备（接口版本）
/// @param deviceDesc 设备描述
/// @return 渲染设备接口
ENGINE_API std::unique_ptr<IRenderDevice> CreateRenderDeviceVulkanInterface(const DeviceDesc& deviceDesc);

} // namespace Prisma::Graphic::Vulkan