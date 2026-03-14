#pragma once

#include "interfaces/IPipeline.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class DepthPrePass;
class OpaquePass;
class SkyboxPass;
class TransparentPass;

/**
 * @brief 基础前向渲染管线
 */
class ForwardPipeline : public IPipeline {
public:
    ForwardPipeline();
    ~ForwardPipeline() override;

    // IPipeline 接口
    bool Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;

private:
    IRenderDevice* m_device = nullptr;

    // 预定义的渲染通道
    std::shared_ptr<DepthPrePass> m_depthPrePass;
    std::shared_ptr<OpaquePass> m_opaquePass;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
    std::shared_ptr<TransparentPass> m_transparentPass;
};

} // namespace Prisma::Graphic
