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

    // 重置统计
    m_stats = {};

    // 设置视口
    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    // 设置视图投影矩阵
    context.deviceContext->SetConstantData(0, &m_viewProjection, sizeof(PrismaMath::mat4));

    // TODO: 设置 G-Buffer 渲染目标
    // G-Buffer 包含多个渲染目标：
    // - Albedo + Metallic (RGBA8)
    // - Normal + Roughness (RGBA8)
    // - Position (RGBA16F) 或 World Space
    // - Emission (RGBA8)

    // TODO: 遍历场景中的渲染对象并绘制到 G-Buffer
    // 这里需要与场景系统集成，获取所有带有 RenderComponent 的对象
    // 目前暂时跳过实际渲染逻辑
}

} // namespace PrismaEngine::Graphic
