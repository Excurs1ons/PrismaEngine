#pragma once

#include "Export.h"
#include "interfaces/IPipeline.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class DepthPrePass;
class ClusteredOpaquePass;
class SkyboxPass;
class TransparentPass;
class UIPass2D;
class IShader;
class IPipelineState;
class IDescriptorSet;
class IComputePipeline;
class IBuffer;

// 分块前向渲染管线 (Clustered Forward Rendering Pipeline)
class ENGINE_API ClusteredForwardPipeline : public IPipeline {
public:
    ClusteredForwardPipeline();
    ~ClusteredForwardPipeline() override;

    // IPipeline 接口
    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;
    RenderMode GetMode() const override { return RenderMode::Mode3D_ClusteredForward; }

private:
    void RenderOverlay(const RenderContext& ctx);

    IRenderDevice* m_device = nullptr;
    
    // 基础 Pass
    std::shared_ptr<DepthPrePass> m_depthPrePass;
    std::shared_ptr<ClusteredOpaquePass> m_opaquePass;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
    std::shared_ptr<UIPass2D> m_uiPass;

    // 分块计算管线
    std::shared_ptr<IComputePipeline> m_buildPipeline;
    std::shared_ptr<IComputePipeline> m_cullPipeline;

    // 分块计算资源
    std::shared_ptr<IBuffer> m_clusterAABBs;
    std::shared_ptr<IBuffer> m_lightBuffer;
    std::shared_ptr<IBuffer> m_globalLightIndexList;
    std::shared_ptr<IBuffer> m_clusterGrid;
    std::shared_ptr<IBuffer> m_globalLightCount;
    std::shared_ptr<IBuffer> m_cameraUBO;

    std::shared_ptr<IDescriptorSet> m_buildDS;
    std::shared_ptr<IDescriptorSet> m_cullDS;

    // Gizmo 覆盖层管线
    std::shared_ptr<IShader> m_gizmoVertShader;
    std::shared_ptr<IShader> m_gizmoFragShader;
    std::shared_ptr<IPipelineState> m_gizmoPSO;
};

} // namespace Prisma::Graphic
