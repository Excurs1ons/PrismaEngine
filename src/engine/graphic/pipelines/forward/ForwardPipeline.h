#pragma once

#include "interfaces/IPipeline.h"
#include "interfaces/IShader.h"
#include "interfaces/IPipelineState.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class DepthPrePass;
class OpaquePass;
class SkyboxPass;
class TransparentPass;
class Light2DPass;

/**
 * @brief 基础前向渲染管线
 */
class ForwardPipeline : public IPipeline {
public:
    ForwardPipeline();
    ~ForwardPipeline() override;

    // IPipeline 接口
    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;

private:
    void EnsureGizmoPSO();

    IRenderDevice* m_device = nullptr;

    // 预定义的渲染通道
    std::shared_ptr<DepthPrePass> m_depthPrePass;
    std::shared_ptr<OpaquePass> m_opaquePass;
    std::shared_ptr<Light2DPass> m_light2DPass;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
    std::shared_ptr<TransparentPass> m_transparentPass;

    // Gizmo 覆盖层管线（UnlitVertex：纯顶点色，无纹理无光照）
    std::shared_ptr<IShader> m_gizmoVertShader;
    std::shared_ptr<IShader> m_gizmoFragShader;
    std::shared_ptr<IPipelineState> m_gizmoPSO;
};

} // namespace Prisma::Graphic
