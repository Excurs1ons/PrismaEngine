#pragma once

// DirectX12适配器集合
// 包含所有DirectX12特定的实现类

#include "DX12RenderDevice.h"
#include "DX12CommandBuffer.h"
#include "DX12Fence.h"
#include "DX12SwapChain.h"
#include "DX12Texture.h"
#include "DX12Buffer.h"
#include "DX12Shader.h"
#include "DX12PipelineState.h"
#include "DX12Sampler.h"
#include "DX12ResourceFactory.h"

namespace PrismaEngine::Graphic::DX12 {

/// @brief 创建DirectX12设备适配器
/// @param backend 现有的DirectX12后端
/// @return 设备适配器智能指针
std::unique_ptr<DX12RenderDevice> CreateDX12RenderDevice(RenderBackendDirectX12* backend);

} // namespace PrismaEngine::Graphic::DX12