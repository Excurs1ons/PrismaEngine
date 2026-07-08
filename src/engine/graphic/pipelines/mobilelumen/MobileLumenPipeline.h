#pragma once

#include "Export.h"
#include "interfaces/IPipeline.h"
#include <memory>

namespace Prisma::Graphic {

class IRenderDevice;
class DepthPrePass;
class OpaquePass;
class ProceduralSkyPass;

// MobileLumen 管线:最小可渲染骨架
// DepthPrePass + OpaquePass + ProceduralSkyPass,预留 MLGI hook
class ENGINE_API MobileLumenPipeline : public IPipeline {
public:
    MobileLumenPipeline() = default;
    ~MobileLumenPipeline() override;

    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;
    void OnSceneLoaded(Scene* scene) override {}
    RenderMode GetMode() const override { return RenderMode::Mode3D_MobileLumen; }

    // MLGI hook (Phase 2 预留,当前空实现)
    virtual void UpdateGI(const RenderContext& /*ctx*/) {}

private:
    IRenderDevice* m_device = nullptr;
    bool m_initialized = false;

    std::shared_ptr<DepthPrePass> m_depthPrePass;
    std::shared_ptr<OpaquePass> m_opaquePass;
    std::shared_ptr<ProceduralSkyPass> m_skyPass;
};

} // namespace Prisma::Graphic
