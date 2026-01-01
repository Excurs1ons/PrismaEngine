#include "DepthPrePass.h"

#include "ForwardRenderPassBase.h"

namespace PrismaEngine::Graphic {

DepthPrePass::DepthPrePass()
    : ForwardRenderPass("DepthPrePass") {
    // 深度预渲染优先级最高，最先执行
    m_priority = 50;
}

void DepthPrePass::Update(float deltaTime) {
    UpdateTime(deltaTime);
}

void DepthPrePass::Execute(const PassExecutionContext& context) {
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

    // TODO: 遍历场景中的不透明渲染对象并绘制深度
    // 深度预渲染只写入深度缓冲，不写入颜色缓冲
    // 使用简化的着色器，只输出深度值
    //
    // 这里需要与场景系统集成，获取所有带有 RenderComponent 的对象
    // 目前暂时跳过实际渲染逻辑
}

} // namespace PrismaEngine::Graphic
