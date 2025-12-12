#pragma once

#include "graphic/RenderPass.h"
#include <memory>
#include <vector>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Forward {

// 透明物体渲染通道 - 使用深度缓冲和Alpha混合
class TransparentPass : public RenderPass
{
public:
    TransparentPass();
    ~TransparentPass();

    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;

    // 设置渲染目标
    void SetRenderTarget(void* renderTarget) override;

    // 清屏操作
    void ClearRenderTarget(float r, float g, float b, float a) override;

    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;

    // 设置深度缓冲区（只读）
    void SetDepthBuffer(void* depthBuffer);

    // 设置视图矩阵
    void SetViewMatrix(const XMMATRIX& view);

    // 设置投影矩阵
    void SetProjectionMatrix(const XMMATRIX& projection);

    // 启用/禁用深度写入
    void SetDepthWrite(bool enable);

private:
    // 深度缓冲区（只读）
    void* m_depthBuffer = nullptr;

    // 相机矩阵
    XMMATRIX m_view = DirectX::XMMatrixIdentity();
    XMMATRIX m_projection = DirectX::XMMatrixIdentity();
    XMMATRIX m_viewProjection = DirectX::XMMatrixIdentity();

    // 深度写入标志
    bool m_depthWrite = false;

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