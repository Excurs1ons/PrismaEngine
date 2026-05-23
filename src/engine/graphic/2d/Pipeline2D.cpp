#include "Pipeline2D.h"
#include "CanvasPass2D.h"
#include "Light2DPass.h"
#include "UIPass2D.h"
#include "PostProcessPass2D.h"
#include "graphic/Renderer.h"
#include "graphic/Renderer2D.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/pipelines/forward/OpaquePass.h"
#include "Logger.h"

namespace Prisma::Graphic {

Pipeline2D::Pipeline2D() = default;

Pipeline2D::~Pipeline2D() {
    Shutdown();
}

int Pipeline2D::Initialize(IRenderDevice* device) {
    m_device = device;
    m_lightPass = std::make_shared<Light2DPass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_opaquePass->SetDevice(device);
    m_canvasPass = std::make_shared<CanvasPass2D>();
    m_ppPass = std::make_shared<PostProcessPass2D>();
    m_uiPass = std::make_shared<UIPass2D>();
    
    LOG_INFO("Pipeline2D", "纯 2D 渲染管线初始化完成");
    return 0;
}

void Pipeline2D::Shutdown() {
    m_lightPass.reset();
    m_opaquePass.reset();
    m_canvasPass.reset();
    m_ppPass.reset();
    m_uiPass.reset();
}

void Pipeline2D::Execute(const RenderContext& ctx) {
    if (!m_device || !ctx.commandBuffer) return;

    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;

    if (!ctx.targetTexture) {
        ctx.device->BeginSwapChainRenderPass(ctx.clearColor);
    }

    // ── 1. 2D 光照预处理 (离屏) ──
    if (!ctx.targetTexture) {
        ctx.device->EndSwapChainRenderPass();
    }

    if (m_lightPass) {
        m_lightPass->SetViewMatrix(view);
        m_lightPass->SetProjectionMatrix(proj);
        m_lightPass->ExecuteLight(ctx.commandBuffer, m_device, ctx.width, ctx.height);
        // 将光照纹理传递给 Renderer2D
        Renderer2D::SetLightTexture(m_lightPass->GetLightTexture());
    }

    if (!ctx.targetTexture) {
        ctx.device->BeginSwapChainRenderPass(ctx.clearColor);
    }

    // ── 2. 渲染 Renderer2D 内容 (Sprite batching) ──
    const auto& commands = Renderer::GetCommandQueue();
    if (!commands.empty() && m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->Execute(ctx.commandBuffer, commands);
    }

    // ── 3. 世界空间 Canvas 渲染 (Graphics2D) ──
    if (m_canvasPass) {
        m_canvasPass->Render(ctx.commandBuffer, m_device);
    }

    // ── 4. 2D 后处理 ──
    if (m_ppPass) {
        // m_ppPass->Process(ctx.commandBuffer, m_device, ...);
    }

    // ── 5. 屏幕空间 UI 渲染 ──
    if (m_uiPass) {
        m_uiPass->RenderUI(ctx.commandBuffer, m_device, ctx.width, ctx.height);
    }
}

} // namespace Prisma::Graphic
