#pragma once

#include "../pipelines/forward/ForwardRenderPassBase.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"

namespace Prisma::Graphic {

/* 2D 画布渲染通道 */
class ENGINE_API CanvasPass2D : public ForwardRenderPass {
public:
    CanvasPass2D();
    ~CanvasPass2D() override;

    void Execute(const PassExecutionContext& context) override;
    void Update(Prisma::Timestep ts) override;
    
    // 显式执行渲染
    void Render(ICommandBuffer* cmd, IRenderDevice* device);
};

} // namespace Prisma::Graphic
