#pragma once

#include "graphic/RenderPass.h"
#include "math/MathTypes.h"
#include <memory>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

// 深度预渲染通道 - 用于提前构建深度缓冲，优化后续渲染的遮挡剔除
class DepthPrePass : public RenderPass
{
public:
    DepthPrePass();
    ~DepthPrePass();

    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;

    // 设置渲染目标
    void SetRenderTarget(void* renderTarget) override;

    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;

    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;

    // 设置深度缓冲区
    void SetDepthBuffer(void* depthBuffer);

    // 设置视图投影矩阵
    void SetViewProjectionMatrix(const PrismaMath::mat4& viewProjection);

    // 设置视图和投影矩阵
    void SetViewMatrix(const PrismaMath::mat4& view);
    void SetProjectionMatrix(const PrismaMath::mat4& projection);

private:
    // 深度缓冲区
    void* m_depthBuffer = nullptr;

    // 视图和投影矩阵
    PrismaMath::mat4 m_view = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_projection = PrismaMath::mat4(1.0f);
    PrismaMath::mat4 m_viewProjection = PrismaMath::mat4(1.0f);

    // 渲染目标
    void* m_renderTarget = nullptr;

    // 视口尺寸
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace Forward
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine