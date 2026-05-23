#pragma once

#include "Export.h"
#include "graphic/LogicalPass.h"
#include "graphic/pipelines/forward/ForwardRenderPassBase.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IGBuffer.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/Renderer.h"
#include "math/MathTypes.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class GeometryPass : public ForwardRenderPass {
public:
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t objects = 0;
        uint32_t culledObjects = 0;
    };

    GeometryPass();
    ~GeometryPass() override = default;

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;

    void Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands, IRenderDevice* device);

    void SetGBuffer(IGBuffer* gBuffer) { m_gBuffer = gBuffer; }
    IGBuffer* GetGBuffer() const { return m_gBuffer; }

    void SetDepthPrePass(bool enable) { m_depthPrePass = enable; }
    bool GetDepthPrePass() const { return m_depthPrePass; }

    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }
    void ResetStats() { m_stats = RenderStats(); }

private:
    bool EnsureDefaultPipeline(IRenderDevice* device);

    IGBuffer* m_gBuffer;
    bool m_depthPrePass;
    RenderStats m_stats;

    std::shared_ptr<IShader> m_defaultVertexShader;
    std::shared_ptr<IShader> m_defaultPixelShader;
    std::shared_ptr<IPipelineState> m_defaultPipelineState;
};

} // namespace Prisma::Graphic
