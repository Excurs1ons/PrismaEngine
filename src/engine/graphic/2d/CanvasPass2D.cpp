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

void CanvasPass2D::Execute(const PassExecutionContext& context) {
    // 默认由 Pipeline 显式调用 Render
}

void CanvasPass2D::Render(ICommandBuffer* cmd, IRenderDevice* device) {
    if (!cmd || !device) return;
    
    // 执行 Graphics2D 的指令队列
    Graphics2D::Execute(cmd, device);
}

} // namespace Prisma::Graphic
