#include "GeometryPass.h"

namespace PrismaEngine::Graphic {

GeometryPass::GeometryPass()
    : ForwardRenderPass("GeometryPass")
    , m_gBuffer(nullptr)
    , m_depthPrePass(true) {
    // 几何通道在延迟渲染中最早执行
    m_priority = 50;
}

void GeometryPass::Update(float deltaTime) {
    UpdateTime(deltaTime);
}

void GeometryPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    m_stats = {};

    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    context.deviceContext->SetConstantData(0, &m_viewProjection, sizeof(PrismaMath::mat4));

    if (m_gBuffer) {
        m_gBuffer->SetAsRenderTarget(context.deviceContext);
    }
}

} // namespace PrismaEngine::Graphic
