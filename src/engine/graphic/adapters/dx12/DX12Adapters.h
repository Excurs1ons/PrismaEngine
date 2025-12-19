#pragma once

// DirectX12适配器集合
// 包含所有DirectX12特定的实现类

#include "DX12RenderDevice.h"
#include "DX12Fence.h"
#include "interfaces/IRenderDevice.h"

#include <memory>

namespace PrismaEngine::Graphic::DX12 {
class DX12RenderDevice;
/// @brief 创建DirectX12渲染设备
/// @param deviceDesc 设备描述
/// @return DirectX12渲染设备实例
std::unique_ptr<DX12RenderDevice> CreateDX12RenderDevice(const DeviceDesc& deviceDesc);

/// @brief 创建DirectX12渲染设备（接口版本）
/// @param deviceDesc 设备描述
/// @return 渲染设备接口
std::unique_ptr<IRenderDevice> CreateDX12RenderDeviceInterface(const DeviceDesc& deviceDesc);

} // namespace PrismaEngine::Graphic::DX12