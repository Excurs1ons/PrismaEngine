#include "CanvasPass2D.h"
#include "Graphics2D.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"

namespace Prisma::Graphic {

CanvasPass2D::CanvasPass2D()
    : ForwardRenderPass("CanvasPass2D") {
    m_priority = 100; // 在不透明物体之后渲染
}

CanvasPass2D::~CanvasPass2D() {}

void CanvasPass2D::Update(Prisma::Timestep ts) {
    UpdateTime(ts);
}

void CanvasPass2D::Execute([[maybe_unused]] const PassExecutionContext& context) {
    // 无需在此处执行逻辑：该 Pass 的渲染由 Pipeline 通过 Render() 显式调用
}

void CanvasPass2D::Render(ICommandBuffer* cmd, IRenderDevice* device) {
    if (!cmd || !device) return;
    
    // 执行 Graphics2D 的指令队列
    Graphics2D::Execute(cmd, device);
}

} // namespace Prisma::Graphic
