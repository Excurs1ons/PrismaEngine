#pragma once

/// @file RenderPass.h
/// @deprecated 此文件已被弃用，请使用 graphic/interfaces/IPass.h 中的 IPass 接口
/// @note 旧版 RenderPass 类将被 LogicalPass 替代
/// @note 此文件仅为向后兼容保留，将在未来版本中移除

#include <memory>
#include "interfaces/IDeviceContext.h"
#include "math/MathTypes.h"
#include "Mesh.h"

// 前向声明
namespace PrismaEngine::Graphic {
class IDeviceContext;
class RenderCommandContext;
}

/// @deprecated 使用 graphic/LogicalPass.h 中的 LogicalPass 替代
class [[deprecated("Use LogicalPass from graphic/LogicalPass.h instead")]] RenderPass
{
public:
    RenderPass();
    virtual ~RenderPass();

    /// @deprecated 使用 IPass::Execute(const PassExecutionContext&) 替代
    virtual void Execute(PrismaEngine::Graphic::RenderCommandContext* context) = 0;

    /// @deprecated 使用 IDeviceContext::SetRenderTarget() 替代
    virtual void SetRenderTarget(void* renderTarget) = 0;

    /// @deprecated 使用 IDeviceContext::ClearRenderTarget() 替代
    virtual void ClearRenderTarget(float r, float g, float b, float a) = 0;

    /// @deprecated 使用 IPass::SetViewport() 或 IDeviceContext::SetViewport() 替代
    virtual void SetViewport(uint32_t width, uint32_t height) = 0;
};

/// @deprecated 2D 渲染请使用 graphic/LogicalPass.h 中的 LogicalPass 创建专门的 2D Pass
class [[deprecated("Create a dedicated 2D pass using LogicalPass instead")]] RenderPass2D : public RenderPass
{
public:
    RenderPass2D();
    ~RenderPass2D() override;

    void Execute(PrismaEngine::Graphic::RenderCommandContext* context) override;

    void AddMeshToRenderQueue(const std::shared_ptr<Mesh>& mesh, const PrismaMath::mat4& transform);
    void SetCameraMatrix(const PrismaMath::mat4& viewProjection);
    void SetViewport(uint32_t width, uint32_t height) override;

private:
    PrismaMath::mat4 m_cameraMatrix;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
