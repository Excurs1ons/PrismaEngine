#include "OpaquePass.h"
#include <algorithm>

namespace PrismaEngine::Graphic {

OpaquePass::OpaquePass()
    : ForwardRenderPass("OpaquePass")
    , m_ambientColor(0.1f, 0.1f, 0.1f)
    , m_ambientIntensity(1.0f) {
    // 不透明物体优先级较低，最先渲染
    m_priority = 100;
}

void OpaquePass::Update(float deltaTime) {
    UpdateTime(deltaTime);
}

void OpaquePass::Execute(const PassExecutionContext& context) {
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

    // 设置环境光
    float ambientData[4] = {
        m_ambientColor.x * m_ambientIntensity,
        m_ambientColor.y * m_ambientIntensity,
        m_ambientColor.z * m_ambientIntensity,
        1.0f
    };
    context.deviceContext->SetConstantData(1, ambientData, sizeof(ambientData));

    // TODO: 遍历场景中的渲染对象并绘制
    // 这里需要与场景系统集成，获取所有带有 RenderComponent 的对象
    // 目前暂时跳过实际渲染逻辑

    // 设置光源数据
    if (!m_lights.empty()) {
        // 光源数据包含: position(3) + color(4) + direction(3) + type(1) = 11 floats
        std::vector<float> lightData;
        lightData.reserve(m_lights.size() * 11);

        for (const auto& light : m_lights) {
            lightData.push_back(light.position.x);
            lightData.push_back(light.position.y);
            lightData.push_back(light.position.z);

            lightData.push_back(light.color.x);
            lightData.push_back(light.color.y);
            lightData.push_back(light.color.z);
            lightData.push_back(light.color.w);

            lightData.push_back(light.direction.x);
            lightData.push_back(light.direction.y);
            lightData.push_back(light.direction.z);

            lightData.push_back(static_cast<float>(light.type));
        }

        context.deviceContext->SetConstantData(2, lightData.data(),
            static_cast<uint32_t>(lightData.size() * sizeof(float)));

        uint32_t lightCount = static_cast<uint32_t>(m_lights.size());
        context.deviceContext->SetConstantData(3, &lightCount, sizeof(lightCount));
    }
}

} // namespace PrismaEngine::Graphic
