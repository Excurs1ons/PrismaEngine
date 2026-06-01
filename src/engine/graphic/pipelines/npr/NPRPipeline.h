#pragma once

#include "Export.h"
#include "interfaces/IPipeline.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class NPROpaquePass;
class SkyboxPass;
class PostProcessPass2D;
class UIPass2D;

/* NPR 渲染管线 */
class ENGINE_API NPRPipeline : public IPipeline {
public:
    NPRPipeline();
    ~NPRPipeline() override;

    // IPipeline 接口
    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;
    RenderMode GetMode() const override { return RenderMode::Mode3D_NPR; }

private:
    IRenderDevice* m_device = nullptr;

    std::shared_ptr<NPROpaquePass> m_nprOpaquePass;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
    std::shared_ptr<PostProcessPass2D> m_postProcessPass;
    std::shared_ptr<UIPass2D> m_uiPass;
};

} // namespace Prisma::Graphic
