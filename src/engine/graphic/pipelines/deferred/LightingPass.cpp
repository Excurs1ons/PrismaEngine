#include "LightingPass.h"

namespace PrismaEngine::Graphic {

LightingPass::LightingPass()
    : LogicalPass("LightingPass")
    , m_gBuffer(nullptr)
    , m_ambientLight(0.1f, 0.1f, 0.1f)
    , m_iblEnabled(true)
    , m_irradianceMap(nullptr)
    , m_prefilterMap(nullptr)
    , m_brdfLUT(nullptr) {
    // 光照通道在几何通道之后执行
    m_priority = 150;
}

void LightingPass::Update(float deltaTime) {
    UpdateTime(deltaTime);
}

void LightingPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    // 重置统计
    m_stats = {};

    // 设置视口
    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    // 设置环境光
    float ambientData[4] = {
        m_ambientLight.x,
        m_ambientLight.y,
        m_ambientLight.z,
        1.0f
    };
    context.deviceContext->SetConstantData(0, ambientData, sizeof(ambientData));

    // TODO: 绑定 G-Buffer 作为着色器资源
    // 需要绑定：
    // - Albedo + Metallic 纹理
    // - Normal + Roughness 纹理
    // - Position 纹理
    // - Depth 纹理

    // TODO: 渲染所有光源
    // 对于每个光源：
    // 1. 设置光源参数
    // 2. 如果是方向光，渲染全屏四边形
    // 3. 如果是点光源，渲染光球体
    // 4. 如果是聚光灯，渲染圆锥体
    // 5. 使用加法混合累积光照

    m_stats.lightsRendered = static_cast<uint32_t>(m_lights.size());
    m_stats.shadowCastingLights = 0;
    for (const auto& light : m_lights) {
        if (light.castShadows) {
            m_stats.shadowCastingLights++;
        }
    }

    // TODO: 应用 IBL 照明（如果启用）
    if (m_iblEnabled && m_irradianceMap && m_prefilterMap && m_brdfLUT) {
        // 绑定 IBL 纹理
        // context.deviceContext->SetTexture(m_irradianceMap, slot);
        // context.deviceContext->SetTexture(m_prefilterMap, slot);
        // context.deviceContext->SetTexture(m_brdfLUT, slot);
    }
}

void LightingPass::SetIBLTextures(ITexture* irradianceMap, ITexture* prefilterMap, ITexture* brdfLUT) {
    m_irradianceMap = irradianceMap;
    m_prefilterMap = prefilterMap;
    m_brdfLUT = brdfLUT;
}

} // namespace PrismaEngine::Graphic
