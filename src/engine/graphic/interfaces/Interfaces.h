#pragma once

// 基础类型
#include "RenderTypes.h"

// 核心系统接口
#include "IRenderDevice.h"
#include "ICommandBuffer.h"
#include "IResourceManager.h"
#include "IResourceFactory.h"

// 资源接口
#include "IResource.h"
#include "ITexture.h"
#include "IBuffer.h"
#include "IShader.h"
#include "IPipelineState.h"
#include "IPipeline.h"
#include "ISampler.h"

// 辅助接口
#include "ISwapChain.h"
#include "IFence.h"

namespace PrismaEngine::Graphic {

/// @brief 渲染系统命名空间
/// 包含所有渲染相关的抽象接口和类型定义

} // namespace PrismaEngine::Graphic