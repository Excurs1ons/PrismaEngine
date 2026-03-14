#include "LightingPass.h"

namespace Prisma::Graphic {

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

void LightingPass::Update(Timestep ts) {
    UpdateTime(ts);
}

void LightingPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    m_stats = {};

    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    float ambientData[4] = {
        m_ambientLight.x,
        m_ambientLight.y,
        m_ambientLight.z,
        1.0f
    };
    context.deviceContext->SetConstantData(0, ambientData, sizeof(ambientData));

    if (m_gBuffer) {
        m_gBuffer->BindAsShaderResources(context.deviceContext, 0);
    }

    m_stats.lightsRendered = static_cast<uint32_t>(m_lights.size());
    m_stats.shadowCastingLights = 0;
    for (const auto& light : m_lights) {
        if (light.castShadows) {
            m_stats.shadowCastingLights++;
        }
    }

    if (m_iblEnabled && m_irradianceMap && m_prefilterMap && m_brdfLUT) {
    }
}

void LightingPass::SetIBLTextures(ITexture* irradianceMap, ITexture* prefilterMap, ITexture* brdfLUT) {
    m_irradianceMap = irradianceMap;
    m_prefilterMap = prefilterMap;
    m_brdfLUT = brdfLUT;
}

} // namespace Prisma::Graphic
