#include "ForwardPipeline.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "../SkyboxRenderPass.h"
#include "graphic/Renderer.h"
#include "graphic/RenderCommandContext.h"
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

    const auto& commands = Renderer::GetCommandQueue();
    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;
    const PrismaMath::mat4 viewProjection = proj * view;

    RenderCommandContext fallbackContext;
    IDeviceContext* deviceContext = &fallbackContext;

    SceneData sceneData;
    sceneData.camera.view = view;
    sceneData.camera.projection = proj;
    sceneData.camera.viewProjection = viewProjection;
    sceneData.camera.position = ctx.camera.position;
    sceneData.camera.nearPlane = ctx.camera.nearPlane;
    sceneData.camera.farPlane = ctx.camera.farPlane;
    sceneData.time.ts = ctx.deltaTime;
    sceneData.viewport.width = ctx.width;
    sceneData.viewport.height = ctx.height;

    PassExecutionContext passContext;
    passContext.deviceContext = deviceContext;
    passContext.sceneData = &sceneData;

    if (m_depthPrePass) {
        m_depthPrePass->SetViewMatrix(view);
        m_depthPrePass->SetProjectionMatrix(proj);
        m_depthPrePass->Execute(passContext);
    }

    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->SetLights(ctx.lights);
        m_opaquePass->Execute(passContext);
        if (ctx.commandBuffer) {
            m_opaquePass->Execute(ctx.commandBuffer, commands);
        }
    }

    if (m_skyboxPass) {
        m_skyboxPass->SetViewMatrix(view);
        m_skyboxPass->SetProjectionMatrix(proj);
        m_skyboxPass->Execute(passContext);
    }

    if (m_transparentPass) {
        m_transparentPass->SetViewMatrix(view);
        m_transparentPass->SetProjectionMatrix(proj);
        m_transparentPass->Execute(passContext);
    }
}

} // namespace Prisma::Graphic
