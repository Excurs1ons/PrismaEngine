#include "ForwardPipeline.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "graphic/Renderer.h" // 包含 Renderer
#include "Logger.h"

namespace Prisma::Graphic {

// ... 初始化和 Shutdown 保持不变 ...

void ForwardPipeline::Execute(const RenderContext& ctx) {
    if (!m_device || !ctx.commandBuffer) return;

    // 1. 获取指令队列
    const auto& commands = Renderer::GetCommandQueue();
    if (commands.empty()) return;

    // 2. 设置全局渲染状态
    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;

    // 3. 顺序执行 Pass，并将指令传递给它们
    if (m_depthPrePass) {
        m_depthPrePass->SetCameraData(view, proj);
        m_depthPrePass->Execute(ctx.commandBuffer, commands);
    }

    if (m_opaquePass) {
        m_opaquePass->SetCameraData(view, proj);
        m_opaquePass->SetLights(ctx.lights);
        m_opaquePass->Execute(ctx.commandBuffer, commands);
    }

    if (m_transparentPass) {
        m_transparentPass->SetCameraData(view, proj);
        m_transparentPass->Execute(ctx.commandBuffer, commands);
    }
}

} // namespace Prisma::Graphic
