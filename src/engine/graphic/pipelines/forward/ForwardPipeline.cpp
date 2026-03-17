#include "ForwardPipeline.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "../SkyboxRenderPass.h"
#include "graphic/Renderer.h"
#include "Logger.h"

namespace Prisma::Graphic {

ForwardPipeline::ForwardPipeline() = default;

ForwardPipeline::~ForwardPipeline() {
    Shutdown();
}

int ForwardPipeline::Initialize(IRenderDevice* device) {
    m_device = device;
    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_transparentPass = std::make_shared<TransparentPass>();
    return 0;
}

void ForwardPipeline::Shutdown() {
    m_depthPrePass.reset();
    m_opaquePass.reset();
    m_skyboxPass.reset();
    m_transparentPass.reset();
}

void ForwardPipeline::Execute(const RenderContext& ctx) {
    if (!m_device) return;

    // 1. 获取指令队列
    const auto& commands = Renderer::GetCommandQueue();
    
    // 2. 设置全局渲染状态
    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;

    // 3. 顺序执行 Pass
    if (m_depthPrePass) {
        m_depthPrePass->SetViewMatrix(view);
        m_depthPrePass->SetProjectionMatrix(proj);
        // Execute needs PassExecutionContext, this is a placeholder
        // m_depthPrePass->Execute(...);
    }

    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->SetLights(ctx.lights);
        m_opaquePass->Execute(ctx.commandBuffer, commands);
    }

    if (m_skyboxPass) {
        m_skyboxPass->SetViewMatrix(view);
        m_skyboxPass->SetProjectionMatrix(proj);
        // m_skyboxPass->Execute(...);
    }

    if (m_transparentPass) {
        m_transparentPass->SetViewMatrix(view);
        m_transparentPass->SetProjectionMatrix(proj);
        // m_transparentPass->Execute(...);
    }
}

} // namespace Prisma::Graphic
