#include "TransparentPass.h"
#include <algorithm>

namespace PrismaEngine::Graphic {

TransparentPass::TransparentPass()
    : ForwardRenderPass("TransparentPass")
    , m_depthWrite(false)    // 透明物体默认不写入深度
    , m_depthTest(true)      // 但需要深度测试（只读）
{
    // 透明物体在天空盒之后渲染，但在 UI 之前
    // 透明物体需要从远到近排序（相对于相机）
    m_priority = 300;
}

void TransparentPass::Update(float deltaTime) {
    UpdateTime(deltaTime);
}

void TransparentPass::Execute(const PassExecutionContext& context) {
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

    // 设置深度写入状态
    float depthWriteValue = m_depthWrite ? 1.0f : 0.0f;
    context.deviceContext->SetConstantData(1, &depthWriteValue, sizeof(float));
}

} // namespace PrismaEngine::Graphic
