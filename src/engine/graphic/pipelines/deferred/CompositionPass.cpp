#include "CompositionPass.h"

namespace PrismaEngine::Graphic {

CompositionPass::CompositionPass()
    : LogicalPass("CompositionPass")
    , m_lightingBuffer(nullptr)
    , m_aoBuffer(nullptr)
    , m_bloomBuffer(nullptr) {
    // 合成通道在光照通道之后执行，但在 UI 之前
    m_priority = 900;
}

void CompositionPass::Update(float deltaTime) {
    UpdateTime(deltaTime);
}

void CompositionPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    m_stats = {};
    m_stats.postProcessEffects = 0;

    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    if (m_postProcessSettings.toneMapping) m_stats.postProcessEffects++;
    if (m_postProcessSettings.gammaCorrection) m_stats.postProcessEffects++;
    if (m_postProcessSettings.fxaa) m_stats.postProcessEffects++;
    if (m_postProcessSettings.smaa) m_stats.postProcessEffects++;
    if (m_postProcessSettings.bloom) m_stats.postProcessEffects++;
    if (m_postProcessSettings.ssr) m_stats.postProcessEffects++;
    if (m_postProcessSettings.ssao) m_stats.postProcessEffects++;
    if (m_postProcessSettings.depthOfField) m_stats.postProcessEffects++;
}

void CompositionPass::SetPostProcessEffect(PostProcessEffect effect, bool enable) {
    switch (effect) {
    case PostProcessEffect::ToneMapping:
        m_postProcessSettings.toneMapping = enable;
        break;
    case PostProcessEffect::GammaCorrection:
        m_postProcessSettings.gammaCorrection = enable;
        break;
    case PostProcessEffect::FXAA:
        m_postProcessSettings.fxaa = enable;
        break;
    case PostProcessEffect::SMAA:
        m_postProcessSettings.smaa = enable;
        break;
    case PostProcessEffect::Bloom:
        m_postProcessSettings.bloom = enable;
        break;
    case PostProcessEffect::SSR:
        m_postProcessSettings.ssr = enable;
        break;
    case PostProcessEffect::SSAO:
        m_postProcessSettings.ssao = enable;
        break;
    case PostProcessEffect::DepthOfField:
        m_postProcessSettings.depthOfField = enable;
        break;
        case PostProcessEffect::None:
            break;
    }
}

bool CompositionPass::IsPostProcessEffectEnabled(PostProcessEffect effect) const {
    switch (effect) {
    case PostProcessEffect::ToneMapping:
        return m_postProcessSettings.toneMapping;
    case PostProcessEffect::GammaCorrection:
        return m_postProcessSettings.gammaCorrection;
    case PostProcessEffect::FXAA:
        return m_postProcessSettings.fxaa;
    case PostProcessEffect::SMAA:
        return m_postProcessSettings.smaa;
    case PostProcessEffect::Bloom:
        return m_postProcessSettings.bloom;
    case PostProcessEffect::SSR:
        return m_postProcessSettings.ssr;
    case PostProcessEffect::SSAO:
        return m_postProcessSettings.ssao;
    case PostProcessEffect::DepthOfField:
        return m_postProcessSettings.depthOfField;
    default:
        return false;
    }
}

void CompositionPass::SetToneMappingParams(float exposure, float gamma) {
    m_toneMappingParams.exposure = exposure;
    m_toneMappingParams.gamma = gamma;
}

void CompositionPass::SetFXAAParams(float edgeThresholdMin, float edgeThresholdMax) {
    m_fxaaParams.edgeThresholdMin = edgeThresholdMin;
    m_fxaaParams.edgeThresholdMax = edgeThresholdMax;
}

void CompositionPass::SetSSAOParams(float radius, float bias, float power) {
    m_ssaoParams.radius = radius;
    m_ssaoParams.bias = bias;
    m_ssaoParams.power = power;
}

} // namespace PrismaEngine::Graphic
