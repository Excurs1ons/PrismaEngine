#pragma once

#include "graphic/RenderPass.h"
#include "GBuffer.h"
#include <DirectXMath.h>
#include <memory>

namespace Engine {
class Shader;

namespace Graphic {
namespace Pipelines {
namespace Deferred {

// 几何通道 - 将场景几何信息渲染到G-Buffer
class GeometryPass : public RenderPass
{
public:
    GeometryPass();
    ~GeometryPass();

    // 渲染通道执行函数
    void Execute(RenderCommandContext* context) override;

    // 设置渲染目标（通过G-Buffer）
    void SetGBuffer(std::shared_ptr<GBuffer> gbuffer);

    // 清屏操作（通常由G-Buffer处理）
    void ClearRenderTarget(float r, float g, float b, float a) override;

    // 设置视口
    void SetViewport(uint32_t width, uint32_t height) override;

    // 设置视图矩阵
    void SetViewMatrix(const DirectX::XMMATRIX& view);

    // 设置投影矩阵
    void SetProjectionMatrix(const DirectX::XMMATRIX& projection);

    // 设置视图投影矩阵
    void SetViewProjectionMatrix(const DirectX::XMMATRIX& viewProjection);

    // 启用/禁用深度预渲染
    void SetDepthPrePass(bool enable);

private:
    // G-Buffer引用
    std::shared_ptr<GBuffer> m_gbuffer;

    // 相机矩阵
    DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_projection = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_viewProjection = DirectX::XMMatrixIdentity();

    // 视口尺寸
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // 深度预渲染标志
    bool m_depthPrePass = true;

    // 渲染统计
    struct GeometryPassStats {
        uint32_t renderedObjects = 0;
        uint32_t culledObjects = 0;
        uint32_t triangles = 0;
    } m_stats;

    // 延迟渲染几何通道着色器
    std::shared_ptr<Shader> m_shader;
};

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine